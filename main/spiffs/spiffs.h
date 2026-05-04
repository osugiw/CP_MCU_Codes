#pragma once

#include "main.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Init SPIFFS (SPI Flash File System) functions
 */
class SPIFFS {
    public:
        /**
         * Init SPIFFS
         * @return Init status
         */
        esp_err_t init(void);

        /**
         * Read data from SPIFFS file
         * @param path: Path to the file in SPIFFS
         * @param  filePath    File filePath
         * @param  dt          Data read from the file        
         * @param  offset      Offset to start reading from
         * @param  chunk_size  Size of data to read
         * @return File return code
         */
        esp_err_t read_spiffs(std::string path, uint8_t* dt, size_t offset, size_t chunk_size);

        /**
         * Write data to SPIFFS file
         * @param path: Path to the file in SPIFFS
         * @param data: Data to be written to the file
         * @return File status
         */
        esp_err_t write_spiffs(std::string path, const char* data);

        /**
         * Unmount SPIFFS
         */
        void unmount_spiffs(void);
};

#ifdef __cplusplus
}
#endif