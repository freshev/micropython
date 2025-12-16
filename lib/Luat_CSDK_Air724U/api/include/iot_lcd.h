#ifndef __IOT_LCD_H__
#define __IOT_LCD_H__

#include "iot_os.h"

/**
 * @ingroup iot_sdk_device ÍâÉè½Ó¿Ú
 * @{*/
/**
 * @defgroup iot_sdk_lcd lcd½Ó¿Ú
 * @{*/
 
/**@example zbar/demo_zbar_lcd.c
* LCD&¼üÅÌ½Ó¿ÚÊ¾Àý*/ 

/**Ð´èë lcdãüeî
*@param cmd: ãüeî
**/
VOID iot_lcd_write_cmd(                          
                        UINT8 cmd 
                   );

/**lcd Ð´ÈëlcdÊý¾Ý 
*@param	 	data: Êý¾Ý
**/
VOID iot_lcd_write_data(                               
                        UINT8 data                
                );


/**LCD³õê¼ »¯
*@param param: lcd³õê¼ »¯²îêý
*@Return True: ³é¹¦
* FALSE: § ° ü
**/	
BOOL iot_lcd_color_init(T_AMOPENAT_COLOR_LCD_PARAM *param );

/** Âlcd
*@param rect: Ðâââ€™ÂÂÂÂ
*@param pDisplayBuffer: Â Â³”³ø³Cø
*@return TRUE: ³É¹´
* FALSE: SE§°Ü
**/
VOID iot_lcd_update_color_screen(
				T_AMOPENAT_LCD_RECT_T* rect,        /*Ðèòªë ¢ ðâµäçøóò*/
				UINT16 *pDisplayBuffer    );

/** ½âÂëjpg¸ñÊ½Í¼Æ¬
*@param		buffer:ÏÔÊ¾buffer
*@param		len:ÏÔÊ¾buffer³¤¶È
*@param		imageinfo: ÎÄ¼þ¸ñÊ½
*@return	INT32: ½âÂë×´Ì¬Âë
**/
INT32 iot_decode_jpeg(
                    UINT8 * buffer,
                    UINT32 len,
                    T_AMOPENAT_IMAGE_INFO * imageinfo
                    );

/** ÊÍ·Åjpg¸ñÊ½½âÂëÊý¾Ý
*@param		imageinfo: ÎÄ¼þ¸ñÊ½
*@return	INT32: ÊÍ·Å×´Ì¬Âë
**/
INT32 iot_free_jpeg_decodedata(
                    T_AMOPENAT_IMAGE_INFO * imageinfo
                    );
/** @}*/
/** @}*/
#endif









