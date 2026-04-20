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
         */
        void init(void);

        /**
         * Read data from SPIFFS file
         * @param path: Path to the file in SPIFFS
         */
        void read_spiffs(std::string path);

        /**
         * Write data to SPIFFS file
         * @param path: Path to the file in SPIFFS
         * @param data: Data to be written to the file
         */
        void write_spiffs(std::string path, const char* data);

        /**
         * Unmount SPIFFS
         */
        void unmount_spiffs(void);
};

#ifdef __cplusplus
}
#endif