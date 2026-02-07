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

// Device settings
#define MAX_DEV_SETT_DATA           1
#define MAX_DEV_SETT_NAME           15
#define SAMPLE_DEVICE_NAME         "WEARABLE_PHI"

#define WIFI_SSID                  "PHI_LB"
#define WIFI_PASSWORD              "09876543"

// SD Test path
#define SD_TEST_PATH    "/sdcard/test.txt"

//  SD Pin assignments
#define PIN_NUM_CS      GPIO_NUM_21 
#define PIN_NUM_SCLK    GPIO_NUM_7      // D8/A8
#define PIN_NUM_MISO    GPIO_NUM_8      // D9/A9
#define PIN_NUM_MOSI    GPIO_NUM_9      // D10/A10

// Debug TAGs
static const char *SD_TAG = "SD_CARD";
static const char *GATTS_TABLE_TAG = "BLE_XIAO_S3";

#ifdef __cplusplus
}
#endif