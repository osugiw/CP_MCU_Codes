/*
 * wearable_ai.h
 *
 */

#ifndef SCENARIO_APP_WEARABLE_AI_H_
#define SCENARIO_APP_WEARABLE_AI_H_

#include <stdio.h>
#include <stdlib.h>
#include "WE2_device.h"
#include "WE2_core.h"
#include "board.h"
#include "ff.h"
// #include <string.h>
// #include <vector>

#define DRV         "/sdcard"
#define FILE_TEST   "new_groove.txt"
#define FIVEKB_TEST "test.txt"
#define TESTSTR     "This is groove"

/**
*   @brief  Initialize SD Peripheral
*   @param  None
*   @return Init status
*/
FRESULT sd_init(void);

/**
*   @brief  Uinnitialize SD Peripheral
*   @param  None
*   @return None
*/
void sd_uninitialize(void);

/**
*   @brief  Write a file to the SD Card
*   @param  filePath    File filePath
*   @param  data    Data to write
*   @return 
*           Write status code
*/
FRESULT sd_write_file(const char *filePath, uint8_t *dt);

/**
*   @brief  Read a file from the SD Card
*   @param  filePath    File filePath
*   @param  dt          Data read from the file        
*   @param  offset      Offset to start reading from
*   @param  chunk_size  Size of data to read
*   @return 
*           Number of bytes read, or -1 on error
*/
uint32_t sd_read_file(const char *filePath, uint8_t *dt, size_t offset, size_t chunk_size) ;

/**
*   @brief  List files on a directory
*   @param  path    Directory Path
*   @return List dir results
*/
FRESULT sd_list_dir(const char *path);

/** 
 * @brief   Perform depth scan of directory including its sub-folder
 * @param   path    Director path parent
 * @return  Scan results
 */
FRESULT scan_files(char* path);

/**
 * @brief   Delete a sub-directory even if it contains any file
 * @param   path    Path name buffer with the sub-directory to delete 
 * @param   sz_buff Size of path name buffer (items) 
 * @param   fno     Name read buffer
 * @return  Delete status
 */
FRESULT delete_node (TCHAR* path, UINT sz_buff, FILINFO* fno);

/**
*   @brief  Check file size
*   @param  filePath    File path to check size
*   @return 
*           File size in bytes
*/
uint32_t sd_check_file_size(const char *filePath);

#endif /* SCENARIO_APP_WEARABLE_AI_H_ */
