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

#ifdef __cplusplus
extern "C" {
#endif

/****************** Public Enum ******************/
// Attributes State Machine
enum
{
    IDX_SVC,
    IDX_CHAR_TRANSCRIPT,
    IDX_CHAR_VAL_TRANSCRIPT,
    IDX_CHAR_CFG_TRANSCRIPT,

    FILE_TRF_NB,
};

/****************** Public Define ******************/
#define ENABLE_LOGGING              1

// BLE Macros
#define PROFILE_NUM                 1
#define PROFILE_APP_IDX             0
#define ESP_APP_ID                  0x50
#define SAMPLE_DEVICE_NAME          "WEARABLE_PHI"
#define SVC_INST_ID                 0

// The max length of characteristic value
#define MAX_MTU_SIZE                500
#define GATTS_CHAR_VAL_LEN_MAX      500
#define PREPARE_BUF_MAX_SIZE        1024
#define CHAR_DECLARATION_SIZE       (sizeof(uint8_t))

#define ADV_CONFIG_FLAG             (1 << 0)
#define SCAN_RSP_CONFIG_FLAG        (1 << 1)

//  SD Pin assignments
#define PIN_NUM_CS      GPIO_NUM_21 
#define PIN_NUM_SCLK    GPIO_NUM_7
#define PIN_NUM_MISO    GPIO_NUM_8
#define PIN_NUM_MOSI    GPIO_NUM_9

static const char *SD_TAG = "SD_CARD";

#ifdef __cplusplus
}
#endif