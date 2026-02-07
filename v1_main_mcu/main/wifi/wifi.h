#pragma once

#include "main.h"
#include "esp_wifi.h"
#include "nvs_flash.h" 
#include "lwip/err.h"
#include "lwip/sys.h"

#ifdef __cplusplus
extern "C" {
#endif


/****************** Public Structs ******************/
typedef enum {
    WIFI_STATE_SUCCESS_INIT,
    WIFI_STATE_DISCONNECTED,
    WIFI_STATE_CONNECTED,

    // Init error states
    ERR_WIFI_FAIL_CONNECT,
    ERR_WIFI_FAIL_SET_MODE,
    ERR_WIFI_FAIL_SET_CONFIG,
    ERR_WIFI_FAIL_START,
    ERR_WIFI_UNEXPECTED_EVENT,

    // Event Handling states
    EVT_WIFI_CONNECTING,
    EVT_WIFI_DISCONNECTING,
    EVT_WIFI_STARTED,
    EVT_WIFI_STOPPED,
    EVT_WIFI_RETRYING,
    EVT_WIFI_GET_IP,

} wifi_state_t;

/****************** Public Define ******************/
#define WIFI_MAX_RETRY                20

class WIFI_Class{
    public:
        /**
        *   @brief  Class constructor for WIFI
        *   @param  None
        *   @return None
        */
        WIFI_Class();

        /**
        *   @brief  Class desstructor for WIFI
        *   @param  None
        *   @return None
        */
        ~WIFI_Class();

        /**
        *   @brief  Initialize WiFi Interface
        *   @param  None
        *   @return None
        */
        void init(void);

        /**
        *   @brief  Connect to WiFi
        *   @param  None
        *   @return None
        */
        void connect(void);

        /**
        *   @brief  Disconnect from WiFi
        *   @param  None
        *   @return None
        */
        void disconnect(void);

        /**
        *   @brief  Get WiFi Status
        *   @param  None
        *   @return 
        *           WiFi Status
        */
        wifi_state_t wifi_status(void);
};

// Public WiFi state variable
extern wifi_state_t wifi_state;

#ifdef __cplusplus
}
#endif