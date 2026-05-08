#pragma once

#include "main.h"
#include "driver/rmt_encoder.h"
#include "driver/rmt_tx.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Enum for LED HSV Value  */
typedef enum {
    RED = 0,
    GREEN = 120,
    BLUE = 240,
    WHITE = 0,
} HSV_hue_en;

typedef enum {
    UNSATURATED = 0,
    HALF_PURE = 50,
    PURE = 100,
} HSV_saturation_en;

typedef enum {
    BLACK = 0,
    LOW_BRIGHTNESS = 10,
    HALF_BRIGHTNESS = 50,
    MAX_BRIGHTNESS = 100
} HSV_value_en;

/**
 * @brief Type of led strip encoder configuration
 */
typedef struct {
    uint32_t resolution; /*!< Encoder resolution, in Hz */
} led_strip_encoder_config_t;
typedef struct {
    rmt_encoder_t base;
    rmt_encoder_t *bytes_encoder;
    rmt_encoder_t *copy_encoder;
    int state;
    rmt_symbol_word_t reset_code;
} rmt_led_strip_encoder_t;

#define RMT_LED_STRIP_RESOLUTION_HZ 10000000 // 10MHz resolution, 1 tick = 0.1us (led strip needs a high resolution)


class RGB_Class{
    private:
        rmt_channel_handle_t led_chan = NULL;       // LED Channel
        rmt_led_strip_encoder_t *led_encoder = NULL;

        rmt_encoder_handle_t ret_encoder = NULL;    // [in] config Encoder configuration
        led_strip_encoder_config_t config = {
            .resolution = RMT_LED_STRIP_RESOLUTION_HZ,  // [out] ret_encoder Returned encoder handle
        };
        rmt_transmit_config_t tx_config = {
            .loop_count = 0, // no transfer loop
        };

        uint32_t red = 0;
        uint32_t green = 0;
        uint32_t blue = 0;

        // Converting HSV ro RGB
        void led_strip_hsv2rgb(uint32_t h, uint32_t s, uint32_t v, uint32_t *r, uint32_t *g, uint32_t *b);

    public:
        RGB_Class(){};
        ~RGB_Class(){};
        
        /**
         * @brief Create RMT encoder for encoding LED strip pixels into RMT symbols and initialize
         * @return
         *      - ESP_ERR_INVALID_ARG for any invalid arguments
         *      - ESP_ERR_NO_MEM out of memory when creating led strip encoder
         *      - ESP_OK if creating encoder and initialization successfull
         */
        esp_err_t rmt_new_led_strip_encoder(void);

        /**
         * @brief Color Set for RGB LED usgin HSV Format
         * @param _h    Hue value
         * @param _s    Saturation value
         * @param _v    Value
         * @param _off  Set LED off
         */
        void rmt_set_color(HSV_hue_en _h, HSV_saturation_en _s, HSV_value_en _v,  bool _off);
};


// Make the class public
extern RGB_Class led;

#ifdef __cplusplus
}
#endif