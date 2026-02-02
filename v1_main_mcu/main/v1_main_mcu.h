/*
 * SPDX-FileCopyrightText: 2021 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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