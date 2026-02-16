#include "wearable_ai.h"
#include "xprintf.h"

#include "hx_drv_gpio.h"
#include "hx_drv_scu.h"

// Variables and class instances
FATFS fs;               /* Filesystem object */

//app implement GPIO_Output_Level/GPIO_Pinmux/GPIO_Dir for fatfs\port\mmc_spi\mmc_we2_spi.c ARM_SPI_SS_MASTER_SW
void SSPI_CS_GPIO_Output_Level(bool setLevelHigh)
{
    hx_drv_gpio_set_out_value(GPIO16, (GPIO_OUT_LEVEL_E) setLevelHigh);
}

void SSPI_CS_GPIO_Pinmux(bool setGpioFn)
{
    if (setGpioFn)
        hx_drv_scu_set_PB5_pinmux(SCU_PB5_PINMUX_GPIO16, 0);
    else
        hx_drv_scu_set_PB5_pinmux(SCU_PB5_PINMUX_SPI_M_CS_1, 0);
}

void SSPI_CS_GPIO_Dir(bool setDirOut)
{
    if (setDirOut)
        hx_drv_gpio_set_output(GPIO16, GPIO_OUT_HIGH);
    else
        hx_drv_gpio_set_input(GPIO16);
}


FRESULT sd_init(void)
{
    FRESULT res;            /* API result code */
    char cur_dir[128];
    uint32_t len = 128;

    hx_drv_scu_set_PB2_pinmux(SCU_PB2_PINMUX_SPI_M_DO_1, 1);
    hx_drv_scu_set_PB3_pinmux(SCU_PB3_PINMUX_SPI_M_DI_1, 1);
    hx_drv_scu_set_PB4_pinmux(SCU_PB4_PINMUX_SPI_M_SCLK_1, 1);
    hx_drv_scu_set_PB5_pinmux(SCU_PB5_PINMUX_SPI_M_CS_1, 1);

    printf("Start to test SD card fatfs (%s, %s)\r\n", __DATE__, __TIME__);

    res = f_mount(&fs, DRV, 1);
    if (res)
    {
        printf("f_mount res = %d\r\n", res);
    }

    printf("Get current directory: \r\n");
    res = f_getcwd(cur_dir, len);      /* Get current directory */
    if (res)
    {
        printf("f_getcwd res = %d\r\n", res);
    }
    else
    {
        printf("cur_dir = %s\r\n", cur_dir);
    }

    // List files of current dir files
    res = sd_list_dir(cur_dir);
    if (res)
        printf("sd_list_dir res = %d\r\n", res);
    
    return res;
}

void sd_uninitialize(void)
{
    f_unmount(DRV);
}

FRESULT sd_write_file(const char *filePath, uint8_t *dt)
{
    FRESULT res;            /* API result code */
    FIL fil_w;              /* File object */
    uint32_t bw;            /* Bytes written */

    res = f_open(&fil_w, filePath, FA_CREATE_ALWAYS | FA_WRITE);
    if (res == FR_OK)
    {
        printf("write file : %s .\r\n", filePath);
        res = f_write(&fil_w, dt, strlen(dt), &bw);
        if (res) { printf("f_write res = %d\r\n", res); }
        f_close(&fil_w);
    }
    else
    {
        printf("[ERR] f_open res = %d\r\n", res, filePath);
    }
    return res;
}

uint32_t sd_read_file(const char *filePath, uint8_t *dt, size_t offset, size_t chunk_size)
{
    FRESULT res;            /* API result code */
    FIL fil_r;              /* File object */
    uint32_t br;            /* Bytes read */
    
    res = f_open(&fil_r, filePath, FA_OPEN_EXISTING | FA_READ);
    if (res)
        printf("f_open res = %d\r\n", res);

    // // Move the file pointer to the start of the next chunk
    f_lseek(&fil_r, offset);

    res = f_read(&fil_r, dt, chunk_size, &br);
    if (res) { printf("f_read res = %d\r\n", res); }

    printf("read %s content\r\n", filePath);
    f_close(&fil_r);
    return br;
}

/* List contents of a directory */
FRESULT sd_list_dir (const char *path)
{
    FRESULT res;
    DIR dir;
    FILINFO fno;
    int nfile, ndir;


    res = f_opendir(&dir, path);                       /* Open the directory */
    if (res == FR_OK) {
        nfile = ndir = 0;
        for (;;) {
            res = f_readdir(&dir, &fno);                   /* Read a directory item */
            if (res != FR_OK || fno.fname[0] == 0) break;  /* Error or end of dir */
            if (fno.fattrib & AM_DIR) {            /* Directory */
                printf("   <DIR>   %s\r\n", fno.fname);
                ndir++;
            } else {                               /* File */
                printf("%10u %s\r\n", fno.fsize, fno.fname);
                nfile++;
            }
        }
        f_closedir(&dir);
        printf("%d dirs, %d files.\r\n", ndir, nfile);
    } else {
        printf("Failed to open \"%s\". (%u)\r\n", path, res);
    }
    return res;
}


/* Recursive scan of all items in the directory */
FRESULT scan_files (char* path)     /* Start node to be scanned (***also used as work area***) */
{
    FRESULT res;
    DIR dir;
    UINT i;
    static FILINFO fno;


    res = f_opendir(&dir, path);                       /* Open the directory */
    if (res == FR_OK) {
        for (;;) {
            res = f_readdir(&dir, &fno);                   /* Read a directory item */
            if (res != FR_OK || fno.fname[0] == 0) break;  /* Break on error or end of dir */
            if (fno.fattrib & AM_DIR) {                    /* It is a directory */
                i = strlen(path);
                sprintf(&path[i], "/%s", fno.fname);
                res = scan_files(path);                    /* Enter the directory */
                if (res != FR_OK) break;
                path[i] = 0;
            } else {                                       /* It is a file. */
                printf("%s/%s\r\n", path, fno.fname);
            }
        }
        f_closedir(&dir);
    }
    return res;
}

FRESULT delete_node (TCHAR* path,UINT sz_buff,FILINFO* fno)
{
    UINT i, j;
    FRESULT fr;
    DIR dir;

    fr = f_opendir(&dir, path); /* Open the sub-directory to make it empty */
    if (fr != FR_OK) return fr;

    for (i = 0; path[i]; i++) ; /* Get current path length */
    path[i++] = _T('/');

    for (;;) {
        fr = f_readdir(&dir, fno);  /* Get a directory item */
        if (fr != FR_OK || !fno->fname[0]) break;   /* End of directory? */
        j = 0;
        do {    /* Make a path name */
            if (i + j >= sz_buff) { /* Buffer over flow? */
                fr = 100; break;    /* Fails with 100 when buffer overflow */
            }
            path[i + j] = fno->fname[j];
        } while (fno->fname[j++]);
        if (fno->fattrib & AM_DIR) {    /* Item is a sub-directory */
            fr = delete_node(path, sz_buff, fno);
        } else {                        /* Item is a file */
            fr = f_unlink(path);
        }
        if (fr != FR_OK) break;
    }

    path[--i] = 0;  /* Restore the path name */
    f_closedir(&dir);

    if (fr == FR_OK) fr = f_unlink(path);  /* Delete the empty sub-directory */
    return fr;
}

uint32_t sd_check_file_size(const char *filePath)
{
    FILINFO fileInfo;
    FRESULT res;
    FSIZE_t fileSize;

    res = f_stat(filePath, &fileInfo);
    fileSize = fileInfo.fsize;
    if(res != FR_OK)
    {
        printf("File to obtain file size\r\n");
        return 0;   
    }
    return (uint32_t) fileSize;
}