#include "http_handle.h"
#include "esp_tls.h"
#include "esp_crt_bundle.h"

// Private Variables
static const char *HTTP_TAG = "HTTP_CLIENT";

HTTP_Class::HTTP_Class(){};
HTTP_Class::~HTTP_Class(){};

void HTTP_Class::init(const char* url)
{
    // char output_buffer[MAX_HTTP_RESPONSE_BUFFER + 1] = {0};   // Buffer to store response of http request
    char* output_buffer = (char*)malloc(MAX_HTTP_RESPONSE_BUFFER + 1);
    esp_http_client_config_t config = {
        .url = url,
        .timeout_ms = 5000,
        // .crt_bundle_attach = esp_crt_bundle_attach,
        
    };
    
    esp_http_client_handle_t client = esp_http_client_init(&config);
    ESP_LOGI(HTTP_TAG, "Initialized HTTP Client connection to %s", url);

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        int status = esp_http_client_get_status_code(client);
        int64_t len = esp_http_client_get_content_length(client);
        ESP_LOGI(HTTP_TAG, "HTTP Status = %d, length = %lld", status, len);

        // Since perform() already fetched data into internal buffers:
        int data_read = esp_http_client_read_response(client, output_buffer, MAX_HTTP_RESPONSE_BUFFER);
        if (data_read >= 0) {
            output_buffer[data_read] = 0;
            ESP_LOGI(HTTP_TAG, "Response: %s", output_buffer);
        }
    } else {
        ESP_LOGE(HTTP_TAG, "HTTP request failed: %s", esp_err_to_name(err));
    }
    esp_http_client_cleanup(client);
    free(output_buffer);
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
    esp_http_client_cleanup(client);
}


esp_err_t HTTP_Class::send_post_request(const char* url, const char* fileName)
{
    size_t offset = 0;
    int bytes_read = 0;
    int total_bytes_read = 0;
    uint8_t file_chunk[MAX_SD_READ_SIZE] = {};

    // SD Card file path
    std::string sdPath = "/sdcard/";
    sdPath += fileName;

    uint32_t file_size = sd_card.check_file_size(sdPath.c_str());
    ESP_LOGI(HTTP_TAG, "Preparing to upload file %s of size %d bytes", sdPath.c_str(), file_size);

    if(file_size <= 0)
    {
        if(sd_card.remove_file(sdPath.c_str()) != ESP_OK)
        {
            return ESP_FAIL;
        }
    }
    
    // Define boundary for multipart
    const char* boundary = "----ESP32Boundary1234";
    std::string body_start =
        "--" + std::string(boundary) + "\r\n"
        "Content-Disposition: form-data; name=\"file\"; filename=\"" + std::string(fileName) + "\"\r\n"
        "Content-Type: application/octet-stream\r\n\r\n";

    std::string body_end = "\r\n--" + std::string(boundary) + "--\r\n";
    int total_length = body_start.length() + file_size + body_end.length();

    // HTTP client config
    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 60000,
        .buffer_size = MAX_HTTP_RESPONSE_BUFFER,
        .buffer_size_tx = MAX_SD_READ_SIZE
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    ESP_LOGI(HTTP_TAG, "Initialized HTTP Client for %s", url);

    // Set multipart header
    std::string content_type = "multipart/form-data; boundary=" + std::string(boundary);
    esp_http_client_set_header(client, "Content-Type", content_type.c_str());

    // Open connection with the total length
    esp_err_t err = esp_http_client_open(client, total_length);
    if (err != ESP_OK) {
        ESP_LOGE(HTTP_TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return err;
    }

    // Send multipart body start
    esp_http_client_write(client, body_start.c_str(), body_start.length());

    // Send file in chunks
    while (total_bytes_read < file_size) {
        bytes_read = sd_card.read_file(sdPath.c_str(), file_chunk, offset, MAX_SD_READ_SIZE);
        if (bytes_read <= 0) break;

        int wlen = esp_http_client_write(client, (char*)file_chunk, bytes_read);
        if (wlen < 0) {
            ESP_LOGE(HTTP_TAG, "Failed to write file chunk");
        }

        offset += bytes_read;
        total_bytes_read += bytes_read;
    }

    // Send multipart ending
    esp_http_client_write(client, body_end.c_str(), body_end.length());

    // Read server response
    char response_buffer[MAX_HTTP_RESPONSE_BUFFER] = {0};
    int content_length = esp_http_client_fetch_headers(client);
    if (content_length >= 0) {
        int data_read = esp_http_client_read_response(client, response_buffer, MAX_HTTP_RESPONSE_BUFFER);
        if (data_read >= 0) {
            ESP_LOGI(HTTP_TAG, "Server response: %s", response_buffer);
        } else {
            ESP_LOGE(HTTP_TAG, "Failed to read server response");
        }
    } else {
        ESP_LOGE(HTTP_TAG, "Failed to fetch headers from server");
    }

    esp_http_client_cleanup(client);
    return ESP_OK;
}

esp_err_t HTTP_Class::uploadAACFile(const char* url, const char* fileName)
{
    esp_err_t err = ESP_OK;

    // SD Card file path
    std::string sdPath;
    sdPath.clear();
    sdPath.append("/sdcard/"); 
    sdPath.append(fileName);

    // SD Card things
    int fd;
    uint8_t *in_buf     = (uint8_t *)malloc(AAC_CODEC_BUFFER_SIZE);
    uint32_t read_size  = 0;
    uint32_t file_size  = 0;
    struct stat info;
    
    // Check file info
    if (stat(sdPath.c_str(), &info) < 0) {
        ESP_LOGE(HTTP_TAG, "Failed to stat file: %s", strerror(errno));
        return ESP_FAIL;
    }
    file_size = (uint32_t) info.st_size;
    ESP_LOGI(HTTP_TAG, "File %s (%ld) bytes", sdPath.c_str(), file_size);
    
    // Remove the file since it might be corrupt
    if(file_size <= 0)
    {
        if(sd_card.remove_file(sdPath.c_str()) != ESP_OK)
        {
            return ESP_FAIL;
        }
    }
    // File is not corrupt
    else 
    {
        // Open File
        fd = open(sdPath.c_str(), O_RDONLY, 0);
        if (fd < 0) {
            ESP_LOGE(HTTP_TAG, "File %s not found", sdPath.c_str());
            return ESP_FAIL;
        }
        ESP_LOGI(HTTP_TAG, "Uploading %s to server", sdPath.c_str());

        char response_buffer[MAX_HTTP_RESPONSE_BUFFER + 1] = {0};
        int content_length = 0;
        esp_http_client_config_t config = {
            .url = url,
            // .port = 5000,
            .method = HTTP_METHOD_POST,
            .timeout_ms = (RECORD_DURATION + 200) * 1000,
            .disable_auto_redirect = true,
            .buffer_size = MAX_HTTP_RESPONSE_BUFFER,
            .buffer_size_tx = AAC_CODEC_BUFFER_SIZE,
            // .crt_bundle_attach = esp_crt_bundle_attach,
            // .transport_type = HTTP_TRANSPORT_OVER_SSL,  // Required for HTTPS
            .keep_alive_enable = true,
            .keep_alive_interval = 20
        };
        esp_http_client_handle_t client = esp_http_client_init(&config);

        // Set HTTP header
        // Define boundary for multipart
        const char* boundary = "----ESP32Boundary1234";
        std::string body_start =
            "--" + std::string(boundary) + "\r\n"
            "Content-Disposition: form-data; name=\"file\"; filename=\"" + std::string(fileName) + "\"\r\n"
            "Content-Type: application/octet-stream\r\n\r\n";

        std::string body_end = "\r\n--" + std::string(boundary) + "--\r\n";
        int total_length = body_start.length() + file_size + body_end.length();
        
        // Set multipart header
        std::string content_type = "multipart/form-data; boundary=" + std::string(boundary);
        esp_http_client_set_header(client, "Content-Type", content_type.c_str());
        err = esp_http_client_open(client, total_length);
        esp_http_client_write(client, body_start.c_str(), body_start.length());
        if (err != ESP_OK) {
            ESP_LOGE(HTTP_TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
            return ESP_FAIL;
        } 
        else {
            size_t chunkSize;
            do {
                // Read file into buffer
                chunkSize = read(fd, in_buf, AAC_CODEC_BUFFER_SIZE);
                // ESP_LOGI(HTTP_TAG, "Data len %d", chunkSize);

                /*  Stream data */
                if(chunkSize > 0)
                {
                    if (esp_http_client_write(client, (char*)in_buf, chunkSize) < 0) 
                    {
                        ESP_LOGE(HTTP_TAG, "HTTP Write failed"); 
                        close(fd);
                        free(in_buf);
                        sdPath.clear();
                        esp_http_client_cleanup(client);
                        return ESP_FAIL;
                    }
                    read_size += chunkSize;
                    ESP_LOGI(HTTP_TAG, "Transmitted data: %ld bytes", read_size);
                }            
            } while(read_size < file_size);
            close(fd);
            free(in_buf);
            sdPath.clear();
            
            // Write the ending boundary
            esp_http_client_write(client, body_end.c_str(), body_end.length());

            // Read server response
            content_length = esp_http_client_fetch_headers(client);
            if (content_length < 0) {
                ESP_LOGE(HTTP_TAG, "HTTP client fetch headers failed");
                esp_http_client_cleanup(client);
                return ESP_FAIL;
            } 
            else 
            {
                int data_read = esp_http_client_read_response(client, response_buffer, MAX_HTTP_RESPONSE_BUFFER);
                if (data_read >= 0) 
                {
                    int retCode = esp_http_client_get_status_code(client); 
                    ESP_LOGI(HTTP_TAG, "HTTP POST Status = %d, content_length = %"PRId64"", retCode, esp_http_client_get_content_length(client));
                    ESP_LOGI(HTTP_TAG, "%s", response_buffer);
                    // ESP_LOG_BUFFER_CHAR(HTTP_TAG, response_buffer, strlen(response_buffer));
                    if(retCode != HttpStatus_Ok)
                    {
                        ESP_LOGE(HTTP_TAG, "Failed to upload with code %d", retCode);
                        esp_http_client_cleanup(client);
                        return ESP_FAIL;
                    }
                } else {
                    ESP_LOGE(HTTP_TAG, "Failed to read response");
                }
            }
        }
        esp_http_client_cleanup(client);
    }
    return ESP_OK;
}