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

/*This demo uses the OsaXXX series API for demonstration.

Tip: The underlying file system is based on lifftefs, which has the characteristic of not losing data even after power failure.

When writing new data, if the file path already exists, the original data will not be cleared immediately when the file is opened for writing.

This feature, in extreme cases, will consume 2 times the flash space. Therefore, the following are suggestions when overwriting files.

1. If the file is important, follow the conventional writing method, and important files should be kept small in size.
2. If the file is not important, OsaRemove should be executed first, and then OsaFopen*/

// standard include
#include "common_api.h"
#include "FreeRTOS.h"
#include "task.h"

//Introduce osa file API
#include "osasys.h"

//Introduce random number API
#include "rng.h"


#define FILE_NAME "test.txt"
#define BUFF_SIZE (24)

//The purpose of this buff is to store the random data of file write and compare it with the data of file read.
// The buff size is 24 because rngGenRandom is fixed at 24 bytes.
static uint8_t tmpbuff[BUFF_SIZE];

static void exmaple_file_write(void) {
    //----------------------------------------------
    //File writing demonstration
    //----------------------------------------------
	// Optional, remove the existing file first, see the tip at the head of this file for details
	// OsaRemove(FILE_NAME);

	// The usage of OsaFopen is consistent with fopen, and supports operations such as rb/wb/wb+/a/a+
	OSAFILE fp = OsaFopen(FILE_NAME, "wb+");
	if (fp == NULL) {
		DBG("open file for write, failed");
		return;
	}
	// Random number generator, note that its input is a 24-byte array.
	rngGenRandom(tmpbuff);

	// Write data, the same as fwrite, the return value is the number of blocks written (1), not the number of bytes
	uint32_t ret = OsaFwrite(tmpbuff, BUFF_SIZE, 1, fp);
	if (ret != 1) {
		DBG("fail to write, excpt 1 but %d", BUFF_SIZE, ret);
	}
	//After the operation is completed, be sure to close the file handle
	OsaFclose(fp);

	return;
}

static void exmaple_file_read(void) {
    //----------------------------------------------
    //File reading demonstration
    //----------------------------------------------
	// The usage of OsaFopen is consistent with fopen, and supports operations such as rb/wb/wb+/a/a+
	OSAFILE fp = OsaFopen(FILE_NAME, "rb");
	if (fp == NULL) {
		DBG("open file for read, failed");
		return;
	}
	
	char buff[BUFF_SIZE];
	// Read data, the same as fread, the return value is the number of blocks (1), not the number of bytes
	uint32_t ret = OsaFread(buff, BUFF_SIZE, 1, fp);
	if (ret != 1) {
		DBG("fail to read, excpt %d but %d", BUFF_SIZE, ret);
	}
	// Compare with fwrite data, it should be consistent
	if (memcmp(buff, tmpbuff, BUFF_SIZE)) {
		DBG("file content NOT match!!");
	}
	// Finally, be sure to close the file handle
	OsaFclose(fp);

	return;
}

void exmaple_fs_osa_main(void) {
	vTaskDelay(1000);
	exmaple_file_write();
	vTaskDelay(1000);
	exmaple_file_read();
}
