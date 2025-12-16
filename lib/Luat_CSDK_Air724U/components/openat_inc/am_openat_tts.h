#ifndef _AM_OPENAT_TTS_H_
#define _AM_OPENAT_TTS_H_
/*+\new\wj\2019.12.27\Ìn¼ÓTTS¹¦ÄÜ*/

typedef enum 
{
	tts_result_ok,
	tts_result_error,

}TTSResultCode;

typedef enum 
{
	OPENAT_TTS_PARAM_SPEED, /*²¥·ÅËÙ¶È -32768 to +32767*/
	OPENAT_TTS_PARAM_VOLUME, /*² ¥ · å å ²²î î ² ê ê*/
	OPENAT_TTS_PARAM_PITCH, /*² ¥ · Åeùµ ÷ ²îêý -32768 to +32767*/
	OPENAT_TTS_PARAM_CODEPAGE,/*ÎÄ±¾ÀàÐÍ*/
}OPENAT_TTS_PARAM_FLAG;

typedef enum 
{
	OPENAT_CODEPAGE_ASCII,
	OPENAT_CODEPAGE_GBK,
	OPENAT_CODEPAGE_BIG5,
	OPENAT_CODEPAGE_UTF16LE, // UTF-16 £ ¬ð¡n ·
	OPENAT_CODEPAGE_UTF16BE,  //UTF-16£¬´óÍ·
	OPENAT_CODEPAGE_UTF8, //UTF-8
	OPENAT_CODEPAGE_UTF16, 
	OPENAT_CODEPAGE_UNICODE,
	OPENAT_CODEPAGE_PHONETIC_PLAIN, // Phonetic plain ò ô ± ± ± â ë
}TTS_CODEPAGE_PARAM;

typedef enum 
{
	FIRST_START,
	PLAYING,
	STOP
}TTSStatus;//tts ÔËÐÐ×´Ì¬

typedef enum 
{	
	OPENAT_TTS_PLAY_OPREATION,	/*² · · to²ù × ÷*/
	OPENAT_TTS_FINISH_OPRETION,	/*²¥·Å½eÊø²Ù×÷*/
	OPENAT_TTS_STOP_OPREATION,	/*In £ Ö¹² ¥ · Å²Ù × ÷*/

}OPENAT_TTS_OPERATION;


typedef struct
{
	int speed;
	int volume;
	int pitch;
	u32 codepage;
}TTS_PARAM_STRUCT;

typedef struct
{
	char *text;
	int size;
}TTS_PLAY_PARAM;

typedef enum 
{
	OPENAT_TTS_CB_MSG_ERROR,
	OPENAT_TTS_CB_MSG_STATUS	

}OPENAT_TTS_CB_MSG;

typedef void (*TTS_PLAY_CB)(OPENAT_TTS_CB_MSG msg_id,u8 event);	/*²¥·Å½eÊø»Øµ÷º¯Êý*/


BOOL OPENAT_tts_init(TTS_PLAY_CB fCb);
BOOL OPENAT_tts_set_param(OPENAT_TTS_PARAM_FLAG flag,u32 value);
BOOL OPENAT_tts_play(char *text,u32 len);
BOOL OPENAT_tts_stop();





/*-\new\wj\2019.12.27\Ìn¼ÓTTS¹¦ÄÜ*/
#endif
