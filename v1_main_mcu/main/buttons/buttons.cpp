#include "buttons.h"

void init_button(gpio_num_t gpio_num)
{
    // Configure the GPIO pin for the button
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE; // No interrupt
    io_conf.mode = GPIO_MODE_INPUT;        // Set as input mode
    io_conf.pin_bit_mask = (1ULL << gpio_num); // Bit mask for the specified GPIO pin
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE; // Enable pull-down resistor
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;    // Disable pull-up resistor
    gpio_config(&io_conf); // Apply the configuration
}

bt_enum_t button_task(bt_state_t *bt_state, gpio_num_t gpio_num)
{
    // Read the current state of the button
    bt_state->readState = gpio_get_level(gpio_num);

    // Check for state change with debounce
    if (bt_state->readState != bt_state->prevState) {
        bt_state->lastDebounceTime = get_millis();
    }

    // If the state has been stable for longer than the debounce time, update the current state
    if ((get_millis() - bt_state->lastDebounceTime) > DEBOUNCE_DELAY) {
        if (bt_state->readState != bt_state->currentState) {
            bt_state->currentState = bt_state->readState;

            // Return the button state based on the current state
            return (bt_state->currentState == 0) ? BT_PRESSED : BT_RELEASED;
        }
    }

    // Update previous state for the next iteration
    bt_state->prevState = bt_state->readState;

    return BT_RELEASED; // Default return value when no change is detected
}

uint32_t get_millis() {
    return esp_timer_get_time() / 1000;
}