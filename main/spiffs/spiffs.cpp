#include "esp_err.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "spiffs.h"

const static char* SPIFFS_TAG = "SPIFFS";

void SPIFFS::init(void) {
    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 2,
      .format_if_mount_failed = true
    };

    // Initialize SPIFFS
    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(SPIFFS_TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(SPIFFS_TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(SPIFFS_TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(SPIFFS_TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(SPIFFS_TAG, "Partition size: total: %d, used: %d", total, used);
    }

    // Check consistency of reported partition size info.
    if (used > total) {
        ESP_LOGW(SPIFFS_TAG, "Number of used bytes cannot be larger than total. Performing SPIFFS_check().");
        ret = esp_spiffs_check(conf.partition_label);
        // Could be also used to mend broken files, to clean unreferenced pages, etc.
        // More info at https://github.com/pellepl/spiffs/wiki/FAQ#powerlosses-contd-when-should-i-run-spiffs_check
        if (ret != ESP_OK) {
            ESP_LOGE(SPIFFS_TAG, "SPIFFS_check() failed (%s)", esp_err_to_name(ret));
            return;
        } else {
            ESP_LOGI(SPIFFS_TAG, "SPIFFS_check() successful");
        }
    }
}

void SPIFFS::read_spiffs(std::string path) {
    std::string full_path = "/spiffs/" + path;
    FILE* f = fopen(full_path.c_str(), "r");
    if (f == NULL) {
        ESP_LOGE(SPIFFS_TAG, "Failed to open file for reading");
        return;
    }
    char line[64];
    fgets(line, sizeof(line), f);
    fclose(f);
    ESP_LOGI(SPIFFS_TAG, "Read from file: '%s'", line);
}

void SPIFFS::write_spiffs(std::string path, const char* data) {
    std::string full_path = "/spiffs/" + path;
    FILE* f = fopen(full_path.c_str(), "w");
    if (f == NULL) {
        ESP_LOGE(SPIFFS_TAG, "Failed to open file for writing");
        return;
    }
    fprintf(f, "%s", data);
    fclose(f);
    ESP_LOGI(SPIFFS_TAG, "File written");
}

void SPIFFS::unmount_spiffs(void) {
    esp_vfs_spiffs_unregister(NULL);
    ESP_LOGI(SPIFFS_TAG, "SPIFFS unmounted");
}