/* Copyright (C) 2018 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * This software is supplied "AS IS" without any warranties.
 * RDA assumes no responsibility or liability for the use of the software,
 * conveys no license or title under any patent, copyright, or mask work
 * right to the product. RDA reserves the right to make changes in the
 * software without notification.  RDA also make no representation or
 * warranty that such application will be suitable for the specified use
 * without further testing or modification.
 */
#include <stdio.h>
#include <ctype.h>
#include "iot_os.h"
#include "iot_debug.h"
#include "iot_tts.h"
#include "string.h"

typedef struct{
	HANDLE demo_tts_task;
	HANDLE demo_tts_sem;
}TTS_PARAM_T;

TTS_PARAM_T g_demo_tts;

#define TTS_DEBUG(fmt, args...) do { iot_debug_print("[iot_tts] "fmt, ##args); } while(0)
#define TTS_SUSPENDED (0xFFFFFFFF)
char *tts_text = "£ £";
const char unicodeHexStr[] = "6B228FCE003100310033";

void tts_play_cb(OPENAT_TTS_CB_MSG msg_id,u8 event)
{
	TTS_DEBUG("tts_play_cb msg_id = %d,event = %d",msg_id,event);
	/*· لتê ðåºmo q What is the letter "*/
	iot_os_release_semaphore(g_demo_tts.demo_tts_sem);
}

static void HexStrToLittleEndianUnicode(const char *source, int sourceLen, char *dest)
{
    short i;
    unsigned char highByte = 0, lowByte = 0;
	char temp =0;

    for (i = 0; i < sourceLen; i += 2)
    {
        highByte = toupper((unsigned char)source[i]);
        lowByte = toupper((unsigned char)source[i + 1]);

        if (highByte > 0x39)
            highByte -= 0x37;
        else
            highByte -= 0x30;

        if (lowByte > 0x39)
            lowByte -= 0x37;
        else
            lowByte -= 0x30;
		
        dest[i / 2] = (highByte << 4) | lowByte;
	}
	for(i = 0; i < (sourceLen / 2); i+=2)
	{
		temp = dest[i];
		dest[i] = dest[i+1];
		dest[i+1] = temp;
	}
    return;
}


static VOID demo_tts_task(PVOID pParameter)
{

    TTS_DEBUG("start");

	iot_tts_init(tts_play_cb);
	/*Í¨¹ýðåºåe¿ × èèû¿øöæ*/
	g_demo_tts.demo_tts_sem = iot_os_create_semaphore(1);
	char unicodeStr[20] = {0};
	while(1)
	{
		/*ÑWe ± Â »*/
		iot_os_sleep(1000);
		TTS_DEBUG("tts player start first");
		iot_os_wait_semaphore(g_demo_tts.demo_tts_sem,TTS_SUSPENDED);
		iot_tts_set_param(OPENAT_TTS_PARAM_CODEPAGE,OPENAT_CODEPAGE_GBK);
		iot_tts_play(tts_text,strlen(tts_text));

		TTS_DEBUG("tts player start second");
		/*µÈ´ý²¥·Å½eÊø*/
		iot_os_wait_semaphore(g_demo_tts.demo_tts_sem,TTS_SUSPENDED);
		/*£00 £¬ · ~ ~ · ~ ~ · + Nomen-± ± ±*/
		iot_tts_set_param(OPENAT_TTS_PARAM_VOLUME,100);
		/*Éèè² ¥ `` `` `H¬ §§§uly*/
		//iot_tts_set_param(OPENAT_TTS_PARAM_SPEED,50);
		HexStrToLittleEndianUnicode(unicodeHexStr,strlen(unicodeHexStr),unicodeStr);
		iot_tts_set_param(OPENAT_TTS_PARAM_CODEPAGE,OPENAT_CODEPAGE_UNICODE);
		iot_tts_play(unicodeStr,strlen(unicodeHexStr)/2);
	}
}

VOID demo_tts(VOID)
{
    //1. ´´½¨demo_tts_task ,Ñ­»·²¥·Åtts
    g_demo_tts.demo_tts_task =  iot_os_create_task(demo_tts_task, NULL, 
        2048, 1, OPENAT_OS_CREATE_DEFAULT, "demo_tts_task");
}

int appimg_enter(void *param)
{    
	iot_debug_print("[tts] appimg_enter");

    demo_tts();
    return 0;
}

void appimg_exit(void)
{
    iot_debug_print("[tts] appimg_exit");
}
