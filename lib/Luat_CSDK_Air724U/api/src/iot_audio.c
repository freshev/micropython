#include "iot_audio.h"

/****************************** AUDIO ******************************/
/**´ò °
*@Note ôún »» ° ªrse ± µ ÷ Oã
*@Return True: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_auido_open_tch(                                           
                        VOID
                )
{
    return IVTBL(open_tch)();
}

/**¹Ø ± õóïòô
*@Note Í »° ½eêøê ± µ ÷ Ó
*@Return True: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_audio_close_tch(                                          
                        VOID
                 )
{
    return IVTBL(close_tch)();
}

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
                 )
{
    return IVTBL(play_tone)(toneType, duration, volume);
}

/**In £ Ö¹² ¥ · ÅtoneÒÔ
*@return true: ³é¹¦
* FALSE: Ê§ ° ü
**/
BOOL iot_audio_stop_tone(                                         
                        VOID
                 )
{
    return IVTBL(stop_tone)();
}


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
                 )
{
    return IVTBL(play_dtmf)(dtmfType, duration, volume);
}

/**In £ Ö¹² ¥ · ÅDTMFÒÔ
*@return true: ³é¹¦
* FALSE: Ê§ ° ü
**/
BOOL iot_audio_stop_dtmf(                                          
                        VOID
                 )
{
    return IVTBL(stop_dtmf)();
}

/**² ¥ · åòôæµ
*@param Playpam: ² ¥ · å²îêý
*@Return True: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_audio_play_music(T_AMOPENAT_PLAY_PARAM*  playParam)
{
    return IVTBL(play_music)(playParam);
}

/**In £ Ö¹ÒÔÆµ² ¥ · å
*@return true: ³é¹¦
* FALSE: Ê§ ° ü
**/
BOOL iot_audio_stop_music(                                       
                        VOID
                  )
{
    return IVTBL(stop_music)();
}

/**OVÍ£INoroÆ²²²²
*@return TRUE: ³É¹´
* FALSE: SE§°Ü
**/
BOOL iot_audio_pause_music(                                       
                        VOID
                   )
{
    return IVTBL(pause_music)();
}

/**»Öçoçoæµ² ¥ · å
*@Return True: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_audio_resume_music(                                     
                        VOID
                    )
{
    return IVTBL(resume_music)();
}

/**Éèöãñïéùæ ÷ ¾²òô
*@Return True: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_audio_mute_speaker(                                       
                        VOID
                    )
{
    return IVTBL(mute_speaker)();
}

/**½âýñïéùæ ÷ ¾²òô
*@Return True: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_audio_unmute_speaker(                                    
                        VOID
                      )
{
    return IVTBL(unmute_speaker)();
}

/**Éèöãñïéùæ ÷ µäôee —öµµ
*@param vol: éèöãñïéùæ ÷ Òôe —öµµ
*@Return True: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_audio_set_speaker_vol(                                   
                        UINT32 vol 
                        )
{
    return IVTBL(set_music_volume)(vol);
}

/** »Of Venòôron ÷ μäòône
* @Tustinturn THEINT32: · Noton to O PLäò TA'elloô TOÖl
**/
UINT32 iot_audio_get_speaker_vol(                
                        VOID
                                           )
{
    return IVTBL(get_music_volume)();
}

/**Éèn »» ° µäôe —öµ
*@param vol: éèöãn »» ° Òôe —öµ
*@Return True: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_audio_set_sph_vol(                                   
                        UINT32 vol 
                        )
{
    return IVTBL(set_sph_vol)(vol);
}

/**»äñè¨»°£ôôe¿ö£
*@return uint32: ·µ»°£µòeôe¿ö¡
* **/
UINT32 iot_audio_get_sph_vol(                
                        VOID
                        )
{
    return IVTBL(get_sph_vol)();
}


/**Éèöãòôæµn µà
*@stop Channel:
*@Return True: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_audio_set_channel(                                        
                        E_AMOPENAT_AUDIO_CHANNEL channel   
                   )
{
    return IVTBL(set_channel)(channel,OPENAT_AUDEV_INPUT_MAINMIC);
}

/**»Ñèmbrith
*@Return e_amopenat_audio_channel: · µ »Øn Øn Ø
**/
E_AMOPENAT_AUDIO_CHANNEL iot_audio_get_current_channel(            
                        VOID
                                               )

{
    return IVTBL(get_current_channel)();
}

/**ªê
*@param param: Âmpté²îêý
*@param cb: »ñè¡àêý¾ý» Øµ ÷
*@Return True: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_audio_rec_start(
                    			E_AMOPENAT_RECORD_PARAM* param,
								AUD_RECORD_CALLBACK_T cb)
{
    return ((IVTBL(audio_record)(param, cb) == 0) ? 1 : 0);
}

/**In £ Ö¹â¼òô
*@return true: ³é¹¦
* FALSE: Ê§ ° ü
**/
BOOL iot_audio_rec_stop()
{
    return ((IVTBL(audio_stop_record)() == 0) ? 1 : 0);
}

/**÷ ² ¥ · å
*@param PLAYFORMAT: EX êýe ÷ àààn
*@param cb: êý¾ýe ÷ »Øµ ÷ ºrdý
*@param Date: Êý¾ýe ÷
*@param len: ok
*@Return> 0: ² ¥ · å³ pol
* -1: ² ¥ · åê§ ° ü
**/
int iot_audio_streamplay(E_AMOPENAT_AUD_FORMAT playformat,AUD_PLAY_CALLBACK_T cb,char* data,int len)
{
	return IVTBL(streamplay)(playformat,cb,data,len);
}


/**÷ ² ¥ · åv2
*@param PLAYFORMAT: EX êýe ÷ àààn
*@param cb: êý¾ýe ÷ »Øµ ÷ ºrdý
*@param Date: Êý¾ýe ÷
*@param len: ok
*@Return> 0: ² ¥ · å³ pol
* -1: ² ¥ · åê§ ° ü
**/
int iot_audio_streamplayV2(E_AMOPENAT_AUD_PLAY_TYPE playtype,E_AMOPENAT_AUD_FORMAT playformat,AUD_PLAY_CALLBACK_T cb,char* data,int len)
{
	return IVTBL(streamplayV2)(playtype,playformat,cb,data,len);
}

