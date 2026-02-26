#pragma once

#include <stdint.h>
#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <vector>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"

// Libraries
#include "sd_card.h"

#ifdef __cplusplus
extern "C" {
#endif

/****************** Public Struct ******************/
typedef struct {
    char*       device_name;
    uint8_t     recording_time;   // In minutes
} device_settings_t;

/****************** Public Define ******************/
#define ENABLE_LOGGING              1

// Testing Purposes
#define ENABLE_WIFI_TESTING             
// #define ENABLE_BLE_TESTING         
// #define GENERATE_DUMMY_5KB_FILE     

// Device settings
#define MAX_DEV_SETT_DATA           1
#define MAX_DEV_SETT_NAME           15
#define SAMPLE_DEVICE_NAME         "WEARABLE_PHI"

// WiFi Parameters
#define WIFI_SSID                   "ASUS"
#define WIFI_PASSWORD               "brkbrkbrkb09"

// SD Card Paremeters
#define MAX_SD_READ_SIZE            512
#define SD_TEST_PATH                "/sdcard/test.txt"

//  SD Pin assignments
#define PIN_NUM_CS              GPIO_NUM_21      // D2/A2 
#define PIN_NUM_SCLK            GPIO_NUM_7      // D8/A8
#define PIN_NUM_MISO            GPIO_NUM_8      // D9/A9
#define PIN_NUM_MOSI            GPIO_NUM_9      // D10/A10

// PDM Mic Parameters
#define GPIO_I2S_DATA_IO        GPIO_NUM_41     // I2S data in io number
#define GPIO_I2S_CLK_IO         GPIO_NUM_42     // I2S bit clock io number

/**
 * @brief   I2S Microphone configuration
 *          For calculating DMA refer to this link:
 *          https://docs.espressif.com/projects/esp-idf/en/v5.4/esp32/api-reference/peripherals/i2s.html
 */
#define I2S_SAMPLE_RATE                     48000
#define I2S_BIT_SAMPLE                      16
#define I2S_CHANNEL_DMA_FRAME_NUM           2046    //2046 x 2 = 4092 (ESP32 Limitation) -- the number of samples in one DMA buffer, which affects the frequency of DMA interrupts and the size of data processed in each interrupt. Larger value means less frequent interrupts and more data processed in each interrupt, but also means higher latency and more RAM usage.>
#define I2S_CHANNEL_DMA_DESC_NUM            6
#define I2S_CHANNEL_DMA_BUFFERS_SIZE        (((I2S_CHANNEL_DMA_FRAME_NUM + 2) * I2S_SLOT_MODE_MONO * I2S_BIT_SAMPLE) / 8)    // Must be <= 4092 (ESP32 Limitation)

/*  AAC Codec & Buffer Config  */
#define CODEC_BITRATE                       96000  // bps
#define AUDIO_GAIN                          10.0f    // Gain factor for volume adjustment (e.g., 1.5 for 50% increase)   
#define I2S_RECV_BUFFER_SIZE                (I2S_CHANNEL_DMA_DESC_NUM * I2S_CHANNEL_DMA_BUFFERS_SIZE)
#define AAC_CODEC_BUFFER_SIZE               (I2S_CHANNEL_DMA_FRAME_NUM + 2)

/*  Recording Config  */
#define RECORD_DURATION         10          // In seconds  
#define MIN_SPACE_LEFT          100          // In MB
#define NUM_FILES_REMOVE        5

#ifdef GENERATE_DUMMY_5KB_FILE
    #define FILE_EXTENSION          ".txt"
#else
    #define FILE_EXTENSION          ".aac"
#endif

// HTTP Client
#define HTTP_ENDPOINT           "http://httpbin.org"
#define HTTP_ROOT_URL           "http://192.168.50.194:5000/"
#define HTTP_UPLOAD_URL         "http://192.168.50.194:5000/upload"

// Debug TAGs
static const char *SD_TAG = "SD_CARD";
static const char *GATTS_TABLE_TAG = "BLE_XIAO_S3";
static const char *WIFI_TAG = "WIFI_CONN";
static const char *RECORD_TAG = "Recording";

#ifdef __cplusplus
}
#endif