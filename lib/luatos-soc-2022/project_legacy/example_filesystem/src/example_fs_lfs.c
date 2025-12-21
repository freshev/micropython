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

/*LFS_xxx series of functions, divided into file functions and folders, and several file system-related functions.
**Note**: Do not call the LFS_init LFS_deinit LFS_format function!!!*/

#include "common_api.h"
#include "FreeRTOS.h"
#include "task.h"

#include "lfs_port.h"
#include "rng.h"

// Demonstrate file operation, LFS_XXX file function
void exmaple_fs_lfs_file(void) {
    lfs_file_t fp = {0}; // Note that if it needs to be used across functions, lfs_file_t requires malloc
    uint8_t *buff = NULL;
    const char* filepath = "lfs_test.txt";
    struct lfs_info info = {0};
    char tmp[100];
    int ret = 0;

    DBG("check file exists? %s", filepath);
    // Determine whether the file exists by getting the size, and delete it if it exists
    if (LFS_stat(filepath, &info) == LFS_ERR_OK) {
        if (info.size > 0) {
            DBG("remove %s", filepath);
            LFS_remove(filepath);
        }
    }

    //----------------------------------------------
    //File writing demonstration
    //----------------------------------------------
    // Start writing the file
    DBG("test lfs file write");
    ret = LFS_fileOpen(&fp, filepath, LFS_O_CREAT | LFS_O_RDWR | LFS_O_TRUNC);
    if (ret != LFS_ERR_OK) {
        DBG("file open failed %d", ret);
        return;
    }
    // Generate random data and simulate business writing
    DBG("call malloc and rngGenRandom");
    buff = malloc(24 * 100);
    if (buff == NULL) {
        DBG("out of memory ?");
        LFS_fileClose(&fp);
        goto exit;
    }
    for (size_t i = 0; i < 100; i++)
    {
        rngGenRandom(buff + i*24);
    }

    //Write data in blocks
    DBG("call LFS_fileWrite");
    for (size_t i = 0; i < 24; i++)
    {
        ret = LFS_fileWrite(&fp, (const void*)(buff + i * 100), 100);
        if (ret != 100) {
            DBG("fail to write ret %d", ret);
            LFS_fileClose(&fp);
            goto exit;
        }
    }
    // To ensure correctness, sync operation can be performed before closing
    LFS_fileSync(&fp);
    // close file
    LFS_fileClose(&fp);

    //----------------------------------------------
    //File reading demonstration
    //----------------------------------------------
    // read file
    ret = LFS_fileOpen(&fp, filepath, LFS_O_RDONLY);
    if (ret != LFS_ERR_OK) {
        DBG("file open failed %d", ret);
        goto exit;
    }
    for (size_t i = 0; i < 24; i++)
    {
        ret = LFS_fileRead(&fp, tmp, 100);
        if (ret != 100) {
            DBG("fail to write ret %d", ret);
            LFS_fileClose(&fp);
            goto exit;
        }
        if (memcmp(tmp, buff + i * 100, 100) != 0) {
            DBG("file data NOT match");
        }
    }
    // Directly locate the position of offset=100 and re-read
    LFS_fileSeek(&fp, 100, LFS_SEEK_SET);
    ret = LFS_fileRead(&fp, tmp, 100);
    if (memcmp(tmp, buff + 100, 100) != 0) {
        DBG("file data NOT match at offset 100");
    }
    ret = LFS_fileTell(&fp);
    if (ret != 200) {
        // According to the previous logic, first set to 100, then read 100, the current offset should be 200
        DBG("file seek NOT match at offset 200");
    }
    
    //Close handle
    LFS_fileClose(&fp);

    //----------------------------------------------
    //File truncation demonstration
    //----------------------------------------------
    // LFS_fileTruncate needs to open the file in read-write mode, otherwise it will crash
    ret = LFS_fileOpen(&fp, filepath, LFS_O_RDWR);

    // Reduce file to 300 bytes
    LFS_fileTruncate(&fp, 300);
    ret = LFS_fileSize(&fp);
    if (ret != 300) {
        //The file size after cropping should be 300
        DBG("file size NOT 300, but %d", ret);
    }
    LFS_fileClose(&fp);


    //----------------------------------------------
    //File rename demonstration
    //----------------------------------------------
    // The file is still there, test the name change
    LFS_remove("newpath");
    LFS_rename(filepath, "newpath");
    // Read the file, the old path should get the file size not greater than 0
    info.size  = 0;
    LFS_stat(filepath, &info);
    if (info.size > 0) {
        DBG("file shall not exist");
    }
    // Read the file, the new path should get the file size equal to 300
    info.size  = 0;
    LFS_stat("newpath", &info);
    if (info.size != 300) {
        DBG("file shall 300 byte but %d", info.size);
    }

    LFS_remove(filepath);
    LFS_remove("newpath");

    //------------------------------------------------------
    // After demonstration, clean up resources
    exit:
        if (buff != NULL) {
            free(buff);
        }
        DBG("file example exited");
        return;
}


// Demonstrate file operation, LFS_XXX folder function
void exmaple_fs_lfs_dir(void) {
    // TODO
}

void exmaple_fs_lfs_main(void) {
    exmaple_fs_lfs_file();
    vTaskDelay(1000);
    exmaple_fs_lfs_dir();
}
