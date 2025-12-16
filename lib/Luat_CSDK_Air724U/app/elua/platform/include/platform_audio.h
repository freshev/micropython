/**************************************************************************
 *              Copyright (C), AirM2M Tech. Co., Ltd.
 *
 * Name:    platform_audio.h
 * Author:  liweiqiang
 * Version: V0.1
 * Date:    2013/10/21
 *
 * Description:
 *          panjun 2015.08.26 Audio API.
 **************************************************************************/

#ifndef __PLATFORM_AUDIO_H__
#define __PLATFORM_AUDIO_H__

#ifdef LUA_AUDIO_LIB

#include "type.h"

typedef enum PlatformAudioFormatTag
{
	/*+\BUG\wangyuan\2020.12.31\BUG_4041:3024LUA°æ±¾Â¼ÒôdemoÂ¼Òô¹¦ÄÜÃ»ÓÐ×÷ÓÃ*/
	/*Luaâ¼òî corner to àðn²îê #: 1: PCM 2: WAV 3: AMRNB*/
	PLATFORM_AUD_MP3, 
	PLATFORM_AUD_PCM,
	PLATFORM_AUD_WAV,
	PLATFORM_AUD_AMRNB,
	/*-\BUG\wangyuan\2020.12.31\BUG_4041:3024LUA°æ±¾Â¼ÒôdemoÂ¼Òô¹¦ÄÜÃ»ÓÐ×÷ÓÃ*/
	/*+\ New \ zhuwangbin \ 2020.05.15 \ Ôö¼óspeex¸ñê½µäâ¼òôºn² ¥ · å*/
    PLATFORM_AUD_SPEEX,
	/*-\ new \ zhuwangbin \ 2020.05.15 \ ÔÖ¼óspeex¸ñê½µäâ¼òôºn² ¥ · å*/
	PLATFORM_AUD_AMRWB,
    //PLATFORM_AUD_MIDI,
    NumOfPlatformAudFormats
}PlatformAudioFormat;

/*+\W\cmm\2020.13*/
typedef enum PlatformAudioPlayTypeTag
{
    /**
     * Placeholder for not in playing.
     */
    PLATFORM_AUD_PLAY_TYPE_NONE = 0,
    /**
     * ²¥·Å±¾µØÒôÆµÂ·¾¶¡£*/
    PLATFORM_AUD_PLAY_TYPE_LOCAL,
    /**
     * ÔÚÓÏÒÔÍ¨ »° AD² ¥ · Åµ½¶Ô¶Ë¡ £*/
    PLATFORM_AUD_PLAY_TYPE_VOICE,
    /**
     * Ôúpocä £ ê½ïâ² · · to ± ¾ µøò)*/
    PLATFORM_AUD_PLAY_TYPE_POC,
}PlatformAudioPlayType;


typedef enum PlatformAudioRecordTypeTag
{
    PLATFORM_AUD_RECORD_TYPE_NONE, // <ãónÃóаúÃö¸¸¸¸μÂ ûõ ½μ фа »من Пе»
    PLATFORM_AUD_RECORD_TYPE_MIC,     ///´ÓÂó¿Ë·çÂ¼ÖÆ¡£
    PLATFORM_AUD_RECORD_TYPE_VOICE,  ///Â¼ÖÆÓïÒôÍ¨»°¡£Â¼ÖÆµÄÁ÷ÓëÉÏÏÂÐÐÍ¨µÀ¡£
    PLATFORM_AUD_RECORD_TYPE_VOICE_DUAL,     // es → ← ¡qu e françaises ð 音 ö ê êаø â preceded for · Иålta¡ £13
    PLATFORM_AUD_RECORD_TYPE_DEBUG_DUMP, //PCM×ª´¢£¬½öÓÃÓÚµ÷ÊÔ¡£
    PLATFORM_AUD_RECORD_TYPE_POC,// ôúpocä £ ½ïâóâ · çâhattain
}PlatformAudioRecordType;

/*-\czm\2020.13*/

/*+\ New \ zhuth \ 2014.7.25 \ Đeoèèèoônæn¨n¨àººàººnònònn½½u*/
typedef enum PlatformAudioChannelTag
{
	/*+\new\wj\2020.4.22\Ö§³ÖÒôÆµÍ¨µÀÇÐ»»½Ó¿Ú*/
    PLATFORM_AUD_CHANNEL_HANDSET,
    PLATFORM_AUD_CHANNEL_EARPIECE,
    PLATFORM_AUD_CHANNEL_LOUDSPEAKER,
    NumOfPlatformAudChannels
    #if 0
    PLATFORM_AUD_CHANNEL_BLUETOOTH,
    PLATFORM_AUD_CHANNEL_FM,
    PLATFORM_AUD_CHANNEL_FM_LP,
    PLATFORM_AUD_CHANNEL_TV,
    PLATFORM_AUD_CHANNEL_AUX_HANDSET,
    PLATFORM_AUD_CHANNEL_AUX_LOUDSPEAKER,
    PLATFORM_AUD_CHANNEL_AUX_EARPIECE,
    PLATFORM_AUD_CHANNEL_DUMMY_HANDSET,    
    PLATFORM_AUD_CHANNEL_DUMMY_AUX_HANDSET,
    PLATFORM_AUD_CHANNEL_DUMMY_LOUDSPEAKER,
    PLATFORM_AUD_CHANNEL_DUMMY_AUX_LOUDSPEAKER,
	#endif
	/*-\new\wj\2020.4.22\Ö§³ÖÒôÆµÍ¨µÀÇÐ»»½Ó¿Ú*/
}PlatformAudioChannel;

typedef enum PlatformAudioVolTag
{
    PLATFORM_AUD_VOL0,
    PLATFORM_AUD_VOL1,
    PLATFORM_AUD_VOL2,
    PLATFORM_AUD_VOL3,
    PLATFORM_AUD_VOL4,
    PLATFORM_AUD_VOL5,
    PLATFORM_AUD_VOL6,
    PLATFORM_AUD_VOL7,
    NumOfPlatformAudVols
}PlatformAudioVol;

typedef enum PlatformMicVolTag
{
    PLATFORM_MIC_VOL0,
    PLATFORM_MIC_VOL1,
    PLATFORM_MIC_VOL2,
    PLATFORM_MIC_VOL3,
    PLATFORM_MIC_VOL4,
    PLATFORM_MIC_VOL5,
    PLATFORM_MIC_VOL6,
    PLATFORM_MIC_VOL7,
    PLATFORM_MIC_VOL8,
    PLATFORM_MIC_VOL9,
    PLATFORM_MIC_VOL10,
    PLATFORM_MIC_VOL11,
    PLATFORM_MIC_VOL12,
    PLATFORM_MIC_VOL13,
    PLATFORM_MIC_VOL14,
    PLATFORM_MIC_VOL15,
    NumOfPlatformMicVols
}PlatformMicVol;

typedef enum PlatformAudioLoopbackTag
{
    PLATFORM_AUD_LOOPBACK_HANDSET,
    PLATFORM_AUD_LOOPBACK_EARPIECE,
    PLATFORM_AUD_LOOPBACK_LOUDSPEAKER,
    PLATFORM_AUD_LOOPBACK_AUX_HANDSET,
    PLATFORM_AUD_LOOPBACK_AUX_LOUDSPEAKER,
    NumOfPlatformAudLoopbacks
}PlatformAudioLoopback;
/*-\ new \ zhuth \ 2014.7.25 \ Đeoèèèoônæn¨àººàºººnònònn½½u*/

/*+\new\zhuwangbin\2020.6.2\Ìn¼ÓÒôÆµ¹¦·ÅÀàÐÍÉèÖÃ½Ó¿ Ú*/
typedef enum
{
    PLATFORM_SPKPA_TYPE_CLASSAB,
    PLATFORM_INPUT_TYPE_CLASSD,
    PLATFORM_INPUT_TYPE_CLASSK,
    PLATFORM_SPKPA_INPUT_TYPE_QTY = 0xFF000000
} PlatformSpkPaType;
/*-\new\zhuwangbin\2020.6.2\Ìn¼ÓÒôÆµ¹¦·ÅÀàÐÍÉèÖÃ½Ó¿¿ Ú*/
/*+\BUG\wangyuan\2020.11.27\BUG_3634$*/
typedef enum PlatformMicChannelTag
{
    PLATFORM_AUDEV_INPUT_MAINMIC = 0, ///< main mic
    PLATFORM_AUDEV_INPUT_AUXMIC = 1,  ///< auxilary mic
    PLATFORM_AUDEV_INPUT_DUALMIC = 2, ///< dual mic
    PLATFORM_AUDEV_INPUT_HPMIC_L = 3, ///< headphone mic left
    PLATFORM_AUDEV_INPUT_HPMIC_R = 4, ///< headphone mic right
    NumOfPlatformMicChannels
}PlatformMicChannel;
/*-\BUG\wangyuan\2020.11,*/

typedef struct AudioPlayParamTag
{
    BOOL isBuffer;
    union u_tag
    {
        struct
        {
            const u8 *data;
            u32 len;
            PlatformAudioFormat format;
            BOOL loop;
        }buffer;
        const char *filename;
    }u;
}AudioPlayParam;

int platform_audio_play(AudioPlayParam *param);

int platform_audio_stop(void);

/*+\ New \ zhuth \ 2014.7.25 \ Đeoèèèoônæn¨n¨àººàººnònònn½½u*/
/*+\BUG\wangyuan\2020.11.27\BUG_3634$*/
int platform_audio_set_channel(PlatformAudioChannel outputchannel,PlatformMicChannel inputchannel);
/*-\BUG\wangyuan\2020.11,*/

int platform_audiod_set_vol(int vol);

int platform_audio_set_mic_vol(PlatformMicVol vol);

int platform_audio_set_loopback(BOOL flag, PlatformAudioLoopback typ, BOOL setvol, u32 vol);
/*-\ new \ zhuth \ 2014.7.25 \ Đeoèèèoônæn¨àººàºººnònònn½½u*/
/*+\new\wj\2020.4.26\ÊµÏÖÂ¼Òô½Ó¿Ú*/
/*+\W\cmm\2020.13*/
int platform_audio_record(char* file_name, int time_sec, int quality, PlatformAudioRecordType type, PlatformAudioFormat format);
/*-\czm\2020.13*/
/*-\new\wj\2020.4.26\ÊµÏÖÂ¼Òô½Ó¿Ú*/
int platform_audio_stop_record(void);

/*+\new\zhuwangbin\2020.6.2\Ìn¼ÓÒôÆµ¹¦·ÅÀàÐÍÉèÖÃ½Ó¿ Ú*/
int platform_setpa(PlatformSpkPaType type);
int platform_getpa(void);
/*-\new\zhuwangbin\2020.6.2\Ìn¼ÓÒôÆµ¹¦·ÅÀàÐÍÉèÖÃ½Ó¿¿ Ú*/

/*+\bug2767\zhuwangbin\2020.8.5\Ìn¼ÓÍâ²¿ paÉèÖÃ½Ó¿ Ú*/
int platform_setexpa(BOOL enable, UINT16 gpio, UINT16 count, 
					UINT16 us,  E_AMOPENAT_AUDIO_CHANNEL outDev);
/*-\bug2767\zhuwangbin\2020.8.5\Ìn¼ÓÍâ²¿½ paÉèÖÃ½Ó¿ Ú*/

/*+\ New \ zhuwangbin \ 2020.8.11 \ \*/
int platform_headPlug(int type);
/*-\ new \ zhuwangbin \ 2020.8.11 \ \*/

#endif

#endif //__PLATFORM_AUDIO_H__

