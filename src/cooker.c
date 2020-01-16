#include "cooker.h"

#include <zephyr.h>
#include <logging/log.h>
#include <device.h>

#include <drivers/gpio.h>
#include <drivers/pwm.h>

#include "settings.h"

#define	PERIOD	1000000UL	// 1000000ns = 1 kHz

u8_t output_power_level = 0;
u8_t output_io_level = 0;

struct device *gpio_dev;
struct device *pwm_dev;

static struct gpio_callback gpio_cb;

#define STACK_SIZE 1024
#define PRIORITY 5

K_THREAD_STACK_DEFINE(manual_power_loop_stack, STACK_SIZE);
struct k_thread manual_power_loop_data;
k_tid_t manual_power_loop_tid;

LOG_MODULE_REGISTER(cooker);

void manual_power_loop()
{
	u32_t io_input_val = 0U;

	while(1) {
		gpio_pin_read(gpio_dev, IO_INPUT_PIN, &io_input_val);
		k_sleep(20);		
	}

	gpio_pin_enable_callback(gpio_dev, IO_INPUT_PIN);
}

void io_switched(struct device *gpiob, struct gpio_callback *cb,
		    u32_t pins)
{
	gpio_pin_disable_callback(gpiob, IO_INPUT_PIN);

	k_thread_create(&manual_power_loop_data, manual_power_loop_stack,
			K_THREAD_STACK_SIZEOF(manual_power_loop_stack),
			manual_power_loop,
			NULL, NULL, NULL,
			PRIORITY + 1, 0, K_NO_WAIT);
}

void cooker_init()
{
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

	gpio_pin_configure(gpio_dev, IO_INPUT_PIN,
			   GPIO_DIR_IN | GPIO_INT | IO_INPUT_PIN_EDGE);

	gpio_init_callback(&gpio_cb, io_switched, BIT(IO_INPUT_PIN));

	gpio_add_callback(gpio_dev, &gpio_cb);
	gpio_pin_enable_callback(gpio_dev, IO_INPUT_PIN);
}

u16_t get_input_power_level()
{
	// read ADC
	return 0;
}

void set_output_power_level(u8_t level)
{
	output_power_level = level;

	u32_t period = (u32_t) level * PERIOD / 255UL;

	if (pwm_pin_set_nsec(pwm_dev, POWER_OUT_CHANNEL,
				PERIOD, period)) {
		LOG_ERR("pwm pin set fails\n");
		return;
	}

	if (pwm_pin_set_nsec(pwm_dev, STATUS_LED_CHANNEL,
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
	// read GPIO
	return 0;
}

void set_output_io_level(u8_t level)
{
	output_io_level = level;
}

u8_t get_output_io_level()
{
	return output_io_level;
}
