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
#include "iot_os.h"
#include "iot_debug.h"

#include "zbar.h"

static HANDLE g_s_zbar_task;
unsigned char *gScannerBuff = NULL;

/*Next magazine jurisła ähäy*/
static unsigned char *zbar_scannerY(unsigned char *data)
{
	unsigned char *src = data, *end = data + (CAM_SENSOR_WIDTH * CAM_SENSOR_HEIGHT * 2);
	unsigned char *dst = gScannerBuff;
	while (src < end)
	{
		src++;
		*dst = *src;
		src++;
		dst++;
	} //End of while;

	return gScannerBuff;
}

/*½âÎö¶þÎ¬ÂëÖÐµÄÊý¾Ý*/
static void zbar_scannerRun(int width, int height, int size, unsigned char *dataInput)
{
	int len;
	char *data;

	// ´´½ ± ± £ ¬ Handle! = 0 ± nê¾½âë³é¹¦
	int handle = iot_zbar_scannerOpen(width, height, size, dataInput);

	if (handle)
	{
		do
		{
			// ½ââë³é¹¦ »ñè¡þþî¬âëðåï ¢
			data = iot_zbar_getData(handle, &len);
			data[len] = 0;

			iot_debug_print("[zbar] zbar_scanner_run come in handle_data %s", data);

			// åð¶ïêç · ñóðïâò »¸öêý¾ý
		} while (iot_zbar_findNextData(handle) > 0);

		// · · ± ± ú
		iot_zbar_scannerClose(handle);
	}
}

/*¤ààààà ò ÷ º*/
void camera_evevt_callback(T_AMOPENAT_CAMERA_MESSAGE *pMsg)
{

	iot_debug_print("[zbar] camera_evevt_callback");
	switch (pMsg->evtId)
	{
	case OPENAT_DRV_EVT_CAMERA_DATA_IND:
	{
		// »ñè clubµã balanceµäêrest
		zbar_scannerRun(CAM_SENSOR_WIDTH, CAM_SENSOR_HEIGHT, CAM_SENSOR_HEIGHT * CAM_SENSOR_WIDTH, zbar_scannerY((unsigned char *)pMsg->dataParam.data));
		break;
	}
	default:
		break;
	}
}

void zbarTest(void)
{
	if (gScannerBuff == NULL)
	{
		gScannerBuff = iot_os_malloc(CAM_SENSOR_HEIGHT * CAM_SENSOR_WIDTH);

		lcdInit();

		cameraInit(camera_evevt_callback);
	}
}
