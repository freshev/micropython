#ifndef __IOT_AUDIO_H__
#define __IOT_AUDIO_H__

#include "iot_os.h"
#include "am_openat.h"



/**
 * @defgroup iot_sdk_audio ÒôÆµ½Ó¿Ú
 * @{*/
/**@example audio/demo_audio.c
* audio½Ó¿ÚÊ¾Àý*/

/**´ò °
*@Note ôún »» ° ªrse ± µ ÷ Oã
*@Return True: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_audio_open_tch(                                        
                        VOID
                );

/**¹Ø ± õóïòô
*@Note Í »° ½eêøê ± µ ÷ Ó
*@Return True: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_audio_close_tch(                                      
                        VOID
                 );

/**² ¥ · åtoneòô
*@param Tonetype: Toneòôààààðn
*@param Duration: ² ¥ · åê ± ³¤
*@stop volume: ² ¥ · åôe
*@Return True: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_audio_play_tone(                                        
                        E_AMOPENAT_TONE_TYPE toneType,     
                        UINT16 duration,                   
                        E_AMOPENAT_SPEAKER_GAIN volume     
                 );

/**In £ Ö¹² ¥ · ÅtoneÒÔ
*@return true: ³é¹¦
* FALSE: Ê§ ° ü
**/
BOOL iot_audio_stop_tone(                                        
                        VOID
                 );


/**² ¥ · ådtmfòô
*@param dtmfftype: dtmfààðn
*@param Duration: ² ¥ · åê ± ³¤
*@stop volume: ² ¥ · åôe
*@Return True: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_audio_play_dtmf(                                        
                        E_AMOPENAT_DTMF_TYPE dtmfType,     
                        UINT16 duration,                   
                        E_AMOPENAT_SPEAKER_GAIN volume     
                 );

/**In £ Ö¹² ¥ · ÅDTMFÒÔ
*@return true: ³é¹¦
* FALSE: Ê§ ° ü
**/
BOOL iot_audio_stop_dtmf(                            
                        VOID
                 );

/**² ¥ · åòôæµ
*@param Playpam: ² ¥ · å²îêý
*@Return True: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_audio_play_music(T_AMOPENAT_PLAY_PARAM*  playParam);

/**In £ Ö¹ÒÔÆµ² ¥ · å
*@return true: ³é¹¦
* FALSE: Ê§ ° ü
**/
BOOL iot_audio_stop_music(                                        
                        VOID
                  );

/**OVÍ£INoroÆ²²²²
*@return TRUE: ³É¹´
* FALSE: SE§°Ü
**/
BOOL iot_audio_pause_music(                                     
                        VOID
                   );

/**»Öçoçoæµ² ¥ · å
*@Return True: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_audio_resume_music(                                       
                        VOID
                    );

/**Éèöãñïéùæ ÷ ¾²òô
*@Return True: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_audio_mute_speaker(                                     
                        VOID
                    );

/**½âýñïéùæ ÷ ¾²òô
*@Return True: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_audio_unmute_speaker(                                   
                        VOID
                      );

/**Éèöãñïéùæ ÷ µäôee —öµµ
*@param vol: éèöãñïéùæ ÷ Òôe —öµµ
*@Return True: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_audio_set_speaker_vol(                                   
                        UINT32 vol 
                        );

/** »Of Count ÷ μärouse (μäroô How
* @Tustinturn THEINT32: · Noton to O PLäò TA'elloô TOÖl
**/
UINT32 iot_audio_get_speaker_vol(                
                        VOID
                                           );

/**Éèn »» ° µäôe —öµ
*@param vol: éèöãn »» ° Òôe —öµ
*@Return True: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_audio_set_sph_vol(                                   
                        UINT32 vol 
                        );

/**»äñè¨»°£ôôe¿ö£
*@return uint32: ·µ»°£µòeôe¿ö¡
* **/
UINT32 iot_audio_get_sph_vol(                
                        VOID
                        );

/**Éèöãòôæµn µà
*@stop Channel:
*@Return True: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_audio_set_channel(                                       
                        E_AMOPENAT_AUDIO_CHANNEL channel    
                   );

/**»Ñèmbrith
*@Return e_amopenat_audio_channel: · µ »Øn Øn Ø
**/
E_AMOPENAT_AUDIO_CHANNEL iot_audio_get_current_channel(            
                        VOID
                                               );

/**ªê
*@param param: Âmpté²îêý
*@param cb: »ñè¡àêý¾ý» Øµ ÷
*@Return True: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_audio_rec_start(
                    			E_AMOPENAT_RECORD_PARAM* param,
								AUD_RECORD_CALLBACK_T cb);

/**In £ Ö¹â¼òô
*@return true: ³é¹¦
* FALSE: Ê§ ° ü
**/
BOOL iot_audio_rec_stop();

/**÷ ² ¥ · å
*@param PLAYFORMAT: EX êýe ÷ àààn
*@param cb: êý¾ýe ÷ »Øµ ÷ ºrdý
*@param Date: Êý¾ýe ÷
*@param len: ok
*@Return> 0: ² ¥ · å³ pol
* -1: ² ¥ · åê§ ° ü
**/
int iot_audio_streamplay(E_AMOPENAT_AUD_FORMAT playformat,AUD_PLAY_CALLBACK_T cb,char* data,int len);

/** @}*/


int iot_audio_streamplayV2(E_AMOPENAT_AUD_PLAY_TYPE playtype,E_AMOPENAT_AUD_FORMAT playformat,AUD_PLAY_CALLBACK_T cb,char* data,int len);

#endif
