#pragma once

#include "v1_main_mcu.h"
#include "esp_vfs_fat.h"
#include "esp_flash.h"
#include "sdmmc_cmd.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MIN_SPACE_LEFT      100          // In MB
#define NUM_FILES_REMOVE    5

/*  SD Class    */
class sd_card_class{
    private:
        uint32_t SDCard_Size;
        uint32_t Flash_Size;
        sdmmc_host_t host = SDSPI_HOST_DEFAULT();
        sdmmc_card_t *card;
        std::vector<std::string> listDir;
    
    public:
        /**
        *   @brief  Class constructor for SD Card
        *   @param  None
        *   @return None
        */
        sd_card_class();

        /**
        *   @brief  Class desstructor for SD Card
        *   @param  None
        *   @return None
        */
        ~sd_card_class();

        /**
        *   @brief  Initialize SD Peripheral
        *   @param  None
        *   @return None
        */
        esp_err_t initialize(void);

        /**
        *   @brief  Uinnitialize SD Peripheral
        *   @param  None
        *   @return None
        */
        esp_err_t uninitialize(void);

        
        /**
        *   @brief  Check file size
        *   @param  filePath    File path to check size
        *   @return 
        *           File size in bytes
        */
        uint32_t check_file_size(const char *filePath);

        /**
        *   @brief  Deletes old files if the remaining space size is low
        *   @param  rec_time    Most Audio files recording time
        *   @return 
        *           Remaining free space in bytes
        */
        uint32_t check_free_space(uint32_t rec_time);
        
        /**
        *   @brief  Write a file to the SD Card
        *   @param  filePath    File filePath
        *   @param  data    Data to write
        *   @return 
        *           Write status code
        */
        esp_err_t write_file(const char *filePath, char *data);
        
        /**
        *   @brief  Read a file from the SD Card
        *   @param  filePath    File filePath
        *   @param  dt          Data read from the file        
        *   @param  offset      Offset to start reading from
        *   @param  chunk_size  Size of data to read
        *   @return 
        *           Number of bytes read, or -1 on error
        */
        int read_file(const char *filePath, uint8_t* dt, size_t offset, size_t chunk_size);

        /**
        *   @brief  Remove a file from the SD Card
        *   @param  fileName    File filePath to be removed
        *   @return 
        *           Removal status code
        */
        esp_err_t remove_file(const char *fileName);

        /**
        *   @brief  List files on a directory
        *   @param  filePath    Directory Path
        *   @return 
        *           Array of Filenames
        */
        std::vector<std::string> list_directory(const char *filePath);

         /**
        *   @brief  Print files inside a directory
        *   @param  listFile    Vector of files inside a directory
        *   @return 
        *           Array of Filenames
        */
        void print_directory(std::vector<std::string> &listFile);

        
        /**
        *   @brief  Generate dummy data for testing
        *   @param  Path    Path to generate the test file
        *   @return 
        *           None
        */
        void generate_5kb_test_file(const char* path);
};


#ifdef __cplusplus
}
#endif