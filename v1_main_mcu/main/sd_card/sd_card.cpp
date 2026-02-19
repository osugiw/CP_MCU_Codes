#include "sd_card.h"

/* Private variables*/
sd_card_class sd_card;
esp_err_t sd_mounted = ESP_OK;

/***********************************************************/
/********************** SD Card Class **********************/
sd_card_class::sd_card_class(){};
sd_card_class::~sd_card_class(){};

esp_err_t sd_card_class::write_file(const char *filePath, char *data)
{
    ESP_LOGI(SD_TAG, "Opening file %s", filePath);
    int fd = open(filePath, O_RDWR | O_CREAT | O_TRUNC, 0);
    if (fd < 0) {
        ESP_LOGE(SD_TAG, "Failed to open file for writing");
        return ESP_FAIL;
    }
    write(fd, data, strlen(data));
    close(fd);
    ESP_LOGI(SD_TAG, "File written");

    return ESP_OK;
}

int sd_card_class::read_file(const char *filePath, uint8_t* dt, size_t offset, size_t chunk_size)
{
    FILE *f = fopen(filePath, "rb");
    if (f == NULL) {
        ESP_LOGE(SD_TAG, "Failed to open file for reading");
        return -1;
    }

    // Move the file pointer to the start of the next chunk
    fseek(f, offset, SEEK_SET);

    // fread returns the number of items successfully read
    size_t bytes_read = fread(dt, 1, chunk_size, f);
    fclose(f);
    return (int)bytes_read;
}

esp_err_t sd_card_class::remove_file(const char *fileName)
{
    esp_err_t err = ESP_OK;
    struct stat st;
    std::string filePath = "/sdcard/";
    filePath.append(fileName);

    // First check if file exists
    if (stat(filePath.c_str(), &st) == 0) {
        // Delete it if it exists
        ESP_LOGI(SD_TAG, "Removing %s", filePath.c_str());
        err = unlink(filePath.c_str()); 
        if(err != 0){
            ESP_LOGE(SD_TAG, "Failed to remove %s from the SD Card", filePath.c_str());
            return ESP_FAIL;
        }
        ESP_LOGI(SD_TAG, "Successfully removed %s from the SD Card", filePath.c_str());
    }
    return ESP_OK;
}

void sd_card_class::print_directory(std::vector<std::string> &listFile)
{
    printf("Filenames: \n");
    for(uint8_t i=0; i<listFile.size(); i++){
        printf("    %s\n", listFile[i].c_str());
    }
}

std::vector<std::string> sd_card_class::list_directory(const char *filePath)
{
    listDir.clear();
    // ESP_LOGI(RECORDING_TAG, "Listing files in %s:", filePath);

    DIR *dir = opendir(filePath);
    if (!dir) {
        printf("Failed to open directory: %s", strerror(errno));
    }
    else{
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            // printf("%d.    %s: %s\n", entry->d_ino, (entry->d_type == DT_DIR) ? "directory" : "file     ", entry->d_name);
            
            if(entry->d_type != DT_DIR)
                listDir.push_back(entry->d_name);
        }
        closedir(dir);
    }

    // printf("TEST FILENAME %s\n", listDir->c_str());
    return listDir;
}

esp_err_t sd_card_class::initialize(void)
{
    esp_err_t ret;
    
    /* If format_if_mount_failed is set to true, SD card will be partitioned and formatted in case when mounting fails  */
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = true,          
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };
    
    const char mount_point[] = "/sdcard";
    ESP_LOGI(SD_TAG, "Initializing SD card");
    ESP_LOGI(SD_TAG, "Using SPI peripheral");
    host = SDSPI_HOST_DEFAULT();   // The default frequency is 20 MHz, use "host.max_freq_khz = 10000;" to change

    /*  Initialize SPI Bus*/
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };
    ret = spi_bus_initialize(SPI2_HOST, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (ret != ESP_OK) {
        ESP_LOGE(SD_TAG, "Failed to initialize SPI bus.");
        return ESP_FAIL;
    }

    /*  This initializes the slot without card detect (CD) and write protect (WP) signals. 
        Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals. */
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_NUM_CS;
    slot_config.host_id = SPI2_HOST;

    ESP_LOGI(SD_TAG, "Mounting filesystem");
    ret = esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config, &mount_config, &card);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(SD_TAG, "Failed to mount filesystem. "
                     "If you want the card to be formatted, set the CONFIG_EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
        } 
        else {
            ESP_LOGE(SD_TAG, "Failed to initialize the card (%s). "
                     "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
        }
        return ESP_FAIL;
    }
    ESP_LOGI(SD_TAG, "Filesystem mounted");

    // Card has been initialized, print its properties
    sdmmc_card_print_info(stdout, card);
    SDCard_Size = ((uint64_t) card->csd.capacity) * card->csd.sector_size / (1024 * 1024);
    // Size: %lluMB\n", ((uint64_t) card->csd.capacity) * card->csd.sector_size / (1024 * 1024)
    return ESP_OK;
}

esp_err_t sd_card_class::uninitialize(void)
{
    esp_err_t err = ESP_OK;
    // All done, unmount partition and disable SPI peripheral
    err = esp_vfs_fat_sdcard_unmount("/sdcard", card);
    ESP_LOGI(SD_TAG, "Card unmounted");
    // Deinitialize the bus after all devices are removed
    err = spi_bus_free(SPI2_HOST);
    return err;
}

uint32_t sd_card_class::check_file_size(const char *filePath)
{
    struct stat info;
    if (stat(filePath, &info) < 0) {
        ESP_LOGE(SD_TAG, "Failed to stat file: %s", strerror(errno));
        return 0;
    }
    return (uint32_t) info.st_size;
}

uint32_t sd_card_class::check_free_space(uint32_t rec_time)
{
    esp_err_t err = ESP_OK;
    std::vector<std::string> listFile = list_directory("/sdcard");
    uint32_t num_files = listFile.size();
    std::string filePath;
    uint32_t totalFileSize = 0;
    uint32_t free_space = 0;
    struct stat info;

    if(!listFile.empty())
    {
        for(uint32_t _file=0; _file < num_files; _file++){
            filePath.clear();
            filePath.append("/sdcard/");
            filePath.append(listFile[_file].c_str());
            if (stat(filePath.c_str(), &info) < 0) {
                ESP_LOGE(SD_TAG, "Failed to stat file: %s", strerror(errno));
                break;
            }
            totalFileSize += (uint32_t) info.st_size;
        }
        totalFileSize = (totalFileSize / 8) / 1000000;
        // ESP_LOGI(SD_TAG, "Total %ld MB", ((totalFileSize / 8) / 1000000));
    }
    
    free_space = SDCard_Size - totalFileSize;
    ESP_LOGI(SD_TAG, "SD Size %ld MB, Used Size is %ld MB, Free Space %ld MB", SDCard_Size, totalFileSize, free_space);

    // Free up some space by removing the oldest file
    if(free_space <= MIN_SPACE_LEFT)
    {
        for(uint8_t i=0; i < NUM_FILES_REMOVE; i++){
            filePath.clear();
            filePath.append("/sdcard/");
            filePath.append(listFile[i].c_str());
            err = remove_file(filePath.c_str());  
        }
    }
    return     free_space = SDCard_Size - totalFileSize;
;
}


void sd_card_class::generate_5kb_test_file(const char* path) {
    FILE* f = fopen(path, "wb");
    if (f == NULL) {
        ESP_LOGE("SD_TEST", "Failed to create file");
        return;
    }

    // 10 blocks of 512 bytes = 5120 bytes (5 KB)
    for (int block = 1; block <= 10; block++) {
        // Write a header (approx 12 bytes)
        int header_len = fprintf(f, "--- START BLOCK %02d ---\n", block);
        
        // Fill the rest of the 512-byte sector with repeating characters
        // We subtract the header length and the footer length
        for (int i = 0; i < (512 - header_len - 15); i++) {
            fputc('.', f); 
        }
        
        fprintf(f, "\n--- END BLOCK %02d ---\n", block);
    }

    fclose(f);
    ESP_LOGI("SD_TEST", "5KB Test File Created: %s", path);
}