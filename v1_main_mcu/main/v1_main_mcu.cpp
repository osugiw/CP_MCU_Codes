/**
 * This project is part of the Capstone Project
 * --------------------------------------------------------------------------
 * Author: Sugiarto Wibowo
 * Date: February 2026
 * --------------------------------------------------------------------------
 * These codes perform read TXT file from SD Card and transmit the read data
 * over BLE using GATT server with notify property.
 * 
 */

#include "v1_main_mcu.h"
#include "nvs_flash.h"

// BLE Instance
ble_conn_class ble_conn;

extern "C" void app_main(void)
{
    esp_err_t ret;

    /* Initialize NVS. */
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    // Initialize SD Card
    sd_mounted = sd_card.initialize();
    if (sd_mounted == ESP_OK){
        sd_card.generate_5kb_test_file(SD_TEST_PATH);
        // ESP_LOGI(SD_TAG, "SD Card mounted successfully");
        // sd_card.write_file(SD_TEST_PATH, (char*)"Hello, this is a test file.\nThis file is used to test SD card read and BLE transfer functionality.\nEnjoy testing!\nLorem ipsum dolor sit amet, consectetur adipiscing elit.\nLorem ipsum dolor sit amet, consectetur adipiscing elit.\nLorem ipsum dolor sit amet, consectetur adipiscing elit.\n");
        
        // // Read a chunk of file from SD Card
        // sd_card.read_file(SD_TEST_PATH, file_chunk, offset, mtu_payload);
        // ESP_LOGI(SD_TAG, "File Content: %s", file_chunk);
    }
    // else{
    //     ESP_LOGE(SD_TAG, "Failed to mount SD Card");
    // }

    // Turn on BLE state
    ble_state = ble_conn.ble_init();
    if(ble_state == BLE_STATE_ON){
        ESP_LOGI(GATTS_TABLE_TAG, "BLE initialized successfully");
    }
    else{
        ESP_LOGE(GATTS_TABLE_TAG, "Failed to initialize BLE");
    }

    while (true)
    {
        /* code */
        // printf("Main task running...\n");
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    
}
