#include "cooker.h"

#include <zephyr.h>
#include <kernel.h>
#include <logging/log.h>
#include <device.h>

#include <adc.h>
#include <drivers/gpio.h>
#include <drivers/pwm.h>

#include "config.h"

#define	PERIOD	1000000UL	// 1000000ns = 1 kHz

u8_t output_power_level = 0;
u8_t output_io_level = 0;

s16_t input_power_level = 0;

struct device *adc_dev;
struct device *gpio_dev;
struct device *pwm_dev;

struct k_sem manual_enter_sem;

K_MUTEX_DEFINE(cooker_control_mutex);

static struct gpio_callback gpio_cb;

#define STACK_SIZE 1024
#define PRIORITY 5

K_THREAD_STACK_DEFINE(manual_power_loop_stack, STACK_SIZE);
struct k_thread manual_power_loop_data;
k_tid_t manual_power_loop_tid;

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

void cooker_init()
{
	k_sem_init(&manual_enter_sem, 1, 1);

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
	return (u8_t) input_power_level;
}

/*
 * VDD = low power
 * GND = full power
 */
void set_output_power_level(u8_t level)
{
	output_power_level = level;

	if (get_output_io_level() == 0)
		output_power_level = 0;

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
	return (u8_t) (value > 0) ;
}

void set_output_io_level(u8_t level)
{
	output_io_level = level;
	if(output_io_level)
	{
		gpio_pin_write(gpio_dev, IO_OUTPUT_PIN, 1);
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
