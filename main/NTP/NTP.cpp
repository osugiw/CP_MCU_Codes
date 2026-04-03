#include "NTP.h"

static const char *TAG = "NTP_Server";
bool isTimeSynchronized = false;

/***********************************************************/
/********************** NTP Class **********************/
NTP::NTP(){};
NTP::~NTP(){};

void time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI(TAG, "NTP time is successfully synchronized");
    isTimeSynchronized = true;
}

void NTP::initialize(void)
{
    time(&now);
    localtime_r(&now, &timeinfo);
    if (timeinfo.tm_year < (2016 - 1900)) 
    {
        ESP_LOGI(TAG, "Time is not set yet. Getting time over NTP.");
        /**
         * Acquire NTP server address via DHCP,
         * NOTE: This call should be made BEFORE esp acquires IP address from DHCP,
         * otherwise NTP option would be rejected by default.
         */
        ESP_LOGI(TAG, "Initializing SNTP");
        esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG_MULTIPLE(2, ESP_SNTP_SERVER_LIST(NTP_SERVER1, NTP_SERVER2));
        config.start = false;                       // start SNTP service explicitly (after connecting)
        config.server_from_dhcp = true;             // accept NTP offers from DHCP server, if any (need to enable *before* connecting)
        config.renew_servers_after_new_IP = true;   // let esp-netif update configured SNTP server(s) after receiving DHCP lease
        config.index_of_first_server = 1;           // updates from server num 1, leaving server 0 (from DHCP) intact
        config.ip_event_to_renew = IP_EVENT_STA_GOT_IP;
        config.sync_cb = time_sync_notification_cb; // only if we need the notification function
        esp_netif_sntp_init(&config);

        ESP_LOGI(TAG, "Starting SNTP");
        esp_netif_sntp_start();

        // wait for time to be set
        now = 0;
        timeinfo = { 0 };
        retry = 0;
        
        while (esp_netif_sntp_sync_wait(2000 / portTICK_PERIOD_MS) == ESP_ERR_TIMEOUT && ++retry < retry_count) {
            ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        }
        
        localtime_r(&now, &timeinfo);
        time(&now); // update 'now' variable with current time

        // esp_netif_sntp_deinit();
    }

    // Set timezone to New Zealand Standard Time
    setenv("TZ", "CST6CDT,M3.2.0,M11.1.0", 1);
    tzset();
}

char* NTP::currentDateTime(void)
{
    time(&now);
    tm * obtain_time = localtime_r(&now, &timeinfo);
    if(obtain_time == NULL)
    {
        ESP_LOGE(TAG, "Failed to obtain time");
        return NULL;
    }
    strftime(strftime_buf, sizeof(strftime_buf), "%Y-%m-%d_%H%M%S", &timeinfo);
    // ESP_LOGI(TAG, "The current date/time in Chicago is: %s", strftime_buf);
    return strftime_buf;
}

void NTP::forceSync(void)
{
    while (esp_netif_sntp_sync_wait(2000 / portTICK_PERIOD_MS) == ESP_ERR_TIMEOUT && ++retry < retry_count) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
    }
}

bool NTP::getSyncStatus(void)
{
    sntp_sync_status_t status = sntp_get_sync_status();
    
    if (status == SNTP_SYNC_STATUS_COMPLETED) {
        ESP_LOGI("SNTP", "Time is synchronized!");
        isTimeSynchronized = true;
        
        // Now you can safely get the current time
        time_t now;
        struct tm timeinfo;
        time(&now);
        localtime_r(&now, &timeinfo);
        ESP_LOGI("SNTP", "Current time: %02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    } else {
        ESP_LOGW("SNTP", "Time not synchronized yet...");
        isTimeSynchronized = false;
    }
    return isTimeSynchronized;
}