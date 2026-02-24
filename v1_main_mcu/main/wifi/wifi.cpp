#include "wifi.h"

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static EventGroupHandle_t s_wifi_event_group;   /* FreeRTOS event group to signal when we are connected*/
int wifi_retry_num = 0;
wifi_state_t wifi_state = WIFI_STATE_DISCONNECTED;
char gateway_ip[16]; // Assuming IPv4 address string representation (e.g., "192.168.1.100")
esp_event_handler_instance_t instance_any_id;
esp_event_handler_instance_t instance_got_ip;

/**
*   @brief  Connection Event Handler
*   @param  arg             Argument pointers
*   @param  event_base      WiFi Event base
*   @param  event_id        WiFi Connection Status ID
*   @param  event_data      Device IP Data
*   @return None
*/
static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        wifi_state = EVT_WIFI_STARTED;
        ESP_LOGI(WIFI_TAG, "WiFi started, attempting to connect...");
    } 
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_STOP) {
        wifi_state = EVT_WIFI_STOPPED;
        ESP_LOGI(WIFI_TAG, "WiFi has disconnected and stopped");
    } 
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_state = EVT_WIFI_RETRYING;
        if (wifi_retry_num < WIFI_MAX_RETRY) {
            esp_wifi_connect();
            wifi_retry_num++;
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(WIFI_TAG, "Connect to the AP fail, retrying");
    } 
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(WIFI_TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        sprintf(gateway_ip, IPSTR, IP2STR(&event->ip_info.gw));
        ESP_LOGI(WIFI_TAG, "Gateway IP: %s", gateway_ip);
        wifi_state = EVT_WIFI_GET_IP;
        wifi_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

/***********************************************************/
/********************** WiFi Class **********************/
WIFI_Class::WIFI_Class(){};
WIFI_Class::~WIFI_Class(){};

void WIFI_Class::init(void)
{
    esp_err_t ret;
    s_wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());                                                
    ESP_ERROR_CHECK(esp_event_loop_create_default());        
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));


    /*  Initialize WiFi Peripheral  */
    ESP_ERROR_CHECK(esp_netif_set_hostname(esp_netif_create_default_wifi_sta(), SAMPLE_DEVICE_NAME));     
    esp_wifi_set_ps(WIFI_PS_NONE);
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();                 
    ret = esp_wifi_init(&cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(WIFI_TAG, "WiFi init failed: %s", esp_err_to_name(ret));
        return;
    }

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
        }
    };
    ret = esp_wifi_set_mode(WIFI_MODE_STA);
    if (ret != ESP_OK) {
        wifi_state = ERR_WIFI_FAIL_SET_MODE;
        ESP_LOGE(WIFI_TAG, "WiFi set mode failed: %s", esp_err_to_name(ret));
        return;
    }
    ret = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    if (ret != ESP_OK) {
        wifi_state = ERR_WIFI_FAIL_SET_CONFIG;
        ESP_LOGE(WIFI_TAG, "WiFi set config failed: %s", esp_err_to_name(ret));
        return;
    }
}

void WIFI_Class::connect(void)
{
    esp_err_t ret;
    ret = esp_wifi_start();
    if (ret != ESP_OK) {
        wifi_state = ERR_WIFI_FAIL_START;
        ESP_LOGE(WIFI_TAG, "WiFi start failed: %s", esp_err_to_name(ret));
        return;
    }           

    wifi_state = EVT_WIFI_CONNECTING;
    ret = esp_wifi_connect();
    if (ret != ESP_OK) {
        wifi_state = ERR_WIFI_FAIL_CONNECT;
        ESP_LOGE(WIFI_TAG, "WiFi connect failed: %s", esp_err_to_name(ret));
    }

    // FreeRTOS event group to signal when we are connected
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);     

    // xEventGroupWaitBits() returns the bits before the call returned
    if (bits & WIFI_CONNECTED_BIT) {
        wifi_state = WIFI_STATE_CONNECTED;
        ESP_LOGI(WIFI_TAG, "Connected to ap SSID:%s password:%s",WIFI_SSID, WIFI_PASSWORD);
    } 
    else if (bits & WIFI_FAIL_BIT) {
        wifi_state = WIFI_STATE_DISCONNECTED;
        ESP_LOGI(WIFI_TAG,"Failed to connect to SSID:%s, password:%s\n", WIFI_SSID, WIFI_PASSWORD);
    } else {
        wifi_state = ERR_WIFI_UNEXPECTED_EVENT;
        ESP_LOGE(WIFI_TAG,"UNEXPECTED EVENT\n");
    }
}

void WIFI_Class::disconnect(void)
{
    wifi_state = EVT_WIFI_DISCONNECTING;
    esp_err_t ret = esp_wifi_disconnect();
    if (ret != ESP_OK) {
        ESP_LOGE(WIFI_TAG, "WiFi disconnect failed: %s", esp_err_to_name(ret));
    }

    ret = esp_wifi_stop();
    if (ret != ESP_OK) {
        ESP_LOGE(WIFI_TAG, "WiFi stop failed: %s", esp_err_to_name(ret));
    }
}

wifi_state_t WIFI_Class::wifi_status(void)
{
    ESP_LOGI(WIFI_TAG, "WiFi Connection Status is %d", wifi_state);
    return wifi_state;
}