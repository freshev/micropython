#include "iot_tts.h"


/**³õêrd »¯ttsòýçæ
*@param cb: tts² ¥ · å½e¹û »Øµ ÷ º thousand
*@Return True: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_tts_init(
                    TTS_PLAY_CB cb         
              )
{
    return OPENAT_tts_init(cb);
}

/**tts² ¥ · åîä ± ¾
*@param text: ´ý² ¥ · åîä ± ¾
*@param len: îä ± ¾³י £ ¨ × Ö porthi £ ©
*@Return True: ³é¹¦
			FALSE: § ° ü
**/
BOOL iot_tts_play(                                   
                    char *text,u32 len                    
              )
{
    return OPENAT_tts_play(text, len);
}

/**TTSÍ £ Ö¹² ¥ · å
*@return true: ³é¹¦
			False: Ê§ ° ü
**/
BOOL iot_tts_stop(      )
{
    return OPENAT_tts_stop();
}

/**Éèöãttsåäöã²îêý
*@param Flag: ²îêý ± êö¾
*@param value: ²îêýöµ
*@Return True: ³é¹¦
			FALSE: § ° ü
**/
BOOL iot_tts_set_param(
		OPENAT_TTS_PARAM_FLAG flag,u32 value
		)
{
	return OPENAT_tts_set_param(flag,value);
}


