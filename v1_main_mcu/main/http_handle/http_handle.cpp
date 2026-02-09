#include "http_handle.h"
// Private Variables
static const char *HTTP_TAG = "HTTP_CLIENT";

HTTP_Class::HTTP_Class(){};
HTTP_Class::~HTTP_Class(){};

void HTTP_Class::init(const char* url)
{
    char output_buffer[MAX_HTTP_RESPONSE_BUFFER + 1] = {0};   // Buffer to store response of http request
    esp_http_client_config_t config = {
        .url = url,
    };
    
    esp_http_client_handle_t client = esp_http_client_init(&config);
    ESP_LOGI(HTTP_TAG, "Initialized HTTP Client connection to %s", url);

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(HTTP_TAG, "HTTP GET Status = %d, content_length = %d", esp_http_client_get_status_code(client), esp_http_client_get_content_length(client));
        int content_length  = esp_http_client_read(client, output_buffer, MAX_HTTP_RESPONSE_BUFFER);
        if (content_length >= 0) {
            output_buffer[content_length] = 0; // Null-terminate the buffer
            ESP_LOGI(HTTP_TAG, "HTTP GET Response: %s", output_buffer);
        } else {
            ESP_LOGE(HTTP_TAG, "Failed to read response");
        }
    } else {
        ESP_LOGE(HTTP_TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
    }
}


void HTTP_Class::send_get_request(const char* url)
{
    char output_buffer[MAX_HTTP_RESPONSE_BUFFER + 1] = {0};   // Buffer
    esp_http_client_config_t config = {
        .url = url,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_method(client, HTTP_METHOD_GET);
    ESP_LOGI(HTTP_TAG, "Initialized HTTP Client connection to %s", url);

    esp_err_t err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
        ESP_LOGE(HTTP_TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
    } else {
        int content_length = esp_http_client_fetch_headers(client);
        if (content_length < 0) {
            ESP_LOGE(HTTP_TAG, "HTTP client fetch headers failed");
        } else {
            int data_read = esp_http_client_read_response(client, output_buffer, MAX_HTTP_RESPONSE_BUFFER);
            if (data_read >= 0) {
                ESP_LOGI(HTTP_TAG, "HTTP GET Status = %d, content_length = %"PRId64,
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
                ESP_LOG_BUFFER_HEX(HTTP_TAG, output_buffer, data_read);
            } else {
                ESP_LOGE(HTTP_TAG, "Failed to read response");
            }
        }
    }
    esp_http_client_close(client);
}


void HTTP_Class::send_post_request(const char* url, const char* fileName)
{
    // Check file size
    size_t offset = 0;
    int bytes_read = 0;
    int total_bytes_read = 0;
    uint8_t file_chunk[MAX_SD_READ_SIZE] = {};
    int check_file_size = sd_card.check_file_size(fileName);
    ESP_LOGI(HTTP_TAG, "Preparing to upload file %s of size %d bytes", fileName, check_file_size);

    std::string _url = std::string(url) + "upload_file";
    esp_http_client_config_t config = {
        .url = _url.c_str(),
        .method = HTTP_METHOD_POST,
        .buffer_size = MAX_HTTP_RESPONSE_BUFFER,
        .buffer_size_tx = check_file_size,
    };
    char response_buffer[MAX_HTTP_RESPONSE_BUFFER + 1] = {0};
    esp_http_client_handle_t client = esp_http_client_init(&config);
    ESP_LOGI(HTTP_TAG, "Initialized HTTP Client connection to %s", _url.c_str());

    std::string content_disposition = "Content-Disposition: form-data; name=\"file\"; filename=\"" + std::string(fileName) + "\"";
    esp_http_client_set_header(client, "Content-Disposition", content_disposition.c_str());
    esp_http_client_set_header(client, "Content-Type", "application/octet-stream");
    esp_err_t err = esp_http_client_open(client, check_file_size);    
    if (err != ESP_OK) {
        ESP_LOGE(HTTP_TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
    } 
    else 
    {
        // Stream data
        ESP_LOGI(SD_TAG, "Reading file %s (%d bytes)", fileName, check_file_size);
        while(total_bytes_read < check_file_size )
        {
            bytes_read = sd_card.read_file(SD_TEST_PATH, file_chunk, offset, MAX_SD_READ_SIZE);
            ESP_LOGI(SD_TAG, "%s", file_chunk);

            int wlen = esp_http_client_write(client, (char*)file_chunk, bytes_read);
            if (wlen < 0) {
                ESP_LOGE(HTTP_TAG, "Write failed");
            }
            // Increment starting point to read next chunk
            offset += bytes_read;
            // Accumulate total bytes read
            total_bytes_read += bytes_read;
            memset(file_chunk, 0, sizeof(file_chunk)); // Clear the buffer
        }

        // Finish the request and read response        
        int content_length = esp_http_client_fetch_headers(client);
        if (content_length < 0) {
            ESP_LOGE(HTTP_TAG, "HTTP client fetch headers failed");
        } 
        else {
            int data_read = esp_http_client_read_response(client, response_buffer, MAX_HTTP_RESPONSE_BUFFER);
            if (data_read >= 0) {
                ESP_LOGI(HTTP_TAG, "HTTP POST Status = %d, content_length = %"PRId64,
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
                ESP_LOGI(HTTP_TAG, "%s", response_buffer);
                // ESP_LOG_BUFFER_CHAR(HTTP_TAG, response_buffer, strlen(response_buffer));
            } 
            else {
                ESP_LOGE(HTTP_TAG, "Failed to read response");
            }
        }
    }
    esp_http_client_cleanup(client);
}