#pragma once

#include "main.h"
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

enum led_state_t {
    LED_OFF = 0,
    LED_ON
};

/**
 * @brief  Initialize LED
 * @param  gpio_num GPIO number for the LED
 * @return None
 */
void led_init(gpio_num_t gpio_num);

/**
 * @brief  Set LED state
 * @param  gpio_num GPIO number for the LED
 * @param  on  true to turn on the LED, false to turn it off
 * @return None
 */
void led_set_state(gpio_num_t gpio_num, bool on);


#ifdef __cplusplus
}
#endif