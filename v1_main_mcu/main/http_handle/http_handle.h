#pragma once

#include "main.h"
#include "esp_http_client.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_HTTP_RESPONSE_BUFFER    2048

class HTTP_Class {
    public:
        /**
        *   @brief  Class constructor for HTTP
        *   @param  None
        *   @return None
        */
        HTTP_Class();

        /**
        *   @brief  Class destructor for Dropbox
        *   @param  None
        *   @return None
        */
        ~HTTP_Class();

        /**
         *  @brief  Initialize the HTTP client and test connection to the server
         *  @param  url The URL to send the GET request to
         *  @return None
         */
        void init(const char* url);

        /**
         *  @brief  Send a GET request to the specified URL
         *  @param  url The URL to send the GET request to
         *  @return None
         */
        void send_get_request(const char* url);

        /**
         * @brief  Send a POST request to the specified URL with the TXT file data as the body
         * @param  url The URL to send the POST request to
         * @param  fileName The name of the file being uploaded (for logging purposes)
         * @return None
         */
        void send_post_request(const char* url, const char* fileName);
};


#ifdef __cplusplus
}
#endif