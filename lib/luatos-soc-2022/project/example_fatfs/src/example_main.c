/*
 * Copyright (c) 2022 OpenLuat & AirM2M
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include "common_api.h"
#include "luat_rtos.h"
#include "luat_debug.h"

#include "luat_gpio.h"
#include "luat_spi.h"
#include "luat_lib_fatfs.h"
#include "luat_fs.h"
#include "common_api.h"
#include "bsp_custom.h"
#include "luat_mem.h"

#include "luat_vfs.h"

#define SPI_TF_SPI_ID 1
#define SPI_TF_CS_PIN	HAL_GPIO_27		//If low power consumption is not required, ordinary GPIO can be used
#define SPI_TF_POWER_PIN HAL_GPIO_28	//If there is no power control pin, you can write 0xff. It is strongly recommended to add power control. If low power consumption is not required, you can use ordinary GPIO.
luat_rtos_task_handle fatfs_task_handle;

static int print_fs_info(const char* dir_path)
{
    luat_fs_info_t fs_info = {0};

    luat_fs_info(dir_path, &fs_info);
    //Print file system space information
    LUAT_DEBUG_PRINT("fs_info %s %d %d %d %d", 
        fs_info.filesystem, 
        fs_info.type, 
        fs_info.total_block, 
        fs_info.block_used, 
        fs_info.block_size);
}

static int recur_fs(const char* dir_path)
{
    luat_fs_dirent_t *fs_dirent = LUAT_MEM_MALLOC(sizeof(luat_fs_dirent_t)*100);
    memset(fs_dirent, 0, sizeof(luat_fs_dirent_t)*100);

    int lsdir_cnt = luat_fs_lsdir(dir_path, fs_dirent, 0, 100);

    if (lsdir_cnt > 0)
    {
        char path[255] = {0};

        LUAT_DEBUG_PRINT("dir_path=%s, lsdir_cnt=%d", dir_path, lsdir_cnt);

        for (size_t i = 0; i < lsdir_cnt; i++)
        {
            memset(path, 0, sizeof(path));            

            switch ((fs_dirent+i)->d_type)
            {
            // file type
            case 0:   
                snprintf(path, sizeof(path)-1, "%s%s", dir_path, (fs_dirent+i)->d_name);             
                LUAT_DEBUG_PRINT("\tfile=%s, size=%d", path, luat_fs_fsize(path));
                break;
            case 1:
                snprintf(path, sizeof(path)-1, "%s/%s/", dir_path, (fs_dirent+i)->d_name);
                recur_fs(path);
                break;

            default:
                break;
            }
        }        
    }

    LUAT_MEM_FREE(fs_dirent);
    fs_dirent = NULL;
    
    return lsdir_cnt;
}

// Demonstrate file operation, luat_fs_XXX file function
void exmaple_fs_luat_file(void) {
    FILE* fp = NULL;
    uint8_t *buff = NULL;
    const char* filepath = "/tf/lfs_test.txt";
    const char* newpath = "/tf/newpath";
    char tmp[100];
    int ret = 0;
    size_t size = 0;

    LUAT_DEBUG_PRINT("check file exists? %s", filepath);
    // Determine whether the file exists by getting the size, and delete it if it exists
    if (luat_fs_fexist(filepath)) {
        LUAT_DEBUG_PRINT("remove %s", filepath);
        luat_fs_remove(filepath);
    }

    //----------------------------------------------
    //File writing demonstration
    //----------------------------------------------
    // Start writing the file
    LUAT_DEBUG_PRINT("test lfs file write");
    fp = luat_fs_fopen(filepath, "wb+");
    if (!fp) {
        LUAT_DEBUG_PRINT("file open failed %s", filepath);
        return;
    }
    // Generate random data and simulate business writing
    LUAT_DEBUG_PRINT("call malloc and rngGenRandom");
    buff = malloc(24 * 100);
    if (buff == NULL) {
        LUAT_DEBUG_PRINT("out of memory ?");
        luat_fs_fclose(fp);
        goto exit;
    }
//    for (size_t i = 0; i < 20; i++)
//    {
//        luat_crypto_trng(buff + i*24, 24);
//    }
    luat_crypto_trng(buff, 24 * 50);
    //Write data in blocks
    LUAT_DEBUG_PRINT("call luat_fs_write");
    for (size_t i = 0; i < 24; i++)
    {
        ret = luat_fs_fwrite((const void*)(buff + i * 100), 100, 1, fp);
        if (ret < 0) {
            LUAT_DEBUG_PRINT("fail to write ret %d", ret);
            luat_fs_fclose(fp);
            goto exit;
        }
    }
    // close file
    luat_fs_fclose(fp);

    //----------------------------------------------
    //File reading demonstration
    //----------------------------------------------
    // read file
    fp = luat_fs_fopen(filepath, "r");
    if (!fp) {
        LUAT_DEBUG_PRINT("file open failed %s", filepath);
        goto exit;
    }
    for (size_t i = 0; i < 24; i++)
    {
        ret = luat_fs_fread(tmp, 100, 1, fp);
        if (ret < 0) {
            LUAT_DEBUG_PRINT("fail to write ret %d", ret);
            luat_fs_fclose(fp);
            goto exit;
        }
        if (memcmp(tmp, buff + i * 100, 100) != 0) {
            LUAT_DEBUG_PRINT("file data NOT match");
        }
    }
    // Directly locate the position of offset=100 and re-read
    luat_fs_fseek(fp, 100, SEEK_SET);
    ret = luat_fs_fread(tmp, 100, 1, fp);
    if (memcmp(tmp, buff + 100, 100) != 0) {
        LUAT_DEBUG_PRINT("file data NOT match at offset 100");
    }
    ret = luat_fs_ftell(fp);
    if (ret != 200) {
        // According to the previous logic, first set to 100, then read 100, the current offset should be 200
        LUAT_DEBUG_PRINT("file seek NOT match at offset 200");
    }
    
    //Close handle
    luat_fs_fclose(fp);

    //----------------------------------------------
    //File truncation demonstration
    //----------------------------------------------

    //Cut directly with path
    luat_fs_truncate(filepath, 300);	//fatfs does not support it, so it is invalid

    //----------------------------------------------
    //File rename demonstration
    //----------------------------------------------
    // The file is still there, test the name change
    luat_fs_remove(newpath);
    luat_fs_rename(filepath, newpath);
    // Read the file, the old path should get the file size not greater than 0
    
    size = luat_fs_fsize(filepath);
    if (size > 0) {
        LUAT_DEBUG_PRINT("file shall not exist");
    }
    // Read the file, the new path should get the file size equal to 300
    size = luat_fs_fsize(newpath);
    if (size != 300) {
        LUAT_DEBUG_PRINT("file shall 300 byte but %d", size);
    }

    luat_fs_remove(filepath);
    luat_fs_remove(newpath);

    //------------------------------------------------------
    // After demonstration, clean up resources
    exit:
        if (buff != NULL) {
            free(buff);
        }
        LUAT_DEBUG_PRINT("file example exited");
        return;
}
// Demonstrate folder operations
void exmaple_fs_lfs_dir(void)
{
    int ret=-1;
    ret=luat_fs_mkdir("/tf/luatos");
    LUAT_DEBUG_PRINT("mkdir result%d",ret);
    if (0==ret)
    {
       LUAT_DEBUG_PRINT("mkdir succeed");
    }
    FILE* fp = NULL;
    uint8_t *buff = NULL;
    const char* filepath = "/tf/luatos/luatos_test.txt";
    fp = luat_fs_fopen(filepath, "wb+");
    if (!fp)
    {
       LUAT_DEBUG_PRINT("file open failed %s", filepath);
       return;
    }
    luat_fs_fclose(fp);
}




static void task_test_fatfs(void *param)
{
	luat_spi_t fatfs_spi_flash = {
	        .id = SPI_TF_SPI_ID,
	        .CPHA = 0,
	        .CPOL = 0,
	        .dataw = 8,
	        .bit_dict = 0,
	        .master = 1,
	        .mode = 1,
	        .bandrate=400000,
	        .cs = SPI_TF_CS_PIN,
	};

	luat_gpio_cfg_t gpio_cfg;
	luat_debug_set_fault_mode(LUAT_DEBUG_FAULT_HANG_RESET);
	luat_fs_init();
    int re = -1;
    luat_spi_setup(&fatfs_spi_flash);
    luat_gpio_set_default_cfg(&gpio_cfg);
    gpio_cfg.pin = SPI_TF_POWER_PIN;
    gpio_cfg.output_level = 0;
    luat_gpio_open(&gpio_cfg);
    gpio_cfg.pin = SPI_TF_CS_PIN;
    gpio_cfg.output_level = 1;
    luat_gpio_open(&gpio_cfg);
    if (re = luat_fatfs_mount(DISK_SPI, SPI_TF_SPI_ID, SPI_TF_CS_PIN, 25600000, SPI_TF_POWER_PIN, 100, "spi", "/tf")!=0){
        LUAT_DEBUG_PRINT("fatfs_init error is %d\n", re);
        while (1)
        {
            luat_rtos_task_sleep(1000);
        }
    }

    print_fs_info("/tf");
    exmaple_fs_lfs_dir();
    exmaple_fs_luat_file();
    recur_fs("/");
    recur_fs("/tf");
    while (1)
    {
        luat_rtos_task_sleep(1000);
    }
}


static void task_demo_fatfs(void)
{
    luat_rtos_task_create(&fatfs_task_handle, 4096, 20, "fatfs", task_test_fatfs, NULL, NULL);
}

INIT_TASK_EXPORT(task_demo_fatfs,"1");

void soc_get_unilog_br(uint32_t *baudrate)
{
#ifdef LUAT_UART0_LOG_BR_12M
	*baudrate = 12000000; //UART0 uses the log port to output a 12M baud rate, which must be converted from high-performance USB to TTL.
#else
	*baudrate = 6000000; //UART0 uses the log port to output a 6M baud rate, which must be converted from high-performance USB to TTL.
#endif
}

/** If you want to maintain the unilog function and use SPI1, you need to multiplex the IO of UART0 to other places. See the following operation.*/

extern int32_t soc_unilog_callback(void *pdata, void *param);
bool soc_init_unilog_uart(uint8_t port, uint32_t baudrate, bool startRecv)
{
	soc_get_unilog_br(&baudrate);
	GPIO_IomuxEC618(GPIO_ToPadEC618(HAL_GPIO_16, 0), 3, 0, 0);
	GPIO_IomuxEC618(GPIO_ToPadEC618(HAL_GPIO_17, 0), 3, 0, 0);
	GPIO_PullConfig(GPIO_ToPadEC618(HAL_GPIO_16, 0), 1, 1);
	GPIO_IomuxEC618(GPIO_ToPadEC618(HAL_GPIO_14, 0), 0, 0, 0);	//The original UART0 TXRX changes back to GPIO function
	GPIO_IomuxEC618(GPIO_ToPadEC618(HAL_GPIO_15, 0), 0, 0, 0);
	Uart_BaseInitEx(port, baudrate, 0, 256, UART_DATA_BIT8, UART_PARITY_NONE, UART_STOP_BIT1, soc_unilog_callback);
	return true;
}

