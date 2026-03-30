#pragma once

#include "main.h"
#include "driver/gpio.h"
#include "esp_timer.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Private Struct for IOs */
typedef struct {
    bool readState;
    bool currentState;
    bool prevState;
    uint32_t lastDebounceTime;
} bt_state_t;


/* Private Enum for IOs */
typedef enum {
    BT_RELEASED,
    BT_PRESSED
} bt_enum_t;


/*  Private Functions   */
/**
 * @brief Initialize the button GPIO pin
 * @param gpio_num The GPIO number to which the button is connected
 */
void init_button(gpio_num_t gpio_num);

/**
 * @brief Task function to read the button state with debounce logic
 * @param bt_state Pointer to the button state structure
 * @param gpio_num The GPIO number to which the button is connected
 * @return The current button state (BT_PRESSED or BT_RELEASED)
 */
bt_enum_t button_task(bt_state_t *bt_state, gpio_num_t gpio_num);

/**
 * @brief Get the current time in milliseconds
 * @return The current time in milliseconds
 */
uint32_t get_millis();

#ifdef __cplusplus
}
#endif