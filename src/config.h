#if defined(DT_ALIAS_PWM_0_LABEL)
#define PWM_DRIVER	DT_ALIAS_PWM_0_LABEL

#if defined(DT_ALIAS_PWM_0_CH0_PIN)
#define STATUS_LED_CHANNEL	DT_ALIAS_PWM_0_CH0_PIN
#else
#error "Choose status LED PWM channel"
#endif

#if defined(DT_ALIAS_PWM_0_CH1_PIN)
#define POWER_OUT_CHANNEL	DT_ALIAS_PWM_0_CH1_PIN
#else
#error "Choose power out PWM channel"
#endif

#else
#error "Choose supported PWM driver"
#endif

#ifdef CONFIG_SOC_NRF52840_QIAA
#ifdef DT_ALIAS_ADC_0_LABEL
#include <hal/nrf_saadc.h>
#define ADC_DRIVER DT_ALIAS_ADC_0_LABEL
#define BATTERY_SENSE_PIN NRF_SAADC_INPUT_AIN2  // P0.04/AIN2
#else
#error "you need to define ADC 0"
#endif
#endif

#ifndef DT_GPIO_KEYS_BUTTON_0_GPIOS_CONTROLLER
#ifdef BUTTON0_GPIO_NAME
#define DT_ALIAS_SW0_GPIOS_CONTROLLER SW0_GPIO_NAME
#else
#error BUTTON0_GPIO_NAME or DT_GPIO_KEYS_BUTTON_0_GPIOS_CONTROLLER needs to be set in board.h
#endif
#endif
#define GPIO_DRIVER	DT_GPIO_KEYS_BUTTON_0_GPIOS_CONTROLLER

#ifdef DT_GPIO_KEYS_BUTTON_0_GPIOS_PIN
#define IO_INPUT_PIN     DT_GPIO_KEYS_BUTTON_0_GPIOS_PIN
#else
#error DT_GPIO_KEYS_BUTTON_0_GPIOS_CONTROLLER needs to be set in board.h
#endif

#ifdef DT_GPIO_KEYS_BUTTON_0_GPIOS_FLAGS
#define IO_INPUT_PIN_EDGE    (DT_GPIO_KEYS_BUTTON_0_GPIOS_FLAGS | GPIO_INT_EDGE | GPIO_INT_ACTIVE_HIGH)
#else
#error DT_GPIO_KEYS_BUTTON_0_GPIOS_FLAGS needs to be set in board.h
#endif

#ifdef DT_ALIAS_GPIO_0_LABEL
#define IO_OUTPUT_PORT	DT_ALIAS_GPIO_0_LABEL
#define IO_OUTPUT_PIN	6	// P0.06
#else
#error "Choose supported GPIO 0 driver"
#endif
