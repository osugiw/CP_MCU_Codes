#pragma once

#include "v1_main_mcu.h"
#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gatt_common_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/****************** Public Enum ******************/
// Attributes State Machine
enum
{
    // Characteristic Indexes for File Transfer Service
    IDX_SVC_TRANSCRIPT,
    IDX_CHAR_TRANSCRIPT,
    IDX_CHAR_VAL_TRANSCRIPT,
    IDX_CHAR_CFG_TRANSCRIPT,
    
    // Service ID
    FILE_TRF_INST_ID = 0,

    // Total Number of Attributes in GATTS Database
    FILE_TRF_NB = 4,
};

enum {
    // Characteristic Indexes for Device Settings Service
    IDX_SVC_SETTINGS,
    IDX_CHAR_DEVICE_NAME,
    IDX_CHAR_VAL_DEVICE_NAME,
    IDX_CHAR_DEVICE_SETTINGS,
    IDX_CHAR_VAL_DEVICE_SETTINGS,
    
    // Service ID
    DEV_SETT_INST_ID = 1,

    // Total Number of Attributes in GATTS Database
    DEV_SETTINGS_NB = 5,
};

/****************** Public Structs ******************/
typedef enum {
    BLE_STATE_OFF,
    BLE_STATE_ON,
    
    // Init error states
    ERR_BLE_BT_CONTROLLER_INIT,
    ERR_BLE_BT_CONTROLLER_ENABLE,
    ERR_BLE_BLUEDROID_INIT,
    ERR_BLE_BLUEDROID_ENABLE,
    ERR_BLE_GATTS_REG_CALLBACK,
    ERR_BLE_GAP_REG_CALLBACK,
    ERR_BLE_GATTS_APP_REGISTER,
    ERR_BLE_SET_LOCAL_MTU,

    // Deinit error states
    ERR_BLE_BLUEDROID_DISABLE,
    ERR_BLE_BLUEDROID_DEINIT,
    ERR_BLE_BT_CONTROLLER_DISABLE,
    ERR_BLE_BT_CONTROLLER_DEINIT
} ble_state_t;

typedef struct {
    uint8_t                 *prepare_buf;
    int                     prepare_len;
} prepare_type_env_t;

/****************** Public Defines ******************/
// BLE Macros
#define PROFILE_NUM                 1
#define PROFILE_APP_IDX             0
#define ESP_APP_ID                  0x50

// The max length of characteristic value
#define MAX_MTU_SIZE                500
#define GATTS_CHAR_VAL_LEN_MAX      500
#define PREPARE_BUF_MAX_SIZE        1024
#define CHAR_DECLARATION_SIZE       (sizeof(uint8_t))

#define ADV_CONFIG_FLAG             (1 << 0)
#define SCAN_RSP_CONFIG_FLAG        (1 << 1)

// BLE Connection class
class ble_conn_class {
    public:
        ble_conn_class(){};
        ~ble_conn_class(){};

        /**
         * @brief Initialize BLE connection and GATT server
         * @return BLE state after initialization
         */
        ble_state_t ble_init(void);

        /**
         * @brief Deinitialize BLE connection and GATT server
         * @return BLE state after deinitialization
         */
        ble_state_t ble_deinit(void);
};

/* Global BLE state variable */
extern ble_state_t ble_state;

#ifdef __cplusplus
}
#endif