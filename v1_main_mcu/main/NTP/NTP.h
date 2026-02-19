#pragma once

#include "main.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_netif_sntp.h"
#include "lwip/ip_addr.h"
#include "esp_sntp.h" 
#include "lwip/err.h"
#include "lwip/sys.h"

#ifdef __cplusplus
extern "C" {
#endif

/*  NTP Config  */
#define MAX_SYNC_RETRY      20
#define NTP_SERVER1         "0.us.pool.ntp.org"
#define NTP_SERVER2         "pool.ntp.org"
#define CHICAGO_TZ          "CST6CDT,M3.2.0,M11.1.0"   // Check the following csv to see your timezone https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv


class NTP{
    private:
        time_t now;
        struct tm timeinfo;
        char strftime_buf[64];
        const int retry_count = MAX_SYNC_RETRY;
        int retry = 0;
    
    public:
        /**
        *   @brief  Class constructor for NTP
        *   @param  None
        *   @return None
        */
        NTP();

        /**
        *   @brief  Class desstructor for NTP
        *   @param  None
        *   @return None
        */
        ~NTP();

        /**
        *   @brief  Initialize SNTP
        *   @param  None
        *   @return None
        */
        void initialize(void);

        /**
        *   @brief  Get Current Time
        *   @param  None
        *   @return 
        *           Current Time (YYYY-MM-DD_hhmmss)
        */
        char* currentDateTime(void);

        /**
        *   @brief  Force NTP Sync
        *   @param  None
        *   @return None
        */
        void forceSync(void);

        /**
        *   @brief  Get NTP Synchronization Status
        *   @param  None
        *   @return 
        *           Synchronization Status
        */
        bool getSyncStatus(void);
};

    

#ifdef __cplusplus
}
#endif