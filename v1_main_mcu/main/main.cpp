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

#include "main.h"
#include "ble_conn.h"
#include "wifi.h"
#include "http_handle.h"
#include "nvs_flash.h"

// BLE Instance
#ifdef ENABLE_BLE_TESTING
ble_conn_class ble_conn;
#endif

#ifdef ENABLE_WIFI_TESTING
WIFI_Class wifi;
HTTP_Class http_client;
#endif

static void http_test_task(void *pvParameters)
{
    std::string _url = "http://" + std::string(gateway_ip) + ":5000/";
    http_client.init(_url.c_str());
    // http_client.send_post_request(HTTP_UPLOAD_FILE, SD_TEST_PATH);
    vTaskDelete(NULL);
}

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
    if (sd_mounted == ESP_OK)
        sd_card.generate_5kb_test_file(SD_TEST_PATH);
    else
        ESP_LOGE(SD_TAG, "Failed to mount SD Card");
    
#ifdef ENABLE_BLE_TESTING
    // Turn on BLE state
    ble_state = ble_conn.ble_init();
    if(ble_state == BLE_STATE_ON){
        ESP_LOGI(GATTS_TABLE_TAG, "BLE initialized successfully");
    }
    else{
        ESP_LOGE(GATTS_TABLE_TAG, "Failed to initialize BLE");
    }
    vTaskDelay(100 / portTICK_PERIOD_MS); // Delay to ensure BLE is initialized before WiFi
#endif

#ifdef ENABLE_WIFI_TESTING
    wifi.init();
    wifi.connect();

    if(wifi.wifi_status() == WIFI_STATE_CONNECTED)
    {
        xTaskCreate(&http_test_task, "http_test_task", 8192, NULL, 2, NULL);
    }
#endif

    while (true)
    {
        /* code */
        // printf("Main task running...\n");
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    
}
