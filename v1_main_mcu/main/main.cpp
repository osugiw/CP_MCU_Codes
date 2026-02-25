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
#include "mic_pdm.h"
#include "NTP.h"
#include "esp_random.h"

// BLE Instance
#ifdef ENABLE_BLE_TESTING
ble_conn_class ble_conn;
#endif

#ifdef ENABLE_WIFI_TESTING
WIFI_Class wifi;
HTTP_Class http_client;
#endif

// Mic i2s
MIC_I2S mic;

/*  FreeRTOS    */
SemaphoreHandle_t sem_task;
TaskHandle_t record_task_hdl;

// #define TASK_1_RECORDING_BIT            1
// #define TASK_2_WIFI_UPLOADING_BIT       2
// #define TASK_3_BLE_ACTIVE_BIT           4
// EventGroupHandle_t xEventGroup;

// Private Variables
static esp_err_t uploadStatus = ESP_FAIL;

#ifdef ENABLE_WIFI_TESTING
NTP ntp;
static void upload_file_task(void *pvParameters)
{
    for(;;)
    {
        if(xSemaphoreTake(sem_task, portMAX_DELAY) == pdPASS)
        {   
            http_client.init(HTTP_ROOT_URL);
            // http_client.send_post_request(HTTP_UPLOAD_FILE, SD_TEST_PATH);

            // Upload if SD Card i mounted and WiFi is connected
            if(sd_mounted == ESP_OK && wifi.wifi_status() == WIFI_STATE_CONNECTED)
            {   
                std::vector<std::string> listFile = sd_card.list_directory("/sdcard");
                // Print files inside the directory for debugging
                // sd_card.print_directory(listFile);

//                 // Upload file when the SD Card isn't empty
//                 if(listFile.size() != 0)
//                 {   
// #ifdef GENERATE_DUMMY_5KB_FILE
//                     uploadStatus = http_client.send_post_request(HTTP_UPLOAD_URL, listFile[0].c_str());
// #else
//                     uploadStatus = http_client.uploadAACFile(listFile[0].c_str());
// #endif   
//                     if(uploadStatus == ESP_OK){
//                         sd_card.remove_file(listFile[0].c_str());
//                     }
//                 }        
//                 // Trying to reconnect to the WiFi while still recording
//                 else if(wifi.wifi_status() == WIFI_STATE_DISCONNECTED)
//                 {
//                     wifi_retry_num = 0;
//                     esp_wifi_connect();
//                 }
                xSemaphoreGive(sem_task);
            }
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }
}
#endif

/*  Task Record and Save    */
void record_and_save_task(void *args)
{
    std::string fileName;
    std::string currentDateTime;
    uint32_t randomName;

    for(;;)
    {   
        if(xSemaphoreTake(sem_task, portMAX_DELAY) == pdPASS)
        {   
            // if(ulTaskNotifyTake(pdTRUE, portMAX_DELAY) > 0)
            // {
            //     xEventGroupSetBits(xEventGroup, TASK_1_RECORDING_BIT);
                // If SD is mounted properly
                if(sd_mounted == ESP_OK)
                {
                    fileName.clear();
                    currentDateTime = ntp.currentDateTime();

                    if(currentDateTime.empty())
                    {
                        randomName = esp_random();
                        fileName.append("/sdcard/");
                        fileName.append(std::to_string(randomName));
                        fileName.append(FILE_EXTENSION);
                        ntp.forceSync();    // Force to Sync the NTP
                    }
                    else 
                    {
                        fileName.append("/sdcard/");
                        fileName.append(currentDateTime);
                        fileName.append(FILE_EXTENSION);
                        currentDateTime.clear();   // Clear currentDateTime
                    }

                    // Free up some space if the local storage size is low
                    uint32_t free_space = sd_card.check_free_space(RECORD_DURATION);
                    if(free_space > MIN_SPACE_LEFT)
                    {
                        ESP_LOGI(RECORD_TAG, "Recording is is in progress");
#ifdef GENERATE_DUMMY_5KB_FILE
                        // sd_card.generate_5kb_test_file(fileName.c_str());
#else
                        esp_err_t err = mic.i2s_record_audio_aac(RECORD_DURATION, fileName.c_str());
#endif
                    //     if(err != ESP_OK)
                    //     {
                    //         esp_restart();
                    //     }
                    }
                }
                else
                {
                    ESP_LOGE(RECORD_TAG, "Restart the ESP since SD was error");
                    esp_restart();
                }
            // }     
            xSemaphoreGive(sem_task);
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

extern "C" void app_main(void)
{
    esp_err_t ret;

    // Initialize SD Card
    sd_mounted = sd_card.initialize();

    // Initialize the MIC I2S
    mic.init_pdm();

    /* Initialize NVS. */
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

#ifdef ENABLE_WIFI_TESTING
    wifi.init();
    wifi.connect();

    if(wifi.wifi_status() == WIFI_STATE_CONNECTED)
    {
        ntp.initialize();
        ntp.getSyncStatus();
    }
#endif

    // Create the semaphore
    sem_task = xSemaphoreCreateBinary();
    xSemaphoreGive(sem_task);

    /*  Record and Save Task  */
    xTaskCreatePinnedToCore(
        record_and_save_task,                   // Task code
        "Record Audio Task",                    // Task name
        8 * 1024,                               // Stack size
        NULL,                                   // Parameter to be passed
        5,                                      // Task Priority
        NULL,                       // Task Handle
        1                                       // Core number
    );

    // /*  Record and Save Task  */
    // xTaskCreatePinnedToCore(
    //     record_and_save_task,                   // Task code
    //     "Record Audio Task",                    // Task name
    //     8 * 1024,                               // Stack size
    //     NULL,                                   // Parameter to be passed
    //     5,                                      // Task Priority
    //     &record_task_hdl,                       // Task Handle
    //     1                                       // Core number
    // );

#ifdef ENABLE_WIFI_TESTING
    // if(wifi.wifi_status() == WIFI_STATE_CONNECTED)
    // {
    //      xTaskCreatePinnedToCore(
    //         upload_file_task,                         // Task code
    //         "HTTP Test Task",                       // Task name
    //         8 * 1024,                               // Stack size
    //         record_task_hdl,                        // Parameter to be passed
    //         3,                                      // Task Priority
    //         NULL,                                   // Task Handle
    //         0                                       // Core number
    //     );
    // }

    if(wifi.wifi_status() == WIFI_STATE_CONNECTED)
    {
         xTaskCreatePinnedToCore(
            upload_file_task,                         // Task code
            "HTTP Test Task",                       // Task name
            8 * 1024,                               // Stack size
            NULL,                        // Parameter to be passed
            3,                                      // Task Priority
            NULL,                                   // Task Handle
            0                                       // Core number
        );
    }
#endif

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

    while (true)
    {
        /* code */
        // printf("Main task running...\n");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    
}
