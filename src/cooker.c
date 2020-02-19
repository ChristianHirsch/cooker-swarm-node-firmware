#include "cooker.h"

#include <zephyr.h>
#include <kernel.h>
#include <logging/log.h>
#include <device.h>

#include <adc.h>
#include <drivers/gpio.h>
#include <drivers/pwm.h>

#include "config.h"
#include "pid.h"
#include "remote_temp.h"

#define	PERIOD	1000000UL	// 1000000ns = 1 kHz

u8_t output_power_level = 0;
u8_t output_io_level = 0;

s16_t input_power_level = 0;

static struct pid_state control_loop_pid;

struct device *adc_dev;
struct device *gpio_dev;
struct device *pwm_dev;

K_SEM_DEFINE(manual_enter_sem, 1, 1);
K_SEM_DEFINE(is_in_on_loop, 1, 1);

K_MUTEX_DEFINE(cooker_control_mutex);

static struct gpio_callback gpio_cb;

#define STACK_SIZE 1024
#define PRIORITY 5

K_THREAD_STACK_DEFINE(manual_power_loop_stack, STACK_SIZE);
struct k_thread manual_power_loop_data;
k_tid_t manual_power_loop_tid;

K_THREAD_STACK_DEFINE(is_on_loop_stack, STACK_SIZE);
struct k_thread is_on_loop_data;
k_tid_t is_on_loop_tid;

K_THREAD_STACK_DEFINE(auto_control_loop_stack, STACK_SIZE);
struct k_thread auto_control_loop_data;
k_tid_t auto_control_loop_tid;

static void cooker_auto_control_loop(struct k_timer *timer);
K_TIMER_DEFINE(auto_loop_timer, cooker_auto_control_loop, NULL);

static const struct adc_channel_cfg adc_cfg = {
		.gain             = ADC_GAIN_1_6,
		.reference        = ADC_REF_VDD_1_4,
		.acquisition_time = ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 40),
		.channel_id       = 0,
		.input_positive   = BATTERY_SENSE_PIN,
};

struct adc_sequence adc_sequence = {
		.buffer      = &input_power_level,
		.buffer_size = sizeof(input_power_level),
		.resolution  = 10,
};

void set_status_led_level(u8_t level);

LOG_MODULE_REGISTER(cooker);

void manual_power_loop()
{
	k_sleep(100);	// debounce

	if(k_mutex_lock(&cooker_control_mutex, K_FOREVER) != 0)
	{
		k_sem_give(&manual_enter_sem);
		return;
	}

	while(get_input_io_level()) {

		set_output_io_level(1);

		u8_t power = get_input_power_level();
		set_output_power_level(power);

		k_sleep(20);
	}
	set_output_io_level(0);

	k_mutex_unlock(&cooker_control_mutex);

	k_sem_give(&manual_enter_sem);
}

void is_on_loop()
{
	while (get_output_io_level())
	{
		set_status_led_level(0xff);
		k_sleep(10);
		set_status_led_level(0x00);
		k_sleep(200);
		set_status_led_level(0xff);
		k_sleep(10);
		set_status_led_level(0x00);
		k_sleep(1750);
	}

	k_sem_give(&is_in_on_loop);
}

void auto_control_loop_worker()
{
	if(k_mutex_lock(&cooker_control_mutex, K_NO_WAIT) != 0) {
		k_timer_stop(&auto_loop_timer);
		return;
	}

	s32_t value  = (s32_t) remote_read_temp();
	if (value < -27315) {
		LOG_ERR("Error getting remote temperature");
		set_output_io_level(0);
		k_mutex_unlock(&cooker_control_mutex);
		return;
	}
	LOG_INF("calculate output level with %d", value);
	s32_t output = pid_calc_control_effort(&control_loop_pid, value) / 100;
	const s32_t cutoff = (255 * 500) / 3500;

	LOG_INF("Set output level %i (int_error = %i)", (output * 3500) / 255,
			control_loop_pid.int_error);
	if (output < cutoff) {
		if (get_output_io_level() != 0) {
			set_output_io_level(0);
		}
		k_timer_start(&auto_loop_timer, K_SECONDS(2), 0);
		k_mutex_unlock(&cooker_control_mutex);
		return;
	}

	if (get_output_io_level() != 1) {
		set_output_io_level(1);
	}
	output -= cutoff;
	output *= 255;
	output /= 255 - cutoff;
	set_output_power_level((u8_t)output);

	k_timer_start(&auto_loop_timer, K_SECONDS(2), 0);
	k_mutex_unlock(&cooker_control_mutex);
}

void io_switched(struct device *gpiob, struct gpio_callback *cb,
		u32_t pins)
{
	if(k_sem_take(&manual_enter_sem, K_NO_WAIT) == 0)
	{
		k_thread_create(&manual_power_loop_data, manual_power_loop_stack,
				K_THREAD_STACK_SIZEOF(manual_power_loop_stack),
				manual_power_loop,
				NULL, NULL, NULL,
				PRIORITY + 1, 0, K_NO_WAIT);
	}
}

static void cooker_auto_control_loop(struct k_timer *timer)
{
	k_thread_create(&auto_control_loop_data, auto_control_loop_stack,
			K_THREAD_STACK_SIZEOF(auto_control_loop_stack),
			auto_control_loop_worker,
			NULL, NULL, NULL,
			PRIORITY + 1, 0, K_NO_WAIT);
}

void cooker_init()
{
	k_sem_init(&manual_enter_sem, 1, 1);

	pid_initialize_state(&control_loop_pid);
	control_loop_pid.setpoint = 2750;
	control_loop_pid.p = 100;
	control_loop_pid.i =   1;
	control_loop_pid.int_error_max = 25500;
	control_loop_pid.output_max = 25500;
	control_loop_pid.output_min = 0;

	adc_dev = device_get_binding(ADC_DRIVER);
	if (!adc_dev) {
		LOG_ERR("Cannot find %s!\n", ADC_DRIVER);
		return;
	}

	gpio_dev = device_get_binding(GPIO_DRIVER);
	if(!gpio_dev) {
		LOG_ERR("Cannot find %s!\n", GPIO_DRIVER);
		return;
	}

	pwm_dev = device_get_binding(PWM_DRIVER);
	if (!pwm_dev) {
		LOG_ERR("Cannot find %s!\n", PWM_DRIVER);
		return;
	}

	adc_channel_setup(adc_dev, &adc_cfg);

	gpio_pin_configure(gpio_dev, IO_INPUT_PIN,
			GPIO_DIR_IN | GPIO_INT | GPIO_INT_DEBOUNCE | IO_INPUT_PIN_EDGE);

	gpio_init_callback(&gpio_cb, io_switched, BIT(IO_INPUT_PIN));

	gpio_add_callback(gpio_dev, &gpio_cb);
	gpio_pin_enable_callback(gpio_dev, IO_INPUT_PIN);

	gpio_pin_configure(gpio_dev, IO_OUTPUT_PIN, GPIO_DIR_OUT);
	gpio_pin_write(gpio_dev, IO_OUTPUT_PIN, 0);
}

void cooker_start_auto_control_loop(void) {
	k_timer_start(&auto_loop_timer, K_NO_WAIT, 0);
}

/* 0xa8 (min) to 0x00 (max) */
u8_t get_input_power_level()
{
	int ret = 0;

	if(adc_channel_setup(adc_dev, &adc_cfg) != 0)
		return 255;

	adc_sequence.channels = BIT(adc_cfg.channel_id);
	ret = adc_read(adc_dev, &adc_sequence);

	if(ret < 0)
		return 255;

	input_power_level = (input_power_level >> 2) & 0x00ff;
	if (input_power_level > 0xf0UL)
		input_power_level = 0;
	input_power_level *= 0xffUL;
	input_power_level /= 0xaaUL;
	input_power_level  = 255 - input_power_level;

	cos_input_power_notify((u8_t) input_power_level);
	return (u8_t) input_power_level;
}

/*
 * VDD = low power
 * GND = full power
 */
void set_output_power_level(u8_t level)
{
	LOG_INF("Set output power level to %u", level);
	return;
	output_power_level = level;

	if (get_output_io_level() == 0)
		output_power_level = 0;

	cos_output_power_notify(output_power_level);

	u32_t period = (u32_t) output_power_level * PERIOD / 255UL;

	if (pwm_pin_set_nsec(pwm_dev, POWER_OUT_CHANNEL,
			PERIOD, period)) {
		LOG_ERR("pwm pin set fails\n");
		return;
	}
}

u8_t get_output_power_level()
{
	return output_power_level;
}

u8_t get_input_io_level()
{
	u32_t value;
	gpio_pin_read(gpio_dev, IO_INPUT_PIN, &value);
	cos_input_io_notify((u8_t) (value > 0));
	return (u8_t) (value > 0) ;
}

void set_output_io_level(u8_t level)
{
	LOG_INF("Set output io level to %u", level);
	return;
	output_io_level = level;
	cos_output_io_notify(output_io_level);
	if(output_io_level)
	{
		gpio_pin_write(gpio_dev, IO_OUTPUT_PIN, 1);

		if(k_sem_take(&is_in_on_loop, K_NO_WAIT) == 0)
		{
			k_thread_create(&is_on_loop_data, is_on_loop_stack,
					K_THREAD_STACK_SIZEOF(is_on_loop_stack),
					is_on_loop,
					NULL, NULL, NULL,
					7, 0, K_NO_WAIT);
		}
	}
	else
	{
		set_output_power_level(0);
		gpio_pin_write(gpio_dev, IO_OUTPUT_PIN, 0);
	}
}

u8_t get_output_io_level()
{
	return output_io_level;
}

void set_status_led_level(u8_t level)
{
	u32_t period = (u32_t) level * PERIOD / 255UL;

	if (pwm_pin_set_nsec(pwm_dev, STATUS_LED_CHANNEL,
			PERIOD, period)) {
		LOG_ERR("pwm pin set fails\n");
	}
}
