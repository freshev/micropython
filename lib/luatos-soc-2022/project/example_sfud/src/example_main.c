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
#include "sfud.h"
#include "luat_fs.h"
#include "common_api.h"
#include "bsp_custom.h"
#include "luat_mem.h"

#include "lfs.h"

/*Note that file operations under sfud are currently thread-unsafe, so please pay attention!!!!!!!!*/

// The Flash VCC power supply of the Cloud Speaker development board is controlled, and there is no power by default!!!
// Other development boards do not need to pay attention to this
#define SPI_FLASH_VCC_PIN (HAL_GPIO_26)

luat_rtos_task_handle sfud_task_handle;

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
    const char* filepath = "/sfud/lfs_test.txt";
    const char* newpath = "/sfud/newpath";
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
    for (size_t i = 0; i < 100; i++)
    {
        luat_crypto_trng(buff + i*24, 24);
    }

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
    luat_fs_truncate(filepath, 300);

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
    ret=luat_fs_mkdir("/sfud/luatos");
    LUAT_DEBUG_PRINT("mkdir result%d",ret);
    if (0==ret)
    {
       LUAT_DEBUG_PRINT("mkdir succeed");
    }
    FILE* fp = NULL;
    uint8_t *buff = NULL;
    const char* filepath = "/sfud/luatos/luatos_test.txt";
    fp = luat_fs_fopen(filepath, "wb+");
    if (!fp)
    {
       LUAT_DEBUG_PRINT("file open failed %s", filepath);
       return;
    }
    luat_fs_fclose(fp);
}


luat_spi_t sfud_spi_flash = {
        .id = 0,
        .CPHA = 0,
        .CPOL = 0,
        .dataw = 8,
        .bit_dict = 0,
        .master = 1,
        .mode = 0,
        .bandrate=25600000,
        .cs = 8
};
extern lfs_t* flash_lfs_sfud(sfud_flash* flash, size_t offset, size_t maxsize);
static void task_test_sfud(void *param)
{

    int re = -1;
    // The Flash power supply of the cloud speaker is directly connected to 3.3V for other development boards, so this section is not needed.
    luat_gpio_cfg_t gpio_cfg;
	luat_gpio_set_default_cfg(&gpio_cfg);
	gpio_cfg.pin = SPI_FLASH_VCC_PIN;
	gpio_cfg.alt_fun = 0;
	gpio_cfg.mode = LUAT_GPIO_OUTPUT;
	gpio_cfg.output_level = 1;
	luat_gpio_open(&gpio_cfg);

    luat_spi_setup(&sfud_spi_flash);

    if (re = sfud_init()!=0){
        LUAT_DEBUG_PRINT("sfud_init error is %d\n", re);
    }
    const sfud_flash *flash = sfud_get_device_table();
    luat_fs_init();
    lfs_t* lfs = flash_lfs_sfud(flash, 0, 0);
    if (lfs) {
	    luat_fs_conf_t conf = {
		    .busname = (char*)lfs,
		    .type = "lfs2",
		    .filesystem = "lfs2",
		    .mount_point = "/sfud",
	    };
	    int ret = luat_fs_mount(&conf);
        LUAT_DEBUG_PRINT("vfs mount %s ret %d", "/sfud", ret);
    }
    else {
        LUAT_DEBUG_PRINT("flash_lfs_sfud error");
    }

    print_fs_info("/sfud");
    exmaple_fs_lfs_dir();
    exmaple_fs_luat_file();
    recur_fs("/");
    recur_fs("/sfud");
    while (1)
    {
        luat_rtos_task_sleep(1000);
    }
}

//Hezhouyun speaker development board opens the comment below to enable flash
// #define FLASH_EN	HAL_GPIO_26
// #define FLASH_EN_ALT_FUN	0

static void task_demo_sfud(void)
{
    // luat_gpio_cfg_t gpio_cfg;
	// luat_gpio_set_default_cfg(&gpio_cfg);

	// gpio_cfg.pin = FLASH_EN;
	// gpio_cfg.alt_fun = FLASH_EN_ALT_FUN;
	// luat_gpio_open(&gpio_cfg);
	// luat_gpio_set(FLASH_EN, LUAT_GPIO_HIGH);

    luat_rtos_task_create(&sfud_task_handle, 2048, 20, "sfud", task_test_sfud, NULL, NULL);
}

INIT_TASK_EXPORT(task_demo_sfud,"1");



