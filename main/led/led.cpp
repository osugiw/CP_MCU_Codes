#include "led.h"

RGB_Class led;
static const char *RGB_TAG = "RGB_LED";

RMT_ENCODER_FUNC_ATTR
static size_t rmt_encode_led_strip(rmt_encoder_t *encoder, rmt_channel_handle_t channel, const void *primary_data, size_t data_size, rmt_encode_state_t *ret_state)
{
    rmt_led_strip_encoder_t *led_encoder = __containerof(encoder, rmt_led_strip_encoder_t, base);
    rmt_encoder_handle_t bytes_encoder = led_encoder->bytes_encoder;
    rmt_encoder_handle_t copy_encoder = led_encoder->copy_encoder;
    rmt_encode_state_t session_state = RMT_ENCODING_RESET;
    rmt_encode_state_t state = RMT_ENCODING_RESET;
    size_t encoded_symbols = 0;
    switch (led_encoder->state) {
    case 0: // send RGB data
        encoded_symbols += bytes_encoder->encode(bytes_encoder, channel, primary_data, data_size, &session_state);
        if (session_state & RMT_ENCODING_COMPLETE) {
            led_encoder->state = 1; // switch to next state when current encoding session finished
        }
        if (session_state & RMT_ENCODING_MEM_FULL) {
            state |= RMT_ENCODING_MEM_FULL;
            goto out; // yield if there's no free space for encoding artifacts
        }
    // fall-through
    case 1: // send reset code
        encoded_symbols += copy_encoder->encode(copy_encoder, channel, &led_encoder->reset_code,
                                                sizeof(led_encoder->reset_code), &session_state);
        if (session_state & RMT_ENCODING_COMPLETE) {
            led_encoder->state = RMT_ENCODING_RESET; // back to the initial encoding session
            state |= RMT_ENCODING_COMPLETE;
        }
        if (session_state & RMT_ENCODING_MEM_FULL) {
            state |= RMT_ENCODING_MEM_FULL;
            goto out; // yield if there's no free space for encoding artifacts
        }
    }
out:
    *ret_state = state;
    return encoded_symbols;
}

static esp_err_t rmt_del_led_strip_encoder(rmt_encoder_t *encoder)
{
    rmt_led_strip_encoder_t *led_encoder = __containerof(encoder, rmt_led_strip_encoder_t, base);
    rmt_del_encoder(led_encoder->bytes_encoder);
    rmt_del_encoder(led_encoder->copy_encoder);
    free(led_encoder);
    return ESP_OK;
}

RMT_ENCODER_FUNC_ATTR
static esp_err_t rmt_led_strip_encoder_reset(rmt_encoder_t *encoder)
{
    rmt_led_strip_encoder_t *led_encoder = __containerof(encoder, rmt_led_strip_encoder_t, base);
    rmt_encoder_reset(led_encoder->bytes_encoder);
    rmt_encoder_reset(led_encoder->copy_encoder);
    led_encoder->state = RMT_ENCODING_RESET;
    return ESP_OK;
}

esp_err_t RGB_Class::rmt_new_led_strip_encoder(void)
{
    esp_err_t ret = ESP_OK;
    
    // Declare all local variables at the top so 'goto err' doesn't cross initializations
    rmt_bytes_encoder_config_t bytes_encoder_config = {};
    rmt_copy_encoder_config_t copy_encoder_config = {};
    uint32_t reset_ticks = 0;
    
    ESP_LOGI(RGB_TAG, "Create RMT TX channel");
    
    rmt_tx_channel_config_t tx_chan_config = {};
    tx_chan_config.gpio_num = RMT_LED_STRIP_GPIO_NUM;
    tx_chan_config.clk_src = RMT_CLK_SRC_DEFAULT;
    tx_chan_config.resolution_hz = RMT_LED_STRIP_RESOLUTION_HZ;
    tx_chan_config.mem_block_symbols = 64; 
    tx_chan_config.trans_queue_depth = 4;
    
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config, &led_chan));

    ESP_LOGI(RGB_TAG, "Install led strip encoder");
    
    // Explicitly cast void* to struct pointer type
    led_encoder = (rmt_led_strip_encoder_t*)rmt_alloc_encoder_mem(sizeof(rmt_led_strip_encoder_t));
    ESP_GOTO_ON_FALSE(led_encoder, ESP_ERR_NO_MEM, err, RGB_TAG, "no mem for led strip encoder");
    
    led_encoder->base.encode = rmt_encode_led_strip;
    led_encoder->base.del = rmt_del_led_strip_encoder;
    led_encoder->base.reset = rmt_led_strip_encoder_reset;
    
    // Timing configs for ws2812b
    bytes_encoder_config.bit0.level0 = 1;
    bytes_encoder_config.bit0.duration0 = 0.3 * config.resolution / 1000000;
    bytes_encoder_config.bit0.level1 = 0;
    bytes_encoder_config.bit0.duration1 = 0.9 * config.resolution / 1000000;
    
    bytes_encoder_config.bit1.level0 = 1;
    bytes_encoder_config.bit1.duration0 = 0.9 * config.resolution / 1000000;
    bytes_encoder_config.bit1.level1 = 0;
    bytes_encoder_config.bit1.duration1 = 0.3 * config.resolution / 1000000;
    
    bytes_encoder_config.flags.msb_first = 1; // WS2812 transfer bit order

    // Pass addresses
    ESP_GOTO_ON_ERROR(rmt_new_bytes_encoder(&bytes_encoder_config, &led_encoder->bytes_encoder), err, RGB_TAG, "create bytes encoder failed");
    ESP_GOTO_ON_ERROR(rmt_new_copy_encoder(&copy_encoder_config, &led_encoder->copy_encoder), err, RGB_TAG, "create copy encoder failed");

    // Reset configs
    reset_ticks = config.resolution / 1000000 * 50 / 2; 
    led_encoder->reset_code.level0 = 0;
    led_encoder->reset_code.duration0 = reset_ticks;
    led_encoder->reset_code.level1 = 0;
    led_encoder->reset_code.duration1 = reset_ticks;

    // Assign pointer directly without dereferencing
    ret_encoder = &led_encoder->base;

    ESP_LOGI(RGB_TAG, "Enable RMT TX channel");
    ESP_ERROR_CHECK(rmt_enable(led_chan));
    ESP_LOGI(RGB_TAG, "Start LED");
    return ESP_OK;

err:
    if (led_encoder) {
        if (led_encoder->bytes_encoder) {
            rmt_del_encoder(led_encoder->bytes_encoder);
        }
        if (led_encoder->copy_encoder) {
            rmt_del_encoder(led_encoder->copy_encoder);
        }
        free(led_encoder);
    }
    return ret;
}

void RGB_Class::led_strip_hsv2rgb(uint32_t h, uint32_t s, uint32_t v, uint32_t *r, uint32_t *g, uint32_t *b)
{
    h %= 360; // h -> [0,360]
    uint32_t rgb_max = v * 2.55f;
    uint32_t rgb_min = rgb_max * (100 - s) / 100.0f;

    uint32_t i = h / 60;
    uint32_t diff = h % 60;

    // RGB adjustment amount by hue
    uint32_t rgb_adj = (rgb_max - rgb_min) * diff / 60;

    switch (i) {
    case 0:
        *r = rgb_max;
        *g = rgb_min + rgb_adj;
        *b = rgb_min;
        break;
    case 1:
        *r = rgb_max - rgb_adj;
        *g = rgb_max;
        *b = rgb_min;
        break;
    case 2:
        *r = rgb_min;
        *g = rgb_max;
        *b = rgb_min + rgb_adj;
        break;
    case 3:
        *r = rgb_min;
        *g = rgb_max - rgb_adj;
        *b = rgb_max;
        break;
    case 4:
        *r = rgb_min + rgb_adj;
        *g = rgb_min;
        *b = rgb_max;
        break;
    default:
        *r = rgb_max;
        *g = rgb_min;
        *b = rgb_max - rgb_adj;
        break;
    }
}

void RGB_Class::rmt_set_color(HSV_hue_en _h, HSV_saturation_en _s, HSV_value_en _v, bool _off)
{
    static uint8_t led_strip_pixels[3];

    if(_off)
    {
        memset(led_strip_pixels, 0, sizeof(led_strip_pixels));
        ESP_ERROR_CHECK(rmt_transmit(led_chan, ret_encoder, led_strip_pixels, sizeof(led_strip_pixels), &tx_config));
        ESP_ERROR_CHECK(rmt_tx_wait_all_done(led_chan, portMAX_DELAY));
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    else {
        led_strip_hsv2rgb(_h, _s, _v, &red, &green, &blue);
        led_strip_pixels[0] = green;
        led_strip_pixels[1] = blue;
        led_strip_pixels[2] = red;

        // Flush RGB values to LEDs
        ESP_ERROR_CHECK(rmt_transmit(led_chan, ret_encoder, led_strip_pixels, sizeof(led_strip_pixels), &tx_config));
        ESP_ERROR_CHECK(rmt_tx_wait_all_done(led_chan, portMAX_DELAY));
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}