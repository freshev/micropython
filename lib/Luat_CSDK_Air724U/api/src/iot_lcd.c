#include "iot_lcd.h"


/**Ð´èë lcdãüeî
*@param cmd: ãüeî
**/
VOID iot_lcd_write_cmd(                          
                        UINT8 cmd 
                   )
{
    return  IVTBL(send_color_lcd_command)(cmd);
}

/**lcd Ð´ÈëlcdÊý¾Ý 
*@param	 	data: Êý¾Ý
**/
VOID iot_lcd_write_data(                               
                        UINT8 data                
                )
{
    return IVTBL(send_color_lcd_data)(data);
}

/**LCD³õê¼ »¯
*@param param: lcd³õê¼ »¯²îêý
*@Return True: ³é¹¦
* FALSE: § ° ü
**/	
BOOL iot_lcd_color_init(T_AMOPENAT_COLOR_LCD_PARAM *param )
{
    return IVTBL(init_color_lcd)(  param );
}

/** Âlcd
*@param rect: Ðâââ€™ÂÂÂÂ
*@param pDisplayBuffer: Â Â³”³ø³Cø
*@return TRUE: ³É¹´
* FALSE: SE§°Ü
**/
VOID iot_lcd_update_color_screen(
				T_AMOPENAT_LCD_RECT_T* rect,        /*Ðèòªë ¢ ðâµäçøóò*/
				UINT16 *pDisplayBuffer    )
{
    IVTBL(update_color_lcd_screen)(                       
                            rect,      
                            pDisplayBuffer       
                                   );
}
/** ½AHAJPGTO
*@param buffer: »º´æïôffer
*@param Len: »º´æïôffer³¤¶è
*@param Imageinfo: îä¼þñê½
*@return int32: ½ââë × ´ì¬âë
**/
INT32 iot_decode_jpeg(
                    UINT8 * buffer,
                    UINT32 len, 
                    T_AMOPENAT_IMAGE_INFO * imageinfo
                    )
{
    return IVTBL(ImgsDecodeJpeg)(buffer, len, imageinfo);
}

/** ÊÍ·Åjpg¸ñÊ½½âÂëÊý¾Ý
*@param		imageinfo: ÎÄ¼þ¸ñÊ½
*@return	INT32: ÊÍ·Å×´Ì¬Âë
**/
INT32 iot_free_jpeg_decodedata(
                    T_AMOPENAT_IMAGE_INFO * imageinfo
                    )
{
    return IVTBL(ImgsFreeJpegDecodedata)(imageinfo);
}
    

