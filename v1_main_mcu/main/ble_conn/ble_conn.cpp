#include "ble_conn.h"

#ifdef ENABLE_BLE_TESTING

/****************** Private Structs ******************/
struct gatts_profile_inst {
    esp_gatts_cb_t gatts_cb;
    uint16_t gatts_if;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_handle;
    esp_gatt_srvc_id_t service_id;
    uint16_t char_handle;
    esp_bt_uuid_t char_uuid;
    esp_gatt_perm_t perm;
    esp_gatt_char_prop_t property;
    uint16_t descr_handle;
    esp_bt_uuid_t descr_uuid;
};

/****************** Private Define ******************/


/****************** Private Variables ******************/
// Write Event Parameters
static prepare_type_env_t prepare_write_env;
static uint8_t adv_config_done       = 0;

// BLE State
ble_state_t ble_state = BLE_STATE_NOT_ADVERTISING;

// Device Settings
device_settings_t device_settings = {
    .device_name = (char *)SAMPLE_DEVICE_NAME,
    .recording_time = 5,   // Default 5 minutes
};

// Service uuid (LSB <--> MSB)
static uint8_t service_uuid[16] = {0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00,};

// The length of adv data must be less than 31 bytes
static esp_ble_adv_data_t adv_data = {
    .set_scan_rsp        = false,
    .include_name        = true,
    .include_txpower     = false,
    .min_interval        = 0x0006,      //slave connection min interval, Time = min_interval * 1.25 msec
    .max_interval        = 0x0010,      //slave connection max interval, Time = max_interval * 1.25 msec
    .appearance          = 0x00,
    .manufacturer_len    = 0,
    .p_manufacturer_data = NULL,  
    .service_data_len    = 0,
    .p_service_data      = NULL,
    .service_uuid_len    = sizeof(service_uuid),
    .p_service_uuid      = service_uuid,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

// Scan response data
static esp_ble_adv_data_t scan_rsp_data = {
    .set_scan_rsp        = true,
    .include_name        = true,
    .include_txpower     = false,
    .min_interval        = 0x0006,
    .max_interval        = 0x0010,
    .appearance          = 0x00,
    .manufacturer_len    = 0,
    .p_manufacturer_data = NULL, 
    .service_data_len    = 0,
    .p_service_data      = NULL,
    .service_uuid_len    = sizeof(service_uuid),
    .p_service_uuid      = service_uuid,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

static esp_ble_adv_params_t adv_params = {
    .adv_int_min         = 0x20,
    .adv_int_max         = 0x40,
    .adv_type            = ADV_TYPE_IND,
    .own_addr_type       = BLE_ADDR_TYPE_PUBLIC,
    .channel_map         = ADV_CHNL_ALL,
    .adv_filter_policy   = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

/****************** GATT Profile Parameters ******************/
// One gatt-based profile one app_id and one gatts_if
static void gatts_profile_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
static struct gatts_profile_inst file_transfer_profile_tab[PROFILE_NUM] = {
    [PROFILE_APP_IDX] = {
        .gatts_cb = gatts_profile_event_handler,
        .gatts_if = ESP_GATT_IF_NONE,       /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
    },
};

/****************** GATT Service Parameters ******************/
// UUIDs
static const uint16_t primary_service_uuid          = ESP_GATT_UUID_PRI_SERVICE;
static const uint16_t character_declaration_uuid    = ESP_GATT_UUID_CHAR_DECLARE;
static const uint16_t character_client_config_uuid  = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;
static const uint16_t GATTS_SERVICE_UUID_TRANSCRIPT = 0x0052;
static const uint16_t GATTS_CHAR_UUID_FILE          = 0x5201;
static const uint16_t GATTS_SERVICE_UUID_SETTINGS   = 0x0053;
static const uint16_t GATTS_CHAR_UUID_DEVICE_NAME   = 0x5301;
static const uint16_t GATTS_CHAR_UUID_DEVICE_SETTINGS  = 0x5302;

// Characteristic Properties
static const uint8_t char_prop_read                 =  ESP_GATT_CHAR_PROP_BIT_READ;
static const uint8_t char_prop_write                = ESP_GATT_CHAR_PROP_BIT_WRITE;
static const uint8_t char_prop_read_write           = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE;
static const uint8_t char_prop_notify               =  ESP_GATT_CHAR_PROP_BIT_NOTIFY;
static const uint8_t f_execute_file_transfer        = 0x00;
static const uint8_t char_value[4]                  = {0x11, 0x22, 0x33, 0x44};

// GATTS attributes variables
uint16_t file_transfer_handle_table[FILE_TRF_NB];
uint16_t device_settings_handle_table[DEV_SETTINGS_NB];
static const uint16_t mtu_payload = MAX_MTU_SIZE - 3; // Calculate safe payload size
uint8_t file_chunk[mtu_payload];

// Full Database Description - Used to add attributes into the database
static const esp_gatts_attr_db_t file_transfer_db[FILE_TRF_NB] =
{
    // Service Declaration
    [IDX_SVC_TRANSCRIPT]        =  {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid, ESP_GATT_PERM_READ, sizeof(uint16_t), sizeof(GATTS_SERVICE_UUID_TRANSCRIPT), (uint8_t *)&GATTS_SERVICE_UUID_TRANSCRIPT}},
    // Characteristic Declaration
    [IDX_CHAR_TRANSCRIPT]     =  {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ, CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_notify}},
    // Characteristic Value
    [IDX_CHAR_VAL_TRANSCRIPT] =  {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_FILE, ESP_GATT_PERM_READ, GATTS_CHAR_VAL_LEN_MAX, sizeof(file_chunk), (uint8_t *)file_chunk}},
    // Client Characteristic Configuration Descriptor
    [IDX_CHAR_CFG_TRANSCRIPT]  = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, sizeof(uint16_t), sizeof(f_execute_file_transfer), (uint8_t *)&f_execute_file_transfer}},
};

static const esp_gatts_attr_db_t device_setting_db[DEV_SETTINGS_NB] =
{
    // Service Declaration
    [IDX_SVC_SETTINGS]        =  {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid, ESP_GATT_PERM_READ, sizeof(uint16_t), sizeof(GATTS_SERVICE_UUID_SETTINGS), (uint8_t *)&GATTS_SERVICE_UUID_SETTINGS}},
    // Characteristic Declaration
    [IDX_CHAR_DEVICE_NAME]     =  {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ, CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write}},
    // Characteristic Value
    [IDX_CHAR_VAL_DEVICE_NAME] =  {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_DEVICE_NAME, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, GATTS_CHAR_VAL_LEN_MAX, MAX_DEV_SETT_NAME, (uint8_t *)device_settings.device_name}},
    // Characteristic Declaration
    [IDX_CHAR_DEVICE_SETTINGS]     =  {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ, CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write}},
        // Characteristic Value
    [IDX_CHAR_VAL_DEVICE_SETTINGS] =  {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_DEVICE_SETTINGS, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, GATTS_CHAR_VAL_LEN_MAX, MAX_DEV_SETT_DATA, (uint8_t *)&device_settings.recording_time}},
};

/****************** Function Definitions ******************/
static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
        case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
            adv_config_done &= (~ADV_CONFIG_FLAG);
            if (adv_config_done == 0){
                esp_ble_gap_start_advertising(&adv_params);
            }
            break;
        case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
            adv_config_done &= (~SCAN_RSP_CONFIG_FLAG);
            if (adv_config_done == 0){
                esp_ble_gap_start_advertising(&adv_params);
            }
            break;
        case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
            /* advertising start complete event to indicate advertising start successfully or failed */
            if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(GATTS_TABLE_TAG, "advertising start failed");
            }else{
                ESP_LOGI(GATTS_TABLE_TAG, "advertising start successfully");
            }
            break;
        case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
            if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(GATTS_TABLE_TAG, "Advertising stop failed");
            }
            else {
                ESP_LOGI(GATTS_TABLE_TAG, "Stop adv successfully");
            }
            break;
        case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
            ESP_LOGI(GATTS_TABLE_TAG, "update connection params status = %d, conn_int = %d, latency = %d, timeout = %d",
                  param->update_conn_params.status,
                  param->update_conn_params.conn_int,
                  param->update_conn_params.latency,
                  param->update_conn_params.timeout);
            break;
        default:
            break;
    }
}

static void prepare_write_event_env(esp_gatt_if_t gatts_if, prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param)
{
    ESP_LOGI(GATTS_TABLE_TAG, "prepare write, handle = %d, value len = %d", param->write.handle, param->write.len);
    esp_gatt_status_t status = ESP_GATT_OK;

    if (param->write.offset > PREPARE_BUF_MAX_SIZE) {
        status = ESP_GATT_INVALID_OFFSET;
    } 
    else if ((param->write.offset + param->write.len) > PREPARE_BUF_MAX_SIZE) {
        status = ESP_GATT_INVALID_ATTR_LEN;
    }
    if (status == ESP_GATT_OK && prepare_write_env->prepare_buf == NULL) {
        prepare_write_env->prepare_buf = (uint8_t *)malloc(PREPARE_BUF_MAX_SIZE * sizeof(uint8_t));
        prepare_write_env->prepare_len = 0;
        if (prepare_write_env->prepare_buf == NULL) {
            ESP_LOGE(GATTS_TABLE_TAG, "%s, Gatt_server prep no mem", __func__);
            status = ESP_GATT_NO_RESOURCES;
        }
    }

    /*send response when param->write.need_rsp is true */
    if (param->write.need_rsp){
        esp_gatt_rsp_t *gatt_rsp = (esp_gatt_rsp_t *)malloc(sizeof(esp_gatt_rsp_t));
        if (gatt_rsp != NULL){
            gatt_rsp->attr_value.len = param->write.len;
            gatt_rsp->attr_value.handle = param->write.handle;
            gatt_rsp->attr_value.offset = param->write.offset;
            gatt_rsp->attr_value.auth_req = ESP_GATT_AUTH_REQ_NONE;
            memcpy(gatt_rsp->attr_value.value, param->write.value, param->write.len);
            esp_err_t response_err = esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, status, gatt_rsp);
            if (response_err != ESP_OK) {
               ESP_LOGE(GATTS_TABLE_TAG, "Send response error");
            }
            free(gatt_rsp);
        }
        else{
            ESP_LOGE(GATTS_TABLE_TAG, "%s, malloc failed", __func__);
            status = ESP_GATT_NO_RESOURCES;
        }
    }
    if (status != ESP_GATT_OK){
        return;
    }
    memcpy(prepare_write_env->prepare_buf + param->write.offset,
           param->write.value,
           param->write.len);
    prepare_write_env->prepare_len += param->write.len;
}

static void exec_write_event_env(prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param)
{
    if (param->exec_write.exec_write_flag == ESP_GATT_PREP_WRITE_EXEC && prepare_write_env->prepare_buf){
        ESP_LOG_BUFFER_HEX(GATTS_TABLE_TAG, prepare_write_env->prepare_buf, prepare_write_env->prepare_len);
    }
    else{
        ESP_LOGI(GATTS_TABLE_TAG,"ESP_GATT_PREP_WRITE_CANCEL");
    }
    if (prepare_write_env->prepare_buf) {
        free(prepare_write_env->prepare_buf);
        prepare_write_env->prepare_buf = NULL;
    }
    prepare_write_env->prepare_len = 0;
}

static void gatts_profile_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    switch (event) 
    {
        case ESP_GATTS_REG_EVT:{
            esp_err_t set_dev_name_ret = esp_ble_gap_set_device_name(SAMPLE_DEVICE_NAME);
            if (set_dev_name_ret){
                ESP_LOGE(GATTS_TABLE_TAG, "set device name failed, error code = %x", set_dev_name_ret);
            }
            //config adv data
            esp_err_t ret = esp_ble_gap_config_adv_data(&adv_data);
            if (ret){
                ESP_LOGE(GATTS_TABLE_TAG, "config adv data failed, error code = %x", ret);
            }
            adv_config_done |= ADV_CONFIG_FLAG;
            //config scan response data
            ret = esp_ble_gap_config_adv_data(&scan_rsp_data);
            if (ret){
                ESP_LOGE(GATTS_TABLE_TAG, "config scan response data failed, error code = %x", ret);
            }
            adv_config_done |= SCAN_RSP_CONFIG_FLAG;
            
            esp_err_t create_attr_ret = esp_ble_gatts_create_attr_tab(file_transfer_db, gatts_if, FILE_TRF_NB, FILE_TRF_INST_ID);
            if (create_attr_ret){
                ESP_LOGE(GATTS_TABLE_TAG, "create attr table failed, error code = %x", create_attr_ret);
            }

            create_attr_ret = esp_ble_gatts_create_attr_tab(device_setting_db, gatts_if, DEV_SETTINGS_NB, DEV_SETT_INST_ID);
            if (create_attr_ret){
                ESP_LOGE(GATTS_TABLE_TAG, "create attr table failed, error code = %x", create_attr_ret);
            }
       	    break;
        }
        case ESP_GATTS_READ_EVT:
            ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_READ_EVT");
       	    break;
        case ESP_GATTS_WRITE_EVT:{
            if (!param->write.is_prep){
                // the data length of gattc write  must be less than GATTS_CHAR_VAL_LEN_MAX.
                ESP_LOGI(GATTS_TABLE_TAG, "GATT_WRITE_EVT, handle = %d, value len = %d, value :", param->write.handle, param->write.len);
                ESP_LOG_BUFFER_HEX(GATTS_TABLE_TAG, param->write.value, param->write.len);
                
                // Notify to the client for file transfer
                if (file_transfer_handle_table[IDX_CHAR_CFG_TRANSCRIPT] == param->write.handle && param->write.len == 2){
                    if (param->write.value[0] == 0x01){
                        ESP_LOGI(GATTS_TABLE_TAG, "Notify enabled");

                        // Read a chunk of file from SD Card
                        if (sd_mounted == ESP_OK){
                            size_t offset = 0;
                            int bytes_read = 0;
                            int total_bytes_read = 0;
                            int check_file_size = sd_card.check_file_size(SD_TEST_PATH);
                            
                            ESP_LOGI(SD_TAG, "Reading file %s (%d bytes)", SD_TEST_PATH, check_file_size);
                            while(total_bytes_read < check_file_size )
                            {
                                bytes_read = sd_card.read_file(SD_TEST_PATH, file_chunk, offset, mtu_payload);
                                ESP_LOGI(SD_TAG, "%s", file_chunk);
                                esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, file_transfer_handle_table[IDX_CHAR_VAL_TRANSCRIPT], sizeof(file_chunk), file_chunk, false);
                                
                                // Increment starting point to read next chunk
                                offset += bytes_read;
                                // Accumulate total bytes read
                                total_bytes_read += bytes_read;
                                memset(file_chunk, 0, sizeof(file_chunk)); // Clear the buffer
                            }
                        }
                        
                        // uint8_t notify_data[MAX_MTU_SIZE-3]; //make sure this array is less than MTU size
                        // for (int i = 0; i < sizeof(notify_data); ++i)
                        // {
                        //     notify_data[i] = i % 0xff;
                        // }
                        // esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, file_transfer_handle_table[IDX_CHAR_VAL_TRANSCRIPT], sizeof(notify_data), notify_data, false);
                    }
                    else if (param->write.value[0] == 0x00){
                        ESP_LOGI(GATTS_TABLE_TAG, "Notify disabled");
                    }
                    else{
                        ESP_LOGE(GATTS_TABLE_TAG, "Unknown descr value");
                        ESP_LOG_BUFFER_HEX(GATTS_TABLE_TAG, param->write.value, param->write.len);
                    }
                }
                else if (device_settings_handle_table[IDX_CHAR_VAL_DEVICE_NAME] == param->write.handle){
                    memcpy(device_settings.device_name, "", MAX_DEV_SETT_NAME);
                    for(uint8_t i = 0; i < param->write.len; i++) {
                        device_settings.device_name[i] = param->write.value[i];
                    }
                    ESP_LOGI(GATTS_TABLE_TAG, "New Device Name Received: %s", device_settings.device_name);
                }
                else if (device_settings_handle_table[IDX_CHAR_VAL_DEVICE_SETTINGS] == param->write.handle){
                    device_settings.recording_time = param->write.value[0];
                    ESP_LOGI(GATTS_TABLE_TAG, "New Device Settings Received: %d", device_settings.recording_time);
                }

                /* send response when param->write.need_rsp is true*/
                if (param->write.need_rsp){
                    esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
                }
            }
            else{
                /* handle prepare write */
                prepare_write_event_env(gatts_if, &prepare_write_env, param);
            }
      	    break;
        }
        case ESP_GATTS_EXEC_WRITE_EVT:{
            // the length of gattc prepare write data must be less than GATTS_CHAR_VAL_LEN_MAX.
            ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_EXEC_WRITE_EVT");
            exec_write_event_env(&prepare_write_env, param);
            break;
        }
        case ESP_GATTS_MTU_EVT:
            ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_MTU_EVT, MTU %d", param->mtu.mtu);
            break;
        case ESP_GATTS_CONF_EVT:
            ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_CONF_EVT, status = %d, attr_handle %d", param->conf.status, param->conf.handle);
            break;
        case ESP_GATTS_START_EVT:
            ESP_LOGI(GATTS_TABLE_TAG, "SERVICE_START_EVT, status %d, service_handle %d", param->start.status, param->start.service_handle);
            break;
        case ESP_GATTS_CONNECT_EVT:{
            ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_CONNECT_EVT, conn_id = %d", param->connect.conn_id);
            ESP_LOG_BUFFER_HEX(GATTS_TABLE_TAG, param->connect.remote_bda, 6);
            esp_ble_conn_update_params_t conn_params = {0};
            memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
            /* For the iOS system, please refer to Apple official documents about the BLE connection parameters restrictions. */
            conn_params.latency = 0;
            conn_params.max_int = 0x20;    // max_int = 0x20*1.25ms = 40ms
            conn_params.min_int = 0x10;    // min_int = 0x10*1.25ms = 20ms
            conn_params.timeout = 400;    // timeout = 400*10ms = 4000ms
            //start sent the update connection parameters to the peer device.
            esp_ble_gap_update_conn_params(&conn_params);
            break;
        }
        case ESP_GATTS_DISCONNECT_EVT:{
            ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_DISCONNECT_EVT, reason = 0x%x", param->disconnect.reason);
            esp_ble_gap_start_advertising(&adv_params);
            break;
        }
        case ESP_GATTS_CREAT_ATTR_TAB_EVT:{
            if (param->add_attr_tab.status != ESP_GATT_OK){
                ESP_LOGE(GATTS_TABLE_TAG, "create attribute table failed, error code=0x%x", param->add_attr_tab.status);
            }
            else if (param->add_attr_tab.num_handle == FILE_TRF_NB){
                ESP_LOGI(GATTS_TABLE_TAG, "create attribute table successfully, the number handle = %d",param->add_attr_tab.num_handle);
                memcpy(file_transfer_handle_table, param->add_attr_tab.handles, sizeof(file_transfer_handle_table));
                esp_ble_gatts_start_service(file_transfer_handle_table[IDX_SVC_TRANSCRIPT]);
            }
            else if (param->add_attr_tab.num_handle == DEV_SETTINGS_NB){
                ESP_LOGI(GATTS_TABLE_TAG, "create attribute table successfully, the number handle = %d",param->add_attr_tab.num_handle);
                memcpy(device_settings_handle_table, param->add_attr_tab.handles, sizeof(device_settings_handle_table));
                esp_ble_gatts_start_service(device_settings_handle_table[IDX_SVC_SETTINGS]);
            }
            else {
                
                ESP_LOGE(GATTS_TABLE_TAG, "create attribute table abnormally, num_handle (%d) \
                        doesn't equal to FILE_TRF_NB(%d)", param->add_attr_tab.num_handle, FILE_TRF_NB);
            }
            break;
        }
        case ESP_GATTS_STOP_EVT:
        case ESP_GATTS_OPEN_EVT:
        case ESP_GATTS_CANCEL_OPEN_EVT:
        case ESP_GATTS_CLOSE_EVT:
        case ESP_GATTS_LISTEN_EVT:
        case ESP_GATTS_CONGEST_EVT:
        case ESP_GATTS_UNREG_EVT:
        case ESP_GATTS_DELETE_EVT:
        default:
            break;
    }
}

static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    /* If event is register event, store the gatts_if for each profile */
    if (event == ESP_GATTS_REG_EVT) {
        if (param->reg.status == ESP_GATT_OK) {
            file_transfer_profile_tab[PROFILE_APP_IDX].gatts_if = gatts_if;
        } 
        else {
            ESP_LOGE(GATTS_TABLE_TAG, "reg app failed, app_id %04x, status %d",
                    param->reg.app_id,
                    param->reg.status);
            return;
        }
    }
    do {
        int idx;
        for (idx = 0; idx < PROFILE_NUM; idx++) {
            /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
            if (gatts_if == ESP_GATT_IF_NONE || gatts_if == file_transfer_profile_tab[idx].gatts_if) {
                if (file_transfer_profile_tab[idx].gatts_cb) {
                    file_transfer_profile_tab[idx].gatts_cb(event, gatts_if, param);
                }
            }
        }
    } while (0);
}

ble_state_t ble_conn_class::ble_init()
{
    esp_err_t ret;
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ble_state = ERR_BLE_BT_CONTROLLER_INIT;
        ESP_LOGE(GATTS_TABLE_TAG, "%s enable controller failed: %s", __func__, esp_err_to_name(ret));
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        ble_state = ERR_BLE_BT_CONTROLLER_ENABLE;
        ESP_LOGE(GATTS_TABLE_TAG, "%s enable controller failed: %s", __func__, esp_err_to_name(ret));
    }

    ret = esp_bluedroid_init();
    if (ret) {
        ble_state = ERR_BLE_BLUEDROID_INIT;
        ESP_LOGE(GATTS_TABLE_TAG, "%s init bluetooth failed: %s", __func__, esp_err_to_name(ret));
    }

    ret = esp_bluedroid_enable();
    if (ret) {
        ble_state = ERR_BLE_BLUEDROID_ENABLE;
        ESP_LOGE(GATTS_TABLE_TAG, "%s enable bluetooth failed: %s", __func__, esp_err_to_name(ret));
    }

    ret = esp_ble_gatts_register_callback(gatts_event_handler);
    if (ret){
        ble_state = ERR_BLE_GATTS_REG_CALLBACK;
        ESP_LOGE(GATTS_TABLE_TAG, "gatts register error, error code = %x", ret);
    }

    ret = esp_ble_gap_register_callback(gap_event_handler);
    if (ret){
        ble_state = ERR_BLE_GAP_REG_CALLBACK;
        ESP_LOGE(GATTS_TABLE_TAG, "gap register error, error code = %x", ret);
    }

    ret = esp_ble_gatts_app_register(ESP_APP_ID);
    if (ret){
        ble_state = ERR_BLE_GATTS_APP_REGISTER;
        ESP_LOGE(GATTS_TABLE_TAG, "gatts app register error, error code = %x", ret);
    }

    ret = esp_ble_gatt_set_local_mtu(MAX_MTU_SIZE);
    if (ret){
        ble_state = ERR_BLE_SET_LOCAL_MTU;
        ESP_LOGE(GATTS_TABLE_TAG, "set local  MTU failed, error code = %x", ret);
    }

    /// BLE State
    if (ret == ESP_OK)
        ble_state = BLE_STATE_ADVERTISING;
    return ble_state;
}


void ble_conn_class::ble_start_advertising(void)
{
    esp_err_t ret = esp_ble_gap_start_advertising(&adv_params);
    if (ret != ESP_OK){
        ESP_LOGE(GATTS_TABLE_TAG, "Start advertising failed, error code = %x", ret);
    }
    else{
        ble_state = BLE_STATE_ADVERTISING;
        ESP_LOGI(GATTS_TABLE_TAG, "Start advertising successfully");
    }
}

void ble_conn_class::ble_stop_advertising(void)
{
    esp_err_t ret;
    ret = esp_ble_gap_stop_advertising();
    if (ret != ESP_OK){
        ESP_LOGE(GATTS_TABLE_TAG, "Stop advertising failed, error code = %x", ret);
    }
    else{
        ble_state = BLE_STATE_NOT_ADVERTISING;
        ESP_LOGI(GATTS_TABLE_TAG, "Stop advertising successfully");
    }
}

#endif // ENABLE_BLE_TESTING