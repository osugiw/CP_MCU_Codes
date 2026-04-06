/**
 * This project is part of the Capstone Project
 * --------------------------------------------------------------------------
 * Author: Sugiarto Wibowo
 * Date: March 2026
 * --------------------------------------------------------------------------
 * The main system performs recording sound from a microphone and save it to the SD Card. It will transmit the oldest
 * recording file to the server when the WiFi is connected. The file will be transmitted in AAC format. Additionally, 
 * the system support BLE service for device configuration
 * 
 */

#include "main.h"
#include "led.h"
#include "buttons.h"
#include "ble_conn.h"
#include "wifi.h"
#include "http_handle.h"
#include "nvs_flash.h"
#include "mic_pdm.h"
#include "NTP.h"
#include "esp_random.h"

/* Private Macros   */
#define TASK_FIRST_PRIORITY    12
#define TASK_SECOND_PRIORITY   10

/*  Private Variables   */
#ifdef ENABLE_BLE_TESTING
ble_conn_class ble_conn;                 // BLE Instance
#endif
#ifdef ENABLE_WIFI_TESTING
WIFI_Class wifi;            // WiFi Instance
HTTP_Class http_client;     // HTTP Client Instance
#endif
static MIC_I2S mic;                        // Mic Instance
static NTP ntp;                             // NTP Instance
static esp_err_t uploadStatus = ESP_FAIL;
SemaphoreHandle_t sem_task;         // Semaphore for task synchronization

// ------------------ Button States
bt_state_t bt_state = {BT_RELEASED, BT_RELEASED, BT_RELEASED, 0};
sys_enum_t sys_state = SYSTEM_OFF;

#ifdef ENABLE_WIFI_TESTING
void upload_file_task(void *pvParameters)
{
    for(;;)
    {
        if(xSemaphoreTake(sem_task, portMAX_DELAY) == pdPASS)
        {   
            // Upload if SD Card i mounted and WiFi is connected
            if(sd_mounted == ESP_OK && wifi.wifi_status() == WIFI_STATE_CONNECTED)
            {   
                std::vector<std::string> listFile = sd_card.list_directory("/sdcard");
                // Print files inside the directory for debugging
                // sd_card.print_directory(listFile);

                // Upload file when the SD Card isn't empty
                if(listFile.size() != 0)
                {   
                    std::string url = HTTP_ROOT_URL;
                    url.append(HTTP_UPLOAD_URL);
                    uploadStatus = http_client.uploadAACFile(url.c_str(), listFile[0]);
                    if(uploadStatus == ESP_OK || uploadStatus == ESP_ERR_NOT_SUPPORTED){
                        sd_card.remove_file(listFile[0].c_str());
                    }
                }        
                // Trying to reconnect to the WiFi while still recording
                else if(wifi.wifi_status() == WIFI_STATE_DISCONNECTED)
                {
                    wifi_retry_num = 0;
                    esp_wifi_connect();
                }
            }
            xSemaphoreGive(sem_task);
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
#endif

/*  Task Record and Save    */
void record_and_save_task(void *args)
{
    std::string fileName;
    std::string currentDateTime;
    uint32_t randomName;

    // UBaseType_t uxHighWaterMark;
    // uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );

    for(;;)
    {   
        if(xSemaphoreTake(sem_task, portMAX_DELAY) == pdPASS)
        {  
            // Record and save if the system is ON
            if(sys_state == SYSTEM_ON)
            {
                if(sd_mounted == ESP_OK)
                {
                    fileName.clear();
                    currentDateTime = ntp.currentDateTime();

                    if(currentDateTime.empty() || currentDateTime.substr(0, 10) == "1970-01-01")
                        fileName = std::to_string(esp_random());
                    else 
                        fileName = currentDateTime;

                    esp_err_t err = mic.i2s_record_audio_aac(RECORD_DURATION, fileName.c_str());

                    // uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
                    // ESP_LOGI(RECORD_TAG, "Task stack size: %d", uxHighWaterMark);
                }
                else
                {
                    ESP_LOGE(RECORD_TAG, "Restart the ESP since SD was error");
                    esp_restart();
                }
            }
            xSemaphoreGive(sem_task);
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

extern "C" void app_main(void)
{
    esp_err_t ret;

    // Initialize the button and LED
    init_button(PIN_ONOFF_BT);
    led_init(PIN_LED);

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
        // ntp.getSyncStatus();
    }
#endif

#ifdef ENABLE_BLE_TESTING      
   // Initialize BLE and GATT server
    ble_state = ble_conn.ble_init();
    if(ble_state == BLE_STATE_ADVERTISING){
        ESP_LOGI(GATTS_TABLE_TAG, "BLE initialized successfully");
    }
    else{
        ESP_LOGE(GATTS_TABLE_TAG, "Failed to initialize BLE");
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);   // Delay to ensure BLE is properly initialized before starting the tasks
#endif

    // Create the semaphore
    sem_task = xSemaphoreCreateBinary();
    xSemaphoreGive(sem_task);

    /*  Record and Save Task  */
    xTaskCreatePinnedToCore(
        record_and_save_task,                   // Task code
        "Record Audio Task",                    // Task name
        14 * 1024,                              // Stack size
        NULL,                                   // Parameter to be passed
        TASK_FIRST_PRIORITY,                    // Task Priority
        NULL,                                   // Task Handle
        1                                       // 0: WiFi/BLE, 1: Apps
    );

#ifdef ENABLE_WIFI_TESTING
    if(wifi.wifi_status() == WIFI_STATE_CONNECTED)
    {
         xTaskCreatePinnedToCore(
            upload_file_task,                       // Task code
            "HTTP Test Task",                       // Task name
            8 * 1024,                               // Stack size
            NULL,                                   // Parameter to be passed
            TASK_SECOND_PRIORITY,                   // Task Priority
            NULL,                                   // Task Handle
            0                                       // 0: WiFi/BLE, 1: Apps
        );
    }
#endif

    ESP_LOGI(MAIN_TAG, "Initialization complete. Entering standby mode...");
    while (true)
    {
        bt_enum_t pwr_bt_state =  button_task(&bt_state, PIN_ONOFF_BT);
        if(pwr_bt_state == BT_PRESSED)
        {
            // Turn on when system is off
            if(sys_state == SYSTEM_OFF)
            {
                led_set_state(PIN_LED, LED_ON);
                sys_state = SYSTEM_ON;
                ESP_LOGI(MAIN_TAG, "System turned ON");
            }
            // Turn off when system is on
            else if(sys_state == SYSTEM_ON)
            {
                led_set_state(PIN_LED, LED_OFF);
                sys_state = SYSTEM_OFF;
                ESP_LOGI(MAIN_TAG, "System turned OFF");
                mic.i2s_force_stop_recording();
            }
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    
}
