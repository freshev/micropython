#ifndef __IOT_CAMERA_H__
#define __IOT_CAMERA_H__

#include "iot_os.h"

/**
 * @defgroup iot_sdk_device ÍâÉè½Ó¿Ú
 * @{*/
	/**@Example zbar/demo_zbar_camera.c
	* Camera½ó¿ones*/ 

/**
 * @defgroup iot_sdk_camera ÉãÏñÍ·½Ó¿Ú
 * @{*/

/**Éãïñn · ³õê¼ »¯
*@param Camerapam: ³õê¼ »¯²îêý
*@Return True: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_camera_init(T_AMOPENAT_CAMERA_PARAM *cameraParam);

/**´ò ‘ªéãïñn ·
*@stop video:
*@Return True: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_camera_poweron(BOOL videoMode);  
/**¹Ø ± õéãïñn ·
*@Return True: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_camera_poweroff(void); 
/**
*@param Preview PREVARE: O¤àà²îêý
*@Return True: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_camera_preview_open(T_AMOPENAT_CAM_PREVIEW_PARAM *previewParam);
/** Íë³Töh to¤9à
* @ Return TRUE: ³é¹|
* FALSE: ê ° ü
**/
BOOL iot_camera_preview_close(void);
/**Åäõõ
*@param Filename: ± £ ´ænsideà¬µäîä¼þãû
*@param Captured: O¤àà²îêý
*@Return True: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_camera_capture(char *fileName, T_AMOPENAT_CAM_CAPTURE_PARAM *captureParam);

/**Éèöãcamerserseà´ææ ÷
*@param initregtable_p: Camsy
*@param Len: Cam mention´ææ ÷ ³³
*@Return True: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_camera_WriteReg(PAMOPENAT_CAMERA_REG initRegTable_p, int len);

/** @}*/
/** @}*/


#endif


