/*,
  Copyright (c), airm2m tech. Co., Ltd.
  Author: Lifei
  Description: Amopenat
  Others:
  History: 
    Version £ º Date: Author: modification:
    V0.1 2012.12.14 Lifei
	V0.2 2012.12.26 Brezen ìnomic
	V0.3 2012.12.29 Brezen ìnoubished
	V0.4 2013.01.08 Brezen ð¸äspi½ó¿
	V0.5 2o13.01.14 Brezen 1¡ ¢ ô ôóºú ° × æe³õÊ »
	                                2¡ ¢ ô öóºude ° × æeçåartment
									3¡ ¢ õéõéùve ° × æ æ æ æ ¼ ¼ ¼ ¼ ¼ £ £ £ £ £ × × × × ã ¥ ¶Êload
    V0.6 2013.01.14 Brezen ð¸äimate ° × æeçeiddagerúó úÊý
	V0.7 2013.01.15 Brezen ð¸äimate
    V0.8 2013.01.17 Brezen 1¡ ¢ ¢ ¢ ¢ ó ó ó ³ ç ody øöæ½– 2¡ ¢ ¢ ì ó ó óuge
    V0.9 2013.01.23 Brezen ð¸äspi>  
    V1.0 2013.01.28 Brezen ìnouben
    V1.1 2013.01.30 Brezen ð¸¸ä pharyon_systemº®round
    V1.2 2013.02.06 jack.li ìnony
    V1.3 2013.02.07 jack.li ìnony
    V1.4 2013.02.10 jack.li
    V1.5 2013.02.26 Brezen ìnóenter_deepsleep/exit_deepsleep½ó¿
    V1.6 2013.03.21 Maliang îïãµïïï½ó¿²ºnvi ¥ · åòôôîîîîääator
    V1.7 2013.04.03 jack.li ôói2c½ó¿
    V1.8 2013.05.15 xc ôöótts½ó¿
	V1.9 2013.07.18 Brezen ìnony_dte_at_filter½ó¿
	V2.0 2013.07.22 Brezen ìnёdata_to_dte send_data_to_ci
	V2.1 2013.08.20 Brezen õëe ô¶¶¤Ê§ ° ü £ ¬ ¬ ¬ ¬Rw_psamìnó ó · · · ¢ ¢ ën² yoursopclock
	V2.2.2 2013.09.16 Brezen ìnóflush_file½ó¿ £ £ £ µçµ®ç ° ç ç ç ç ç ç {çððð ç ç ° ° °
	V2.3 2013.09.24 break
	V2.4 2013.09.26 Brezen ö³une³åpsam
	V2.5 2013.12.30 Brezen ìn due
	V2.6 2014.6.26 Brezen ìnóà )sp½ó¿
	V2.7 2015.04.21 panjun ôóoled ± ± ± ¢ ¢ spi De
	V2.8 2015.05.20 Panjun Export Two Values   'Lua_Lcd_height' and 'Lua_Lcd_Width'.
	V2.9 2015.06.22 panjun optimize mechanism of lua's timer.
	V3.0 2015.07.04 Panjun define a macro 'Openat_max_timer_id', Increase Quantity of Openat's Timer.
	V3.1 2016.03.26 panjun add ttsply's api.
	V3.2 2016.05.03 panjun add an api "rtos.sysdisk_free".
,*/
#ifndef AM_OPENAT_H
#define AM_OPENAT_H

//#include "utils.h"
#include "am_openat_system.h"
#include "am_openat_fs.h"
#include "am_openat_drv.h"
#include "am_openat_vat.h"
#include "am_openat_tts.h"
#include "am_openat_image.h"
/*-\NEW\zhuwangbin\2020.3.25\Ìn¼ÓjpgÎÄ¼þµÄ½âÂëºÍÏÔÊ¾*/
/*+\NEW\WZQ\2014.11.7\¼ÓÈëSSL RSA¹¦ÄÜ*/
#ifdef AM_OPENAT_SSL_RSA_SUPPORT
#include "openat_SSLRSA.h"
#endif

#include "am_openat_zbar.h"
#include "openat_camera.h"
/*-\NEW\WZQ\2014.11.7\¼ÓÈëSSL RSA¹¦ÄÜ*/
/*+\NEW\WJ\2019.1.8\Ìn¼ÓÖ¤ÊéÓÐÐ§Ê±¼äÐ£Ñé*/
typedef struct {
    int     sec;         /* seconds */
    int     min;         /* minutes */
    int     hour;        /* hours */
    int     day;      
    int     mon;         /* month */
    int     year;        /* year */
}t_time;
/*-\NEW\WJ\2019.1.8\Ìn¼ÓÖ¤ÊéÓÐÐ§Ê±¼äÐ£Ñé*/

#undef IVTBL
#define IVTBL(func) OPENAT_##func

#define PUB_TRACE(pFormat, ...)  IVTBL(print)(pFormat, ##__VA_ARGS__)

//#define  NUCLEUS_TIMER_MECHANISM_ENABLE
#define  OPENAT_MAX_TIMERS 50
#define  OPENAT_MAX_TIMER_ID 60000
#define  LUA_TIMER_BASE LUA_APP_TIMER_0
#define  LUA_GIF_TIMER_BASE LUA_GIF_TIMER_0

/*\+NEW\yezhishe\2018.11.23\Ìn¼ÓGPIO8,9,10,11\*/
#define GPIO8_R28 8
#define GPIO9_R27 9
#define GPIO10_R26 10
#define GPIO11_R17 11
/*\-NEW\yezhishe\2018.11.23\Ìn¼ÓGPIO8,9,10,11\*/

/*+\NEW\wangyuan\2020.05.07\BUG_1126:Ö§³Öwifi¶èÎ»¹¦ÄÜ*/
typedef struct
{
	UINT32 bssid_low;  	//< mac address low
	UINT16 bssid_high; 	//< mac address high
	UINT8 channel;	 	//< channel id
	signed char rssival; 	 	//< signal strength
} OPENAT_wifiApInfo;

typedef struct
{
	UINT32 max;		 ///< set by caller, must room count for store aps
	UINT32 found; 	 ///< set by wifi, actual found ap count
	UINT32 maxtimeout; ///< max timeout in milliseconds for each channel
	OPENAT_wifiApInfo *aps;	 ///< room for aps
} OPENAT_wifiScanRequest;
/*-\NEW\wangyuan\2020.05.07\BUG_1126:Ö§³Öwifi¨Î»¹¦ÄÜ*/

typedef void (*openat_wifi_info_cb)(OPENAT_wifiScanRequest* req);

    /*******************************************
    **                 SYSTEM                 **
    *******************************************/
BOOL OPENAT_create_task(                          /*´´½¨Ïß³Ì½Ó¿Ú*/
                            HANDLE* handlePtr,
                            PTASK_MAIN pTaskEntry,  /*Ïß³ìö ÷ º¯êý*/
                            PVOID pParameter,       /*× ÷ª²îêā'' «μý away« μ º ¢ ¡¢¯êý*/
                            PVOID pStackAddr,       /*Ïß³ìõ »µøÖ · £ ¬µ ± Ç ° ²» Ö§³Ö £ ¬ÇË´ «ÈËNullull*/
                            UINT32 nStackSize,      /*Ïß³ìõ »´óð¡*/
                            UINT8 nPriority,        /*Ïß³ó / Resources of £ ¬¸ãxogons Teas 10- Descholy*/
                            UINT16 nCreationFlags,  /*Ïß³ìinary ± ë ê vers £ ¬ ¬ ¬*/
                            UINT16 nTimeSlice,      /*ÔýÊ ± ² »Ö§³Ö £ ¬çË´« ÈË0*/
                            PCHAR pTaskName         /*Lique stone*/
                          );
VOID OPENAT_delete_task(HANDLE         task);

HANDLE OPENAT_current_task(                         /*»± ± ° ° °ß ï³*/
                            VOID
                          );
/*+\BUG\wangyuan\2020.30.30.30*/
BOOL OPENAT_suspend_task(                           /*¹¹æðïß³³½ó¿u*/
    HANDLE hTask            /*Ss³ìense*/
);

BOOL OPENAT_resume_task(                            /*»Ö¸´Ïß³Ì½Ó¿Ú*/
    HANDLE hTask            /*Ss³ìense*/
);
/*-\BUG\wangyuan\2020.30.30.30*/
BOOL OPENAT_get_task_info(                          /*»± ± ç ° ïß®½ ¢ ¢ ¢ ¢ ¢*/
                            HANDLE hTask,           /*Ss³ìense*/
                            T_AMOPENAT_TASK_INFO *pTaskInfo /*Ï³ ¢ ´ ´æaking*/
                         );



    /*****************Û³ vi ïóað ¶óað ¶óeð½óeð ¶óeð½óeð½½³½**/
BOOL OPENAT_wait_message(
                                     HANDLE   task,
                                     int* msg_id,
                                     void* * ppMessage,
                                     UINT32 nTimeOut
                                     );



BOOL OPENAT_free_message(
                                     void* message_body
                                     );

    
BOOL OPENAT_send_message(                           /*· _ Ó v½ón "½ ½ón £ ¬ìn ¢ ¶¶¶û ¢ ¶óeðî e²² what you*/
                                      HANDLE   destTask,
                                      int msg_id,
                                      void* pMessage,          /*'Æ ¢ ïû¸õ¢¢ ¢ ¸õ¸õ¸õ*/
                                      int message_length);

/*+\ Task \ Wangyuan \ 2020.06.28 \ Task_255: Ö§³ööðôödå ° ², ¼æèý2G CSDKµä½ó¿úìn¼ó*/
BOOL OPENAT_SendHighPriorityMessage(			   /*·¢ËÍ¸ßÓÅÏÈ¼¶ÏûÏ¢½Ó¿Ú£¬Ìn¼Óµ½ÏûÏ¢¶ÓÁÐÍ·²¿*/
											  HANDLE   destTask,
											  int msg_id,
											  void* pMessage,		   /*'Æ ¢ ïû¸õ¢¢ ¢ ¸õ¸õ¸õ*/
											  int message_length);

/*-\ Task \ WangYuan \ 2020.06.28 \ Task_255: Ö§³ööðôödå ° ², ¼æèý2G CSDKµä½ó¿úìn¼ó*/


BOOL OPENAT_available_message(                      /*¼ ¼ ¢ ¢ ¢ ¢ · That · that*/
                            HANDLE hTask            /*Ss³ìense*/
                             );

/******************************************************
   ÏòOPENAT TASK·¢ÏûÏ¢µÄ½Ó¿Ú
*******************************************************/
BOOL OPENAT_send_internal_message(                           /*· _ Ó v½ón "½ ½ón £ ¬ìn ¢ ¶¶¶û ¢ ¶óeðî e²² what you*/
                                      int msg_id,
                                      void* pMessage,          /*'Æ ¢ ïû¸õ¢¢ ¢ ¸õ¸õ¸õ*/
                                      int message_length
                                      );

/****************************** Ê±¼ä&¶¨Ê±Æ÷½Ó¿Ú ******************************/
HANDLE OPENAT_create_timer(                         /*´´½¨¶¨Ê±Æ÷½Ó¿Ú*/
                            PTIMER_EXPFUNC pFunc,   /*¶¨Ê±Æ÷µ½Ê±´¦Ànº¯Êý*/
                            PVOID pParameter        /*×²ªîā' «μý¸øîø ± ± ÷ μ il il ± 'byene''|eNo'āÝêý*/
                          );

/*+\ENEW administrative Putbinng \2020.2.1.1.2*/
HANDLE OPENAT_create_timerTask(                         /*´´½¨¶¨Ê±Æ÷½Ó¿Ú*/
                            PTIMER_EXPFUNC pFunc,   /*¶¨Ê±Æ÷µ½Ê±´¦Ànº¯Êý*/
                            PVOID pParameter        /*×²ªîā' «μý¸øîø ± ± ÷ μ il il ± 'byene''|eNo'āÝêý*/
                          );

/*-\ NENTIjn administrbin \2020.2.12.12.½½½ther½½½´O©´½´º´½º MEO*/
HANDLE OPENAT_create_hir_timer(						/*´´½¨¶¨Ê±Æ÷½Ó¿Ú*/
						PTIMER_EXPFUNC pFunc,	/*¶¨Ê±Æ÷µ½Ê±´¦Ànº¯Êý*/
						PVOID pParameter		/*×²ªîā' «μý¸øîø ± ± ÷ μ il il ± 'byene''|eNo'āÝêý*/
					  );


BOOL OPENAT_start_timer(                            /*Æô¶¯¶¨Ê±Æ÷½Ó¿Ú*/
                            HANDLE hTimer,          /*¶¨Ê ± Æ ÷ ¾Ä ± Ú £ ¬CREATE_TIMER½Ó¿Ú · µ »øöµ*/
                            UINT32 nMillisecondes   /*¶¨Ê±Æ÷Ê±¼ä*/
                       );

BOOL OPENAT_loop_start_timer(                            /*Æô¶¯Ñ­»·¶¨Ê±Æ÷½Ó¿Ú*/
                            HANDLE hTimer,          /*¶¨Ê ± Æ ÷ ¾Ä ± Ú £ ¬CREATE_TIMER½Ó¿Ú · µ »øöµ*/
                            UINT32 nMillisecondes   /*¶¨Ê±Æ÷Ê±¼ä*/
                       );

BOOL OPENAT_start_precise_timer(                            /*Æô¶¯¶¨Ê±Æ÷½Ó¿Ú*/
                            HANDLE hTimer,          /*¶¨Ê ± Æ ÷ ¾Ä ± Ú £ ¬CREATE_TIMER½Ó¿Ú · µ »øöµ*/
                            UINT32 nMillisecondes   /*¶¨Ê±Æ÷Ê±¼ä*/
                       );

BOOL OPENAT_stop_timer(                             /*In £ Ö¹¶¨Ê ± Æ ÷ ½Ó¿ú*/
                            HANDLE hTimer           /*¶¨Ê ± Æ ÷ ¾Ä ± Ú £ ¬CREATE_TIMER½Ó¿Ú · µ »øöµ*/
                      );

UINT64 OPENAT_timer_remaining(
							HANDLE hTimer
						);

 BOOL OPENAT_play_gif(
                                             char* buff,
                                            UINT8* gif_buf, 
                                            int length,
                                            int x, 
                                            int y, 
                                            int times
                                            );



BOOL OPENAT_stop_gif(void);

VOID OPENAT_lcd_sleep(BOOL);

BOOL OPENAT_delete_timer(                           /*É¾³ý¶¨Ê±Æ÷½Ó¿Ú*/
                            HANDLE hTimer           /*¶¨Ê ± Æ ÷ ¾Ä ± Ú £ ¬CREATE_TIMER½Ó¿Ú · µ »øöµ*/
                        );



BOOL OPENAT_available_timer(                        /*¼ì²é¶¨Ê±Æ÷ÊÇ·ñÒÑ¾­Æô¶¯½Ó¿Ú*/
                            HANDLE hTimer           /*¶¨Ê ± Æ ÷ ¾Ä ± Ú £ ¬CREATE_TIMER½Ó¿Ú · µ »øöµ*/
                           );



BOOL OPENAT_get_minute_tick(                        /* minute indication infterface */
                            PMINUTE_TICKFUNC pFunc  /* if pFunc != NULL, one MINUTE interval timer will be started. else the timer will be stop */
                           );

UINT32 OPENAT_get_timestamp(void);
BOOL OPENAT_gmtime(UINT32 timestamp, T_AMOPENAT_SYSTEM_DATETIME* pDatetime);


BOOL OPENAT_get_system_datetime(                    /*»Ñłna ± ¼ loub*/
                            T_AMOPENAT_SYSTEM_DATETIME* pDatetime/*´æ´¢Ê±¼äÖ¸Õë*/
                           );



BOOL OPENAT_set_system_datetime(                    /*Big éThough ± ±¿½ú*/
                            T_AMOPENAT_SYSTEM_DATETIME* pDatetime/*´æ´¢Ê±¼äÖ¸Õë*/
                           );
/*+\ New \ wanggyuan \ 2020.05.08 \ ìn¼óéèöãºn »ñè¡µ ± ç ° ê ± çøµä½ó¿ú*/
void OPENAT_Set_TimeZone(INT32 timeZone);

INT8 OPENAT_get_TimeZone(VOID);
/*-\ New \ WangYuan \ 2020.05.08 \ ìn¼óéèöãºn »ñè¡µ ± ç ° ê ± çøµä½ó¿ú*/

/****************************** ALARM½Ó¿Ú ******************************/
/*+\BUG\wangyuan\2020.04.04.30\BUG_1757:Air724ïººº»Ç°Ã»úh´âlarm demo*/
BOOL OPENAT_InitAlarm(                                        /*L*/
                            T_AMOPENAT_ALARM_CONFIG *pConfig   /*Ánåäî î î î î î*/
                       ); 



BOOL OPENAT_SetAlarm(                                        /*Äööóéèöã/É¾³ý½o*/
                            T_AMOPENAT_ALARM_PARAM *pAlarmSet    /*Others, ITO*/
                       );

/*-\BUG\wangyuan\2020.04.30\BUG_1757:Air724ïººº»Ç°Ã»ólarm demo*/

/****************************** ÁÙ½ç×ÊÔ´½Ó¿Ú ******************************/
HANDLE OPENAT_enter_critical_section(               /*½øÈëÁÙ½ç×ÊÔ´Çø½Ó¿Ú£¬¹Ø±ÕËùÓÐÖÐ¶Ï*/
                            VOID
                                    );


VOID OPENAT_exit_critical_section(                  /*Íë³eÙUUAÙU × Â½½ £ ¬ ¬ææð¶¶*/
                            HANDLE hSection         /*Á½½AIAIç × ± ú ± ± ± `± £ ЬCritical_section½.SUPLIES ULL · eentμ*/
                                 );



/********************************************/
HANDLE OPENAT_create_semaphore(                     /*´´ request*/
                            UINT32 nInitCount       /*Captain*/
                              );



BOOL OPENAT_delete_semaphore(                       /*É¾³ýðåºåe¿½ó¿ú*/
                            HANDLE hSem             /*Ðå ºå ¾ä ± ± `± £ ¬craper_semaphorelore ú μ» Øöμ*/
                            );



BOOL OPENAT_wait_semaphore(                         /*»Disclosure*/
                            HANDLE hSem,            /*Ðå ºå ¾ä ± ± `± £ ¬craper_semaphorelore ú μ» Øöμ*/
                            UINT32 nTimeOut         /*Ä¿ç ° ² »*/
                          );



BOOL OPENAT_release_semaphore(
                            HANDLE hSem             /*Ðå ºå ¾ä ± ± `± £ ¬craper_semaphorelore ú μ» Øöμ*/
                             );



UINT32 OPENAT_get_semaphore_value(                   /*»Ñè¡ïûºäe¿öµ*/
                            HANDLE hSem             /*Ðå ºå ¾ä ± ± `± £ ¬craper_semaphorelore ú μ» Øöμ*/  
                            );



/****************************** ÄÚ´æ½Ó¿Ú ******************************/

#define OPENAT_malloc(size) OPENAT_malloc1(size, (char*)__FUNCTION__,(UINT32) __LINE__)
#define OPENAT_calloc(cnt,size) OPENAT_malloc1(cnt*size, (char*)__FUNCTION__,(UINT32) __LINE__)
PVOID OPENAT_malloc1(                                /*ÄÚ´æÉêÇë½Ó¿Ú*/
                            UINT32 nSize,            /*ÉêÇëµÄÄÚ´æ´óÐ¡*/
							/*+\NEW \zhuwangbin\2020.02.3\ÐÞ¸Äwarning*/
                            const char*  func,
							/*-\NEW \zhuwangbin\2020.02.3\ÐÞ¸Äwarning*/
                            UINT32 line
                   );



PVOID OPENAT_realloc(                               /**/
                            PVOID pMemory,          /*ÄÚ´ÆÖ¸õË £ ¬Malloc½Ó¿Ú · µ »øöµ*/
                            UINT32 nSize            /*ÉêÇëµÄÄÚ´æ´óÐ¡*/
                    );



VOID OPENAT_free(                                   /*ÄÚ´æÊÍ·Å½Ó¿Ú*/
                            PVOID pMemory           /*ÄÚ´ÆÖ¸õË £ ¬Malloc½Ó¿Ú · µ »øöµ*/
                );


/*+\bug2307\zhuwangbin\2020.06.20\Ìn¼ÓOPENAT_MemoryUsed½Ó¿ Ú*/
VOID OPENAT_MemoryUsed(UINT32* total, UINT32* used); /*»Ñè¡¿éóÃäú'æÊ¹óÃçÉ¿ö*/
/*-\bug2307\zhuwangbin\2020.06.20\Ìn¼ÓOPENAT_MemoryUsed½Ó¿ Ú*/
/****************************** ÔÓÏî½Ó¿Ú ******************************/
BOOL OPENAT_sleep(                                  /*Ïµn³ë thousand*/
                            UINT32 nMillisecondes   /*Ë¯ÃßÊ±¼ä*/
                 );

BOOL OPENAT_Delayms(                                  /*ÑÓÊ±½Ó¿Ú*/
                            UINT32 nMillisecondes   /*ÑÓÊ±Ê±¼ä*/
                 );

INT64 OPENAT_get_system_tick(                      /*»Ñ Iwhiwick nerve.*/
                            VOID
                             );



UINT32 OPENAT_rand(                                 /*»Ñ Turnènot» otalÙý view of*/
                            VOID
                  );



VOID OPENAT_srand(                                  /*Ureans tan) courses and garöö ó½ó ó½óder*/
                            UINT32 seed             /*Ë well »ú-n️ by*/
                 );



VOID OPENAT_shut_down(                              /*Ø*/
                            VOID
                     );



VOID OPENAT_restart(                                /*ÖØÆô½Ó¿Ú*/
                            VOID
                   );



/*+\NEW\liweiqiang\2013.7.1\[OpenAt]Ôö¼ÓÏµÍ³Ö÷ÆµÉèÖÃ½Ó¿Ú*/
VOID OPENAT_sys_request_freq(                       /*Ö÷Æµ¿ØÖÆ½Ó¿Ú*/
                            E_AMOPENAT_SYS_FREQ freq/*Ö÷ÆµÖµ*/
                   );



UINT16 OPENAT_unicode_to_ascii(UINT8 *pOutBuffer, WCHAR *pInBuffer);


UINT16 OPENAT_ascii_to_unicode(WCHAR *pOutBuffer, UINT8 *pInBuffer);

    
/*-\NEW\liweiqiang\2013.7.1\[OpenAt]Ôö¼ÓÏµÍ³Ö÷ÆµÉèÖÃ½Ó¿Ú*/
    /*******************************************
    **              FILE SYSTEM               **
    *******************************************/
INT32 OPENAT_open_file(                             /*Ãуoî'nerîä¼Þ½*//* Õý³£¾ä±ú·µ»ØÖµ´Ó0¿ªÊ¼£¬Ð¡ÓÚ0´íÎó·¢Éú */
/*+\ Bug wM-719 \ Maliang \ 2013.3.21 \ îä¼þïµn³½ó¿úºn² ¥ · ÅÒÔÆµÎÄ¼Þ½Ó¿ÚµÄÎÄ¼þÃÛ¸ÄÎªunicode Little Endingààðn*/
                            char* pszFileNameUniLe,/*ÎÄ¼þÈ«Â·¾¶Ãû³Æ unicode little endian*/
                            UINT32 iFlag,           /*´ò¿ª±êÖ¾*/
	                        UINT32 iAttr            /*ÎÄ¼þÊôÐÔ£¬ÔÝÊ±²»Ö§³Ö£¬ÇëÌîÈë0*/
                      );



INT32 OPENAT_close_file(                            /*¹Ø±ÕÎÄ¼þ½Ó¿Ú*/
                            INT32 iFd               /*Îä spill ± ± ± ± ± ± ai ±ope_file »» Ø в »Ø Øäóðuraððuish faceý*/
                       );



INT32 OPENAT_read_file(                             /*¶ÁÈ¡ÎÄ¼þ½Ó¿Ú*/
                            INT32 iFd,              /*Îä spill ± ± ± ± ± ± ai ±ope_file »» Ø в »Ø Øäóðuraððuish faceý*/
                            UINT8 *pBuf,            /*Êý¾Ý±£´æÖ¸Õë*/
                            UINT32 iLen             /*buf³¤¶È*/
                      );



INT32 OPENAT_write_file(                            /*Ð´ÈëÎÄ¼þ½Ó¿Ú*/
                            INT32 iFd,              /*Îä spill ± ± ± ± ± ± ai ±ope_file »» Ø в »Ø Øäóðuraððuish faceý*/
                            UINT8 *pBuf,            /*`` `` `` Ome ''*/
                            UINT32 iLen             /*Êý¾Ý³¤¶È*/
                       );



INT32 OPENAT_flush_file(                            /*On ¢ ¼´ð´èëflash*/
                            INT32 iFd               /*Îä spill ± ± ± ± ± ± ai ±ope_file »» Ø в »Ø Øäóðuraððuish faceý*/
                       );    



INT32 OPENAT_seek_file(                             /*ÎÄ¼þ¶¨Î»½Ó¿Ú*/
                            INT32 iFd,              /*Îä spill ± ± ± ± ± ± ai ±ope_file »» Ø в »Ø Øäóðuraððuish faceý*/
                            INT32 iOffset,          /*Ah «Òæe¿*/
                            UINT8 iOrigin           /*Æ «Òææðê¼î» ÖÃ*/
                      );

INT32 OPENAT_tell_file(                             /*ÎÄ¼þ¶¨Î»½Ó¿Ú*/
                            INT32 iFd              /*Îä spill ± ± ± ± ± ± ai ±ope_file »» Ø в »Ø Øäóðuraððuish faceý*/
                      );

INT32 OPENAT_rename_file(char* name, char* new);

INT32 OPENAT_create_file(                           /*´´½¨ÎÄ¼þ½Ó¿Ú*/
                            char* pszFileNameUniLe,/*ÎÄ¼þÈ«Â·¾¶Ãû³Æ unicode little endian*/
                            UINT32 iAttr            /*ÎÄ¼þÊôÐÔ£¬ÔÝÊ±²»Ö§³Ö£¬ÇëÌîÈë0*/
                        );
UINT32 OPENAT_get_file_size(char* pszFileNameUniLe);
UINT32 OPENAT_get_file_size_h(int handle);


INT32 OPENAT_delete_file(                           /*É¾³ýÎÄ¼þ½Ó¿Ú*/
                            char* pszFileNameUniLe/*ÎÄ¼þÈ«Â·¾¶Ãû³Æ unicode little endian*/
                        );



INT32 OPENAT_change_dir(                            /*Çđ »µ ± ç ° ° ° × ÷ ä¿ ä¿â ½ ¿ú¿ I*/
                            char* pszDirNameUniLe  /*Ä¿Â¼Â·¾¶ unicode little endian*/
                       );



INT32 OPENAT_make_dir(                              /*´´½¨Ä¿Â¼½Ó¿Ú*/
                            char* pszDirNameUniLe, /*Ä¿Â¼Â·¾¶ unicode little endian*/
                            UINT32 iMode            /*Ä¿Â¼ÊôÐÔ£¬ÏêÏ¸Çë²Î¼û E_AMOPENAT_FILE_ATTR*/
                     );



INT32 OPENAT_remove_dir(                            /*É¾³ ¿ââ¿ ½ ¿ú¿ú¿ I*//*¸а¯ ± Ø1îî õ о½žoä²ä grazzo|*/
                            char* pszDirNameUniLe  /*Ä¿Â¼Â·¾¶ unicode little endian*/
                       );



INT32 OPENAT_remove_dir_rec(                        /*µéé¾³ ¿â â â ¿úú¿ú¿u*//*Socâäuräururg ,ëiiiii in Yali LOBI LOBI LOBIA LOBALL OR LO LOBALLE TO LO LO LO LO LOBI*/
                            char* pszDirNameUniLe  /*Ä¿Â¼Â·¾¶ unicode little endian*/
                           );
                           

INT32 OPENAT_remove_file_rec(                        /*µÝ¾³Ýîä¼þ½ó¿ú*//*¸ ¸ ¿¿¿â â Â ¸ î ± ± »±» é¾Q*/
                            char* pszDirNameUniLe  /*Ä¿Â¼Â·¾¶ unicode little endian*/
                           );                          



INT32 OPENAT_get_current_dir(                       /*»± ± ° ° ° ä äâ äâu ¿ú¿u*/
                            char* pCurDirUniLe,    /*´æ´¢Ä¿Â¼ÐÅÏ¢ unicode little endian*/
                            UINT32 uUnicodeSize     /*´æ´¢Ä¿Â¼ÐÅÏ¢¿Õ¼ä´óÐ¡*/
                            );

INT32 OPENAT_find_open(                       /*¼ì²éÎÄ¼þ½Ó¿Ú*/
                            char* pszFileNameUniLe/*Ä¿Â¼Â·¾¶»òÎÄ¼þÈ«Â·¾¶ unicode little endian*/
                            );


INT32 OPENAT_find_first_file(                       /*²éÕÒÎÄ¼þ½Ó¿Ú*/
                            char* pszFileNameUniLe,/*Ä¿Â¼Â·¾¶»òÎÄ¼þÈ«Â·¾¶ unicode little endian*/
/*-\ BUG WM-719 \ Maliang \ 2013.3.21 \ ÎÄ¼þÏµn³½Ó¿úºn² ¥ · ÅÒÔÆµÎÄ¼Þ½Ó¿ÚµÄÎÄ¼þÃÛ¸ÄÎªunicode Little Endingààðn*/
                            PAMOPENAT_FS_FIND_DATA  pFindData /*²éÕÒ½e¹ûÊý¾Ý*/
                            );




INT32 OPENAT_find_next_file(                        /*¼ÌÐø²éÕÒÎÄ¼þ½Ó¿Ú*/
                            INT32 iFd,              /*²éõòõätt¾ lingu gali ± `` ou ú ú ú ú ú ¬ît_fest_file у½ úμ italics уб »ø²î oghuce faceý*/
                            PAMOPENAT_FS_FIND_DATA  pFindData /*²éÕÒ½e¹ûÊý¾Ý*/
                           );




INT32 OPENAT_find_close(                            /*²éÕÒ½eÊø½Ó¿Ú*/
                            INT32 iFd               /*²éõòõätt¾ lingu gali ± `` ou ú ú ú ú ú ¬ît_fest_file у½ úμ italics уб »ø²î oghuce faceý*/
                       );

INT32 OPENAT_ftell(                             /*ÎÄ¼þ¶¨Î»½Ó¿Ú*/
                            INT32 iFd             /*Îä spill ± ± ± ± ± ± ai ±ope_file »» Ø в »Ø Øäóðuraððuish faceý*/
                      );

/*+\ Newreq WM-743 \ MALIANG \ 2013.3.28 \ [Openat] ôö¼ó¿ú¿ú¿ú »ñèúîäčžičičľžľžľžľžľžaj ¢*/
INT32 OPENAT_get_fs_info(                            /*»ÎÃ¡ä ¢ ¢ ¢ ¢ ¢ ¢ ¢ ¢ ¢ ¢ ¢ ¢ ¢*/
                            E_AMOPENAT_FILE_DEVICE_NAME       devName,            /*»Ñè, »ä »ädevice nameµäðåï ¢*/
                            T_AMOPENAT_FILE_INFO               *fileInfo,                   /*Рää¼þïμn³μäïï ¢*/
                            char  								*path,                  /*ÊÇ · ñ »ñè¡sd ¿¨ðÅÏ ¢*/
							INT32 type
                       );

/*+\NEW\zhuwangbin\2020.08.08\Ìn¼ÓÎÄ¼þÏµÍ³mount½Ó¿ Ú*/
BOOL OPENAT_fs_mount(T_AMOPENAT_USER_FSMOUNT *param);
BOOL OPENAT_fs_unmount(T_AMOPENAT_USER_FSMOUNT *param);
BOOL OPENAT_fs_format(T_AMOPENAT_USER_FSMOUNT *param);

/*-\NEW\zhuwangbin\2020.08.08\Ìn¼ÓÎÄ¼þÏµÍ³mount½Ó¿¿ Ú*/

/*-\ Newreq WM-743 \ MALIANG \ 2013.3.28 \ [Openat] ôö¼ó¿ú¿ú¿ú »*/
    
    /*+\ Newreq \ jack.li \ 2013.1.17 \ ôöΩótΩ¨½óżú*/
INT32 OPENAT_init_tflash(                            /*³õÊ¼»¯T¿¨½Ó¿Ú*/
                            PAMOPENAT_TFLASH_INIT_PARAM pTlashInitParam/*T production*/
                       );



    /*-\ nereq \ jack.li \ 2013.1.17 \ ôöΩótΩ¨½óżú*/

E_AMOPENAT_MEMD_ERR OPENAT_flash_erase(              /*flash²ÁÐ´ 128K¶ÔÆë*/
                            UINT32 startAddr,
                            UINT32 endAddr
                       );


    
E_AMOPENAT_MEMD_ERR OPENAT_flash_write(              /*dfash*/
                            UINT32 startAddr,
                            UINT32 size,
                            UINT32* writenSize,
                            CONST UINT8* buf
                       );



E_AMOPENAT_MEMD_ERR OPENAT_flash_read(               /*¶Áflash*/
                            UINT32 startAddr,
                            UINT32 size,
                            UINT32* readSize,
                            UINT8* buf
                       );
                       
UINT32 OPENAT_flash_page(void);

/*+\bug2991\zhuwangbin\2020.06.11\Ôö¼Ólua otp½Ó¿Ú.*/
BOOL openat_flash_eraseSecurity(UINT8 num);

BOOL openat_flash_writeSecurity(UINT8 num, UINT16 offset, char * data, UINT32 size);

BOOL openat_flash_readSecurity(UINT8 num, UINT16 offset, char * data, UINT32 size);

BOOL openat_flash_lockSecurity(UINT8 num);
/*-\bug2991\zhuwangbin\2020.06.11\ÔöµÓlua otp½Ó¼ÚµÚ.*/

E_OPENAT_OTA_RESULT OPENAT_fota_init(void);
E_OPENAT_OTA_RESULT OPENAT_fota_download(const char* data, UINT32 len, UINT32 total);
E_OPENAT_OTA_RESULT OPENAT_fota_done(void);
    
    /*******************************************
    **                 NV                     **
    *******************************************/    
    /*How do you say ± eö Dictionary ± erect ± erect ± e £¬²ùï¸¶ï¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶ïtðätaskÖne*/    
INT32 OPENAT_nv_init(                                /*NV ³õê¼ »¯½ó¿ú*/
                      UINT32 addr1,                  /*NV ´æ·ÅµØÖ·1 4KByteµØÖ·¶ÔÆë ´óÐ¡4KByte*/
                      UINT32 addr2                   /*NV ´æ·ÅµØÖ·2 4KByteµØÖ·¶ÔÆë ´óÐ¡4KByte*/
                    );


INT32 OPENAT_nv_add(                                 /*Overview "¸ÖNES I ¢¢ ¢ çageroso*/
                      UINT32 nv_id,                  /*NV ID ä¿ç ° Ö »ÖŞ³Ö0-255*/
                      UINT32 nv_size                 /*NV ÇøÓò´óÐ¡,µ¥Î»Byte,×î´ó512Byte*/
                    );


INT32 OPENAT_nv_delete(                              /*Éľmynv*/
                      UINT32 nv_id
                      );                 


INT32 OPENAT_nv_read(                                /*Án¡nving*/
                     UINT32 nv_id,                   /*NV ID ä¿ç ° Ö »ÖŞ³Ö0-255*/
                     UINT8* buf,                     /*buf*/
                     UINT32 bufSize,                 /*bufµÄ´óÐ¡*/
                     UINT32* readSize                /*Êµ¼Ê¶ÁÈ¡³¤¶È*/
                    );

    
INT32 OPENAT_nv_write(                               /*Ð´ÈëNVÄÚÈÝ*/
                      UINT32 nv_id,                  /*NV ID ä¿ç ° Ö »ÖŞ³Ö0-255*/
                      UINT8* buf,                    /*buf*/
                      UINT32 bufSize,                /*bufµÄ´óÐ¡*/
                      UINT32* writeSize              /*Êµ¼ÊÐ´Èë³¤¶È*/
                     );          
    /*******************************************
    **                Hardware                **
    *******************************************/
    /****************************** GPIO ******************************/
BOOL OPENAT_config_gpio(                          
                            E_AMOPENAT_GPIO_PORT port,  /*GPIO ± atºl*/
                            T_AMOPENAT_GPIO_CFG *cfg    /*vrä´»ä who"*/
                       );


    
BOOL OPENAT_set_gpio(                               
                            E_AMOPENAT_GPIO_PORT port,  /*GPIO ± atºl*/
                            UINT8 value                 /* 0 or 1 */
                    );



/*+: \ Newreq WM-475 \ brezen \ 2012.12.14 \ ð¸¸ägpio½ó¿ú*/				
BOOL OPENAT_read_gpio(                            
                            E_AMOPENAT_GPIO_PORT port,  /*GPIO ± atºl*/
                            UINT8* value                /*½e¹û 0 or 1*/
                      );
/*-: \ Newreq WM-475 \ Brezen \ 2012.12.14 \ ð¸¸ägpio½ó¿ú*/

/*Bug WM-720 \ t*/
BOOL OPENAT_close_gpio(                            
                            E_AMOPENAT_GPIO_PORT port/*GPIO ± atºl*/
                      );
/*-Bug WM-720 \ t*/

bool OPENAT_gpioPulse(E_AMOPENAT_GPIO_PORT port, unsigned high_us, unsigned low_us, unsigned count, unsigned idle);

/****************************** PMD ******************************/
BOOL OPENAT_init_pmd(     
                            E_AMOPENAT_PM_CHR_MODE chrMode,     /*³Äμ2 В½½*/
/*+ \ NEW WM-746 \ Rufi \ 2013.3.30 \ éóð¾óth unputic³g*/
                            T_AMOPENAT_PMD_CFG*    cfg,         /*³ prosecute*/
/*- \ NEW WM-746 \ Rufi \ 2013.3.30 \ éóhð¾¾¬ æ¬μ*/
                            PPM_MESSAGE            pPmMessage   /*Ïuï ¢ »øμ ÷ ÷÷eý*/
                    );



VOID OPENAT_get_batteryStatus(
                            T_AMOPENAT_BAT_STATUS* batStatus    /*µç³Ø×´Ì¬ OUT*/
                             );



VOID OPENAT_get_chargerStatus(
                            T_AMOPENAT_CHARGER_STATUS* chrStatus/*³äµçÆ÷×´Ì¬ OUT*/
                             );



/*+\ New \ rufe \ 2014.2.13 \ ôöstoneoopenat²éñamñ² ÷ hw × ´ì¬½o*/
E_AMOPENAT_CHR_HW_STATUS OPENAT_get_chargerHwStatus(
                            VOID
                            );


/*-\ new \ rufe \ 2014.2.13 \ ôöàopenat²éñ thousand ^ × × ´ì¬½ó*/
/*+\ Task \ Wangyuan \ 2020.06.28 \ Task_255: Ö§³ööðôödå ° ², ¼æèý2G CSDKµä½ó¿úìn¼ó*/
int OPENAT_get_chg_param(BOOL *battStatus, u16 *battVolt, u8 *battLevel, BOOL *chargerStatus, u8 *chargeState);
/*-\ Task \ WangYuan \ 2020.06.28 \ Task_255: Ö§³ööðôödå ° ², ¼æèý2G CSDKµä½ó¿úìn¼ó*/
BOOL OPENAT_poweron_system(                                     /*£ £ ¿ª ª ª ª ª*/  
                            E_AMOPENAT_STARTUP_MODE simStartUpMode,/*‘ªæôsim« · ½ê½*/
                            E_AMOPENAT_STARTUP_MODE nwStartupMode/*¿ªÆôÐ­ÒéÕ»·½Ê½*/
                          );



VOID OPENAT_poweroff_system(                                    /*ÕÝ³ £ ¹ø »Ú £ ¬ ° üà¨¹ø ± õðòÉõ» ºn¹ © µÇ*/        
                            VOID
                           );



BOOL OPENAT_poweron_ldo(                                        /*´ò —ªLDO*/
                            E_AMOPENAT_PM_LDO    ldo,
                            UINT8                level          /*0-7 0: ¹Ø ± õ 1 ~ 7Muçñ¹¹¼¶¶*/
                       );



BOOL OPENAT_gpio_disable_pull(                            /*GPIOÅäÖÃ½Ó¿Ú*/
                            E_AMOPENAT_GPIO_PORT port  /*GPIO ± atºl*/
        );

VOID OPENAT_enter_deepsleep(VOID);                                   /*½øÈëË¯Ãß*/
                      


VOID OPENAT_exit_deepsleep(VOID);                                     /*Íë³öë¯ãß*/

void OPENAT_deepSleepControl(E_AMOPENAT_PMD_M m, BOOL sleep, UINT32 timeout);



/*+NEW OPEANT-104\RUFEWUFEWUFEWUFEWUFEWUFEWUFE.*/
 E_AMOPENAT_POWERON_REASON OPENAT_get_poweronCause (VOID);             /*"*/
/*-NEW OPEANT-104\RUFEWEWUFEWOURUFEWEWUFEWA.6.6.17\ me¼´«Ooúúúúú»º»´*/


    /****************************** UART ******************************/
BOOL OPENAT_config_uart(
                            E_AMOPENAT_UART_PORT port,          /*Urt ± àºå*/
                            T_AMOPENAT_UART_PARAM *cfg          /*¯³Ê¼ »¯²ÎÝ*/
                       );



/*+\NEW\liweiqiang\2013.4.20\Ôö¼Ó¹Ø±Õuart½Ó¿Ú*/
BOOL OPENAT_close_uart(
                            E_AMOPENAT_UART_PORT port           /*Urt ± àºå*/
                       );
/*-\NEW\liweiqiang\2013.4.20\Ôö¼Ó¹Ø±Õuart½Ó¿Ú*/




UINT32 OPENAT_read_uart(                                        /*Êµ¼Ê¶ÁÈ¡³¤¶È*/
                            E_AMOPENAT_UART_PORT port,          /*Urt ± àºå*/
                            UINT8* buf,                         /*´æ´¢Êý¾ÝµØÖ·*/
                            UINT32 bufLen,                      /*´æ´¢¿Õ¼ä³¤¶È*/
                            UINT32 timeoutMs                    /*¶ÁÈ¡³¬Ê± ms*/
                       );



UINT32 OPENAT_write_uart(                                       /*Êµ¼ÊÐ´Èë³¤¶È*/
                            E_AMOPENAT_UART_PORT port,          /*Urt ± àºå*/
                            UINT8* buf,                         /*Ð´ÈëÊý¾ÝµØÖ·*/
                            UINT32 bufLen                       /*Ð´ÈëÊý¾Ý³¤¶È*/
                        );

UINT32 OPENAT_write_uart_sync(E_AMOPENAT_UART_PORT port,          /*Urt ± àºå*/
                           UINT8* buf,                         /*Ð´ÈëÊý¾ÝµØÖ·*/
                           UINT32 bufLen                       /*Ð´ÈëÊý¾Ý³¤¶È*/
                           );


/*+\NEW\liweiqiang\2014.4.12\Ôö¼Ó´®¿Ú½ÓÊÕÖÐ¶ÏÊ¹ÄÜ½Ó¿Ú*/
BOOL OPENAT_uart_enable_rx_int(
                            E_AMOPENAT_UART_PORT port,          /*Urt ± àºå*/
                            BOOL enable                         /*Ê · ·*/
                                );
/*-\NEW\liweiqiang\2014.4.12\Ôö¼Ó´®¿Ú½ÓÊÕÖÐ¶ÏÊ¹ÄÜ½Ó¿Ú*/




/*+\NEW\liweiqiang\2013.12.25\Ìn¼Óhost uart·¢ËÍÊý¾Ý¹¦ÄÜ*/
    /****************************** HOST ******************************/
BOOL OPENAT_host_init(PHOST_MESSAGE hostCallback);



BOOL OPENAT_host_send_data(uint8 *data, uint32 len);
/*-\NEW\liweiqiang\2013.12.25\Ìn¼Óhost uart·¢ËÍÊý¾Ý¹¦ÄÜ*/



    /******************************* SPI ******************************/
/*+\NEW\zhuwangbin\2020.3.7\Ìn¼Óopenat spi½Ó¿ Ú*/
BOOL OPENAT_OpenSPI( E_AMOPENAT_SPI_PORT port, T_AMOPENAT_SPI_PARAM *cfg);
UINT32 OPENAT_ReadSPI(E_AMOPENAT_SPI_PORT port, CONST UINT8 * buf, UINT32 bufLen);
UINT32 OPENAT_WriteSPI(E_AMOPENAT_SPI_PORT port, CONST UINT8 * buf, UINT32 bufLen, BOOLEAN type);
UINT32 OPENAT_RwSPI(E_AMOPENAT_SPI_PORT port, CONST UINT8* txBuf, CONST UINT8* rxBuf,UINT32 bufLen);
BOOL OPENAT_CloseSPI( E_AMOPENAT_SPI_PORT port);
/*-\NEW\zhuwangbin\2020.3.7\Ìn¼Óopenat spi½Ó¿¿ Ú*/







/******************************* I2C ******************************/
BOOL OPENAT_open_i2c(
                            E_AMOPENAT_I2C_PORT  port,          /*I2C ± àºå*/
                            T_AMOPENAT_I2C_PARAM *param         /*¯³Ê¼ »¯²ÎÝ*/
                      );




BOOL OPENAT_close_i2c(
                            E_AMOPENAT_I2C_PORT  port           /*I2C ± àºå*/
                      );



UINT32 OPENAT_write_i2c(                                        /*Êµ¼ÊÐ´Èë³¤¶È*/
                            E_AMOPENAT_I2C_PORT port,          /*I2C ± àºå*/
                            UINT8 salveAddr,
                            CONST UINT8 *pRegAddr,              /*I2CÍâÉè¼Ä´æÆ÷µØÖ·*/
                            CONST UINT8* buf,                   /*Ð´ÈëÊý¾ÝµØÖ·*/
                            UINT32 bufLen                       /*Ð´ÈëÊý¾Ý³¤¶È*/
                       );




UINT32 OPENAT_read_i2c(                                         /*Êµ¼Ê¶ÁÈ¡³¤¶È*/
                            E_AMOPENAT_I2C_PORT port,          /*I2C ± àºå*/
                            UINT8 slaveAddr, 
                            CONST UINT8 *pRegAddr,              /*I2CÍâÉè¼Ä´æÆ÷µØÖ·*/
                            UINT8* buf,                         /*´æ´¢Êý¾ÝµØÖ·*/
                            UINT32 bufLen                       /*´æ´¢¿Õ¼ä³¤¶È*/
                      );



BOOL  OPENAT_open_bt(
                            T_AMOPENAT_BT_PARAM* param
                     );



BOOL  OPENAT_close_bt(VOID);
                           



BOOL  OPENAT_poweron_bt(VOID);

  
BOOL  OPENAT_poweroff_bt(VOID);

BOOL  OPENAT_send_cmd_bt
                      (
                            E_AMOPENAT_BT_CMD cmd, 
                            U_AMOPENAT_BT_CMD_PARAM* param
                      );    

BOOL  OPENAT_build_rsp_bt
                      (
                            E_AMOPENAT_BT_RSP rsp,
                            U_AMOPENAT_BT_RSP_PARAM* param
                      );   

/*± ¾¶ë × ÷ªdee ± ¸ ÷¶÷¶ ÷¶¯ or ÷¶ðe¬__ ÷¶ða½Openat_bt_SP_CNonnect_cnf_cnf
的 ¸ ¸ ¸ ¸ð ¸¼½óâ¶ªç¾ªªªªμμμ÷÷½¸¸¸ ðó è ú о ¬
Hotel Oliói Period The Eêõopenat_bt_Spp_Connect_indan ¢*/
BOOL  OPENAT_connect_spp                              
                      (
                            T_AMOPENAT_BT_ADDR* addr,
                            T_AMOPENAT_UART_PARAM* portParam    /*ÔÝÊ±²»Ö§³Ö,¿ÉÒÔÐ´NULL£¬Ä¬ÈÏÅäÖÃÎª9600,8(data),1(stop),none(parity)*/
                      );

BOOL  OPENAT_disconnect_spp                                    /*¶Ï¿ªÁ¬½Ó£¬½e¹û OPENAT_BT_SPP_DISCONNECT_CNF*/
                      (
                            UINT8   port                        /*¶Ë¿ÚºÅ£¬»eÔÚOPENAT_BT_SPP_CONNECT_IND/OPENAT_BT_SPP_CONNECT_CNFÖÐÉÏ±¨*/
                      ); 

INT32  OPENAT_write_spp                                         /*·¢ââRâice¹”e´àµ´µ¯µµBT_BET_BET_SPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP.*/
                                                                /*!
                                                                  Openat_bt_spp_send_data_cnfêâ¼þéï ± ¨*/
                      (
                            UINT8   port,                       /*¶Ë¿ÚºÅ£¬»eÔÚOPENAT_BT_SPP_CONNECT_IND/OPENAT_BT_SPP_CONNECT_CNFÖÐÉÏ±¨*/
                            UINT8*  buf,                        /*²” rllsµµeèµeâ»®®®® üæèèèughèughèèèèâS´S´´ü´SOâ  ´àâls0*/
                            UINT32  bufLen                      /*Ò»´Î×î¶à´«ÊäT_AMOPENAT_BT_SPP_CONN_IND.maxFrameSize´óÐ¡×Ö½ÚµÄÊý¾Ý*/
                      );


INT32  OPENAT_read_spp                                         /*»Øµ ÷ ºrd ºrdýêõêõ ½OopeNAT_BT_SPP_DATA_indêârse £ ¬µ ÷ Ónção½ó*/
                                                                /*· Оμ »Øöμît · french · p¼¶OT¡³*/
                      (
                            UINT8   port,                       /*¶Ë¿ÚºÅ£¬»eÔÚOPENAT_BT_SPP_CONNECT_IND/OPENAT_BT_SPP_CONNECT_CNFÖÐÉÏ±¨*/
                            UINT8*  buf,
                            UINT32  bufLen
                      );   


/****************************** AUDIO ******************************/
BOOL OPENAT_open_tch(VOID);                                       /*How do you love £ ours е ° »¿â ¿e у μ μ ÷ ÷*/
                           


BOOL OPENAT_close_tch(VOID);                                         /*¹Ø±ÕÓïÒô£¬Í¨»°½eÊøÊ±µ÷ÓÃ*/

                            
BOOL OPENAT_play_tone(                                          /*²¥·ÅTONEÒô½Ó¿Ú*/
                            E_AMOPENAT_TONE_TYPE toneType,      /*They were loringð*/
                            UINT16 duration,                    /*²¥·ÅÊ±³¤*/
                            E_AMOPENAT_SPEAKER_GAIN volume      /*² ¥ · å å å å*/
                     );


BOOL OPENAT_stop_tone(VOID);                                          /*In £ Ö¹² ¥ · ÅtoneÒÔ½Ó¿ú*/


                            
BOOL OPENAT_play_dtmf(                                          /*²¥·ÅDTMFÒô½Ó¿Ú*/
                            E_AMOPENAT_DTMF_TYPE dtmfType,      /*Dtmfààðn*/
                            UINT16 duration,                    /*²¥·ÅÊ±³¤*/
                            E_AMOPENAT_SPEAKER_GAIN volume      /*² ¥ · å å å å*/
                     );


BOOL OPENAT_stop_dtmf(VOID);                                          /*In £ Ö¹² ¥ · ÅDTMFÒÔ½Ó¿Ú*/


/*+ \ And opt \ cmm \ cmm \ es¿ ep "439 'ðèoªpö³³³ olive grape ¥ ¥ ¥ ¥ ¥ ¥ ¥ ¥μ × ¥ ¥ ¥μ × 】μ × × × × × × × × × × × × × × × × × × Italiana)*/
int OPENAT_streamplayV2(E_AMOPENAT_AUD_PLAY_TYPE playtype,E_AMOPENAT_AUD_FORMAT playformat,AUD_PLAY_CALLBACK_T cb,char* data,int len);
/*- \ andîñi · ÷î.20 \ ÷²uro froªoªpö³ï ol ï ol i m activï oh "t" the tigure*/
int OPENAT_streamplay(E_AMOPENAT_AUD_FORMAT playformat,AUD_PLAY_CALLBACK_T cb,char* data,int len);

                            
/*+\ Newreq wm-584 \ Maliang \ 21.21.21 \ [Openat] Ö§aint*/
BOOL OPENAT_play_music(T_AMOPENAT_PLAY_PARAM*  playParam);
/*-\ Newreq WM-584 \ Maliang \ 21.2.21 \*/


BOOL OPENAT_stop_music(VOID);                                         /*In £ Ö¹ÒÔæµ² ¥ · Å½Ó¿ú*/


                            
BOOL OPENAT_pause_music(VOID);                                        /*Make a Rile £ ì²² ² ² iranikue.*/


                            
BOOL OPENAT_resume_music(VOID);                                      /*In £ Ö¹ÒÔæµ² ¥ · Å½Ó¿ú*/

                           
/*+\NewReq WM-710\maliang\2013.3.18\ [OpenAt]Ôö¼Ó½Ó¿ÚÉèÖÃMP3²¥·ÅµÄÒôÐ§*/
BOOL OPENAT_set_eq(                                       /*ÉèÖÃMP3ÒôÐ§*/
                            E_AMOPENAT_AUDIO_SET_EQ setEQ
                          );
/*-\NewReq WM-710\maliang\2013.3.18\ [OpenAt]Ôö¼Ó½Ó¿ÚÉèÖÃMP3²¥·ÅµÄÒôÐ§*/

BOOL OPENAT_open_mic(VOID);                                           /*¿ªæômic½ó you*/
                            


BOOL OPENAT_close_mic(VOID);                                          /*¹Ø±ÕMIC½Ó¿Ú*/

                           
BOOL OPENAT_mute_mic(VOID);                                           /*MIC¾²Òô½Ó¿Ú*/


BOOL OPENAT_unmute_mic(VOID);                                         /*½â³ýMIC¾²Òô½Ó¿Ú*/

                            
BOOL OPENAT_set_mic_gain(                                       /*ÉèÖÃMICÔöÒæ½Ó¿Ú*/
                            UINT16 micGain                      /*É'èödicācoæiòóoæ £ ¥î 3.óªª20*/
                        );



int OPENAT_audio_record( 									  /*Â¼Òô½Ó¿Ú*/
										E_AMOPENAT_RECORD_PARAM* param,
										AUD_RECORD_CALLBACK_T cb);



int OPENAT_audio_stop_record(VOID);

BOOL OPENAT_open_speaker(VOID);                                       /*Indianno ... ÷ ÷ у (÷ ½ó?*/


BOOL OPENAT_close_speaker(VOID);                                      /*¹Ø±ÕÑïÉùÆ÷½Ó¿Ú*/


BOOL OPENAT_mute_speaker(VOID);                                       /*ÑïÉùÆ÷¾²Òô½Ó¿Ú*/


BOOL OPENAT_unmute_speaker(VOID);                                     /*½â³ýÑïÉùÆ÷¾²Òô½Ó¿Ú*/


                           
BOOL OPENAT_set_speaker_gain(                                   /*ÉèÖÃÑïÉùÆ÷µÄÔöÒæ*/
                            E_AMOPENAT_SPEAKER_GAIN speakerGain /*ÉèÖÃÑïÉùÆ÷µÄÔöÒæ*/
                            );



/*+\bug\wj\2020.5.6\Ôö¼ÓÍ¨»°ÖÐµ÷½ÚÒôÁ¿½Ó¿Ú*/
BOOL OPENAT_set_sph_vol(								   
						UINT32 vol);
/*-\bug\wj\2020.5.6\Ôö¼ÓÍ¨»°ÖÐµ÷½ÚÒôÁ¿½Ó¿Ú*/
/*+\BUG\wangyuan\2020.08.10\BUG_2801: Ë¼ÌØCSDK Í¨¹ýiot_audio_set_speaker_vol()½Ó¿ÚÉèÖÃÍ¨¹ýÒôÁ¿ÎÞÐ§ AT+CLVL¿ÉÒÔÐÞ¸ÄÍ¨»°ÒôÁ¿*/
UINT32 OPENAT_get_sph_vol(void);
/*-\BUG\wangyuan\2020.08.10\BUG_2801: Ë¼ÌØCSDK Í¨¹ýiot_audio_set_speaker_vol()½Ó¿ÚÉèÖÃÍ¨¹ýÒôÁ¿ÎÞÐ§ AT+CLVL¿ÉÒÔÐÞ¸ÄÍ¨»°ÒôÁ¿*/

E_AMOPENAT_SPEAKER_GAIN OPENAT_get_speaker_gain(VOID);                /*»÷ µ*/


/*+\BUG\wangyuan\2020.11.27\BUG_3634$*/
BOOL OPENAT_set_channel(                                        /*ÉèÖÃÒôÆµÍ¨µÀ½Ó¿Ú*/
                            E_AMOPENAT_AUDIO_CHANNEL outputchannel    /*Đee³³à*/,
                            E_AMOPENAT_MIC_CHANNEL inputchannel    /*ÊäÈëÍ¨µÀ*/
                       );
/*-\BUG\wangyuan\2020.11,*/

/*+\BUG\wgyuan\2020.06,088jBUG_2163:CSDHS´ audiough²²²»«*/
BOOL OPENAT_set_music_volume(UINT32 vol);		/*Earṣ astrology! 2. Selecta*/

UINT32 OPENAT_get_music_volume(void);		/*"O'T» I'm, Here appear 38*/

void OPENAT_delete_record(VOID);
/*-\BUG\wgyuan\2020.06,088jBUG_2163:CSDY¹²²²²«²²«²²²».*/

VOID OPENAT_set_channel_with_same_mic(                          /*And*/
                        E_AMOPENAT_AUDIO_CHANNEL channel_1,     /*Í¨µÀ 1*/
                        E_AMOPENAT_AUDIO_CHANNEL channel_2      /*Í¨µÀ 2*/
                   );



/*+\BUG WM-882\rufei\2013.7.18\ÍêÉÆÍ¨µÀÉèÖÃ*/
BOOL set_hw_channel(
                          E_AMOPENAT_AUDIO_CHANNEL hfChanne,    /*ÊÖ±úÍ¨µÀ*/
                          E_AMOPENAT_AUDIO_CHANNEL erChanne,    /*¶ú»úÍ¨µÀ*/
                          E_AMOPENAT_AUDIO_CHANNEL ldChanne    /*ÃâÌeÍ¨µÀ*/
                         );
/*-\BUG WM-882\rufei\2013.7.18\ÍêÉÆÍ¨µÀÉèÖÃ*/

/*+\ New \ zhuwangbin \ 2020.8.11 \ \*/
int OPENAT_headPlug(E_OPENAT_AUD_HEADSET_TYPE type);
/*-\ new \ zhuwangbin \ 2020.8.11 \ \*/

E_AMOPENAT_AUDIO_CHANNEL OPENAT_get_current_channel(VOID);



/*+\NewReq WM-711\maliang\2013.3.18\[OpenAt]Ôö¼Ó½Ó¿Ú´ò¿ª»ò¹Ø±ÕÒôÆµ»Ø»·²âÊÔ*/
/*+\New\lijiadi\2014.7.30\çÐç»OoroÆÆÆâââ‼²«²»µµÓISpLeavlAd²²Áughlölä
                           Èce¹IsSpkLevelAdjustästâs «FALSE,spkLeyµµµµ´èèèèov.´¶*/
BOOL  OPENAT_audio_loopback(BOOL  start,                    /*¿ªê¼ »òn £ Ö¹» ø »· ²âêô*/
                                        E_AMOPENAT_AUDIO_LOOPBACK_TYPE type,   /*»Ø» · ²âêôµäààðn*/
                                        BOOL IsSpkLevelAdjust,   /*Spkéùòô´óð¡êç · ñ¿éö¸¶¨*/
                                        UINT8 SpkLevel);   /*SPKÖ¸¶¨µÄÉùÒô´óÐ¡SpkLevelÈ¡Öµ·¶Î§AUD_SPK_MUTE--AUD_SPK_VOL_7*/
/*-\New\lijiadi\2014.7.30\çÐÑÐÑÐÑÑöughugà²«²»»²àµÓSpLEspLEspLed²ºèlelâ
                           Èce¹IsSpkLevelAdjustästâs «FALSE,spkLeyµµµµ´èèèèov.´¶*/
/*-\NewReq WM-711\maliang\2013.3.18\[OpenAt]Ôö¼Ó½Ó¿Ú´ò¿ª»ò¹Ø±ÕÒôÆµ»Ø»·²âÊÔ*/



BOOL  OPENAT_audio_inbandinfo(PINBANDINFO_CALLBACK callback); 

int OPENAT_WritePlayData(char* data, unsigned size);

    
/****************************** ADC ******************************/
/*ment*/
/*+\bug3689\zhuwangbin\2020.11.25\adcÌn¼Ó¿ ÉÑ¡ ²ÎÊýscale*/
BOOL OPENAT_InitADC(
	    E_AMOPENAT_ADC_CHANNEL channel  /*Adc ± toºl*/,
	    E_AMOPENAT_ADC_CFG_MODE mode
	    	);


BOOL OPENAT_ReadADC(
				    E_AMOPENAT_ADC_CHANNEL channel,  /*Adc ± toºl*/
				    kal_uint32*               adcValue,   /*Adc ֵ*/
				    kal_uint32*               voltage    /*μçaud¹öμ*/
				);

BOOL OPENAT_CloseADC(
    E_AMOPENAT_ADC_CHANNEL channel  /*Adc ± toºl*/
);

BOOL OPENAT_SetScaleAdc(E_AMOPENAT_ADC_CHANNEL channel,
			E_AMOPENAT_ADC_SCALE scale);

/*-\bug3689\zhuwangbin\2020.11.25\adcÌn¼Ó¿ ÉÑ¡ ²ÎÊýscale*/

/*-\BUG\ 2020.30.30\ 2424:CSDK-8*/

/*+\bug3708\zhuwangbin\2020.11.26\ÓÅ»¯pwm´úÂë*/
bool OPENAT_pwm_open(E_AMOPENAT_PWM_PORT port);
bool OPENAT_pwm_set(T_AMOPENAT_PWM_CFG *pwm_cfg);
bool OPENAT_pwm_close(E_AMOPENAT_PWM_PORT port);
/*-\bug3708\zhuwangbin\2020.11.26\ÓÅ»¯pwm´úÂë*/

/****************************** LCD ******************************/
/* MONO */                                                  /*º ° × âe*/			
BOOL OPENAT_init_mono_lcd(                                      /*Ænd ³ ³ ¯ ¯ ¯ ¯ ¯*/
                            T_AMOPENAT_MONO_LCD_PARAM*  monoLcdParamP
                    );

/*+\bug2958\czm\2020.9.1\disp.close() Ö®ºóÔÙÖ´ÐÐdisp.init ÎÞÌeÊ¾Ö±½ÓÖØÆô*/
BOOL OPENAT_close_mono_lcd(void);/*Lcd¹Ø±Õ½Ó¿Ú*/
/*-\bug2958\czm\2020.9.1\disp.close() Ö®ºóÔÙÖ´ÐÐdisp.init ÎÞÌeÊ¾Ö±½ÓÖØÆô*/

VOID OPENAT_send_mono_lcd_command(                              /*·¢ËÍÃüÁî½Ó¿Ú*/
                            UINT8 cmd                           /*.Áî*/
                                 );


VOID OPENAT_send_mono_lcd_data(                                 /*·¢ËÍÊý¾Ý½Ó¿Ú*/
                            UINT8 data                          /*Êý¾Ý*/
                              );



VOID OPENAT_update_mono_lcd_screen(                             /*¸üÐÂÆÁÄ»½Ó¿Ú*/
                            T_AMOPENAT_LCD_RECT_T* rect         /*Ðèòªë ¢ ðâµäçøóò*/
                                  );


VOID OPENAT_clear_mono_lcd(                                     /*Çåæe £ ¬Ò »° ÃóÃÓÚÊµ¼ÊLCD RAM ± ÈÏÔÊ¾ÇØÓÒ´ÓµÄçÉ¿Ö*/
                            UINT16 realHeight,                  /*Êµ¼ÊLCD RAM ¸ß¶È*/
                            UINT16 realWidth                    /*Êµ¼ÊLCD RAM ¿n¶È£¬±ØÐëÊÇ4µÄ±¶Êý*/
                          );


/* COLOR */                                                 /*²êé «æe*/
BOOL OPENAT_init_color_lcd(                                     /*Ænd ³ ³ ¯ ¯ ¯ ¯ ¯*/
                            T_AMOPENAT_COLOR_LCD_PARAM *param   /*Ice-english »ï b²oeêý*/
                          );

/*+\bug2958\czm\2020.9.1\disp.close() Ö®ºóÔÙÖ´ÐÐdisp.init ÎÞÌeÊ¾Ö±½ÓÖØÆô*/
BOOL OPENAT_close_color_lcd(void);/*Lcd¹Ø±Õ½Ó¿Ú*/
/*-\bug2958\czm\2020.9.1\disp.close() Ö®ºóÔÙÖ´ÐÐdisp.init ÎÞÌeÊ¾Ö±½ÓÖØÆô*/

BOOL OPENAT_spiconfig_color_lcd(                     /*²êæespiåäöã*/
                        void                    
                        );

VOID OPENAT_send_color_lcd_command(                             /*·¢ËÍÃüÁî½Ó¿Ú*/
                            UINT8 cmd                           /*.Áî*/
                                  );


VOID OPENAT_send_color_lcd_data(                                /*·¢ËÍÊý¾Ý½Ó¿Ú*/
                            UINT8 data                          /*Êý¾Ý*/
                               );


VOID OPENAT_update_color_lcd_screen(                            /*¸üÐÂÆÁÄ»½Ó¿Ú*/
                            T_AMOPENAT_LCD_RECT_T* rect,        /*Ðèòªë ¢ ðâµäçøóò*/
                            UINT16 *pDisplayBuffer              /*Ë "ðâμä º³åçø*/
                                   );


void OPENAT_layer_flatten(OPENAT_LAYER_INFO* layer1,
                                   OPENAT_LAYER_INFO* layer2,
                                   OPENAT_LAYER_INFO* layer3) ;



/*+\ New \ Jack.Li \ 2013.2.9 \ ôö¼óé · · · êóµöæčó¿ú*/
BOOL OPENAT_camera_videorecord_start(                           /*¿ªÊ¼Â¼ÖÆÊÓÆµ*/
                    INT32 iFd                               /*Â¼ÏñÎÄ¼þ¾ä±ú*/
                    );


BOOL OPENAT_camera_videorecord_pause(                           /*Surclose € O. Year*/
                    void                    
                    );


BOOL OPENAT_camera_videorecord_resume(                          /*»Ö¸´Â¼ÖÆÊÓÆµ*/
                        void                    
                        );


BOOL OPENAT_camera_videorecord_stop(                            /*In £ Ö¹â¼öæêóæµ*/
                        void                    
                        );


/*-\ New \ Jack.Li \ 2013.2.9 \ ôö¼óé · · · êóµöæčó¿ú*/

/*-\NEW\Jack.li\2013.1.28\Ôö¼ÓÉãÏñÍ·Çý¶¯*/

/*+\ New \ Jack.Li \ 2013.2.10 \ ôö¼ó thehouse · · Å½ó¿ú*/
BOOL OPENAT_video_open(                                         /*Thongleærei '' how much*/
                        T_AMOPENAT_VIDEO_PARAM *param           /*Ocereadý*/
                        );


BOOL OPENAT_video_close(                                        /*¹Ø±ÕÊÓÆµ»·¾³*/
                        void
                        );


BOOL OPENAT_video_get_info(                                     /*»Ñè¡êóæµðåï ¢*/
                        T_AMOPENAT_VIDEO_INFO *pInfo            /*ÊÓæµÐÅÏ ¢*/
                        );


BOOL OPENAT_video_play(                                         /*² ¥ · to*/
                        void
                        );


BOOL OPENAT_video_pause(                                        /*Smapteritions £*/
                        void
                        );


BOOL OPENAT_video_resume(                                       /*»Ö¸´*/
                        void
                        );


BOOL OPENAT_video_stop(                                         /*ֹͣ*/
                        void
                        );
    /*-\ New \ Jack.Li \ 2013.2.10 \ ôö¼ó thehouse · · Å½ó¿ú*/

#if 0 
void OPENAT_ttsply_Error_Callback(S32 ttsResult);
void OPENAT_ttsply_State_Callback(int ttsplyState);
BOOL OPENAT_ttsply_initEngine(AMOPENAT_TTSPLY_PARAM *param);
BOOL OPENAT_ttsply_setParam(U16 playParam, S16 value);
S16 OPENAT_ttsply_getParam(U16 playParam);
BOOL OPENAT_ttsply_play(AMOPENAT_TTSPLY *param);
BOOL OPENAT_ttsply_pause(void);
BOOL OPENAT_ttsply_stop(void);
#endif //__AM_LUA_TTSPLY_SUPPORT__


void OPENAT_AW9523B_display(
                                                                u8 num1, 
                                                                u8 num2, 
                                                                u8 num3
                                                                );



void OPENAT_AW9523B_set_gpio(
                                                                    u8 pin_num, 
                                                                    u8 value
                                                                    );

void OPENAT_AW9523B_init(void);


BOOL OPENAT_register_msg_proc(
                                                                    int msg_id, 
                                                                    openat_msg_proc msg_proc
                                                                    );



void OPENAT_SLI3108_init(openat_msg_proc msg_proc);


typedef enum SLI3108_STATUS_TAG
{
    SLI3108_STATUS_INVALID,
    SLI3108_STATUS_LOW,                          /*³¬³ö×îµÍ·§Öµ£¬±nÊ¾Î´Åå´÷*/
    SLI3108_STATUS_HIGH,                         /*³¬³ö×î¸ß·§Öµ£¬±nÊ¾ÒÑÅå´÷*/
}SLI3108_STATUS; 


    /* NULL */
    /****************************** KEYPAD ******************************/
BOOL OPENAT_init_keypad(                                        /*¼üÅÌ³õÊ¼»¯½Ó¿Ú*/
                            T_AMOPENAT_KEYPAD_CONFIG *pConfig   /*¼üÅäÃ²îý*/
                       );
    

    /****************************** TOUCHSCREEN ******************************/
BOOL OPENAT_init_touchScreen(                                   /*´ ¥ ãþæe³õêtent »¯½ó¿ú*/
                            PTOUCHSCREEN_MESSAGE pTouchScreenMessage /*The ¥ æoy '¢ Øμ ÷ ilesreadý*/
                            );
    


VOID OPENAT_TouchScreen_Sleep_In(VOID);



VOID OPENAT_TouchScreen_Sleep_Out(VOID);

    
/******************************** PSAM ***********************************/
/*× ¢ ÒÂ ::: psam¿¨½ó¿úÔÚ²Ù × ÷ ÉÈ ± ¸Ê ± »eµ¼öÂµ ÷ óÃõß ±» ¹ÒæÐ £ ¬Ö ± µ½ÉÈ ± ¸óÐÏÌÓ¦ »Òõß2S+³¬Ê ±*/
E_AMOPENAT_PSAM_OPER_RESULT OPENAT_open_psam(                   /*'Ò¿ªpsam*/
                            E_AMOPENAT_PSAM_ID id               /*Ó²¼lamsim¿½*/
                                            );


VOID OPENAT_close_psam(                                         /*¹Ø±Õpsam*/
                            E_AMOPENAT_PSAM_ID id               /*Ó²¼lamsim¿½*/
                      );
	/*rw_psamº²¿n»µèµ
		psam ÖÁiÁil
		------------------------------------------------------- -
	    ´úÂÂ |	Öµ
		------------------------------------------------------- -
		CLA 80
		INS 82
		P1 00
		P2 00" orÃ»‱ugh¨K¨K¨
		Roy 08
		DATE µWÕèÜ«
		------------------------------------------------------- -

	óyâ âlowâ â€™SOµWYµ¼²²²²²²²²»²¢»
	  µ ²¯² ·¢ââîÖµµ²²²²²±±±LÃ‟rxLÉ =1£stopClock = FAALSE
	  	· µ”µµ¿çWWWY«Ins ¡¢ ~Ins 
	  		Èce¹zo ·â€à´è»´øøøÈøÈøö²²´
	  		Èce¹zo ·â »IC~Ins£µ»øøal´öögü»
	  ¼¶ü½´ Èce¹DATATELECE£óµµ²´óóóóóóóóóóâ ²»¢è ½âTAATE£0º´´Éâ™nxxâ‱nÉ²»atedClock Clock = FAALSE£¬
	        ·â€™Ö«ug»´øøÈøÈ»´
	     ·µÖµµug´´Sºnµè»µµµ´´
		   
	  µ²²²¢iceâ µWâ ²¾à»®±±BStopClock=TRUE,rxL ¾¾ð*/					  
E_AMOPENAT_PSAM_OPER_RESULT OPENAT_rw_psam(                     /*´«ÊäÊý¾Ý*/
                            E_AMOPENAT_PSAM_ID id,              /*Ó²¼lamsim¿½*/
                            CONST UINT8*  txBuf,                /*Ð´»º´æ*/
                            UINT16        txLen,                /*Ð´»º´æ³¤¶È*/
                            UINT8*        rxBuf,                /*¶Á»º´æ*/
                            UINT16        rxLen,                /*¶Á»º´æ³¤¶È*/
                            BOOL          stopClock             /*ALóW»¿¤â€™èWâ »¤FALSE, Ãâ ughâ ughè²Jè²²²²²²AS.*/  
                                          );


    
E_AMOPENAT_PSAM_OPER_RESULT OPENAT_reset_psam(                  /*¸ »» PSM*/
                            E_AMOPENAT_PSAM_ID id,              /*Ó²¼lamsim¿½*/
                            UINT8*      atrBuf,                 /*Atr »º´æ*/
                            UINT16      atrBufLen,              /*Atr »º´æ³¤¶È*/
                            E_AMOPENAT_PSAM_VOLT_CLASS volt     /*¹¤×÷µçÑ¹*/
                                             );



E_AMOPENAT_PSAM_OPER_RESULT OPENAT_setfd_psam(                  /*FèÃFÖºÍDÖ¬£¬ÈÏF=372 D=1*/
                            E_AMOPENAT_PSAM_ID id,              /*Ó²¼lamsim¿½*/
                            UINT16      f,                      /*F ֵ*/
                            UINT8       d                       /*D…*/
                                             );

    /******************************** PWM ***********************************/
/*+\NEW\RUFEI\2015.9.8\Add pwm function */
BOOL OPENAT_OpenPwm(E_AMOPENAT_PWM_PORT port);

BOOL OPENAT_SetPwm(T_AMOPENAT_PWM_CFG *cfg);

BOOL OPENAT_ClosePwm(E_AMOPENAT_PWM_PORT port);
/*-\NEW\RUFEI\2015.9.8\Add pwm function */

    /****************************** FM ******************************/
BOOL OPENAT_open_fm(											/*'Ò¿¿ªfm*/
                            T_AMOPENAT_FM_PARAM *fmParam        /*³õÊ¼»¯Êý¾Ý*/
                   );


BOOL OPENAT_tune_fm(											/*µ÷µ½Ö¸¶¨ÆµÂÊ*/
                            UINT32 frequency                    /*ÆµÂÊ(KHZ)*/
                   );


BOOL OPENAT_seek_fm(											/*ËÑË÷ÏÂÒ»¸öÌ¨*/
                            BOOL seekDirection					/*TRUE:ÆµÂÊÔö¼ÓµÄ·½Ïò FALSE::ÆµÂÊ¼õÐ¡µÄ·½Ïò*/		
                   );


BOOL OPENAT_stopseek_fm(										/*In £ Ö¹ëñë ÷*/
                            void
                       );


BOOL OPENAT_setvol_fm(											/*ÉèÖÃÒôÐ§*/
                            E_AMOPENAT_FM_VOL_LEVEL volume, 	/*Knowledge well*/
                            BOOL bassBoost, 
                            BOOL forceMono
                     );



BOOL OPENAT_getrssi_fm(											/*»Display*/
                            UINT32* pRssi
                      );


BOOL OPENAT_close_fm(											/*¹Ø±ÕFM*/
                            void
                    );


/*******************************************
**               AT COMMAND               **
*******************************************/
BOOL OPENAT_init_at(                                            /*ÐéÄâATÍ¨Â·³õÊ¼»¯½Ó¿Ú*/
                            PAT_MESSAGE pAtMessage              /*Athistry ¢ د. Øμ ÷ º¯heý*/
                   );



BOOL OPENAT_send_at_command(                                    /*·¢ËÍATÃüÁî½Ó¿Ú*/
                            UINT8 *pAtCommand,                  /*Atãüeî*/
                            UINT16 nLength                      /*AtÃüeî³¤¶¤¶ гive*/
                           );



/*+\NEW WM-733\xc\2013.04.19\ÐÞ¸Ä¼ÓÃÜ¿¨Á÷³Ì(Ìn¼Óopenat´æÈ¡½Ó¿Ú)*/
/*******************************************
**               ¼ÓÃÜ¿¨ÉèÖÃ               **
*******************************************/
BOOL OPENAT_set_encinfo(                         /*ÉèÖÃÃÜÔ¿ÐÅÏ¢*/
                        UINT8 *encInfo,
                        UINT32 len
              );


BOOL OPENAT_get_encinfo(                         /*¶ ¢ ¢*/
                        UINT8 *encInfo,
                        UINT32 len
              );



UINT8 OPENAT_get_encresult(                         /*¶Áè¡¼óãːð £ ñéle¹û*/
                        void
              );



/*+\NEW WM-733\xc\2013.05.06\ÐÞ¸Ä¼ÓÃÜ¿¨Á÷³Ì5(Ìn¼Ó»ñÈ¡¿¨ÀàÐÍµÄ½Ó¿Ú)*/
UINT8 OPENAT_get_cardtype(                         /*¶ÁÈ¡¿¨ÀàÐÍ 0Î´Öª  1¼ÓÃÜ¿¨  2ÆÕÍ¨¿¨*/
                        void
              );
/*-\NEW WM-733\xc\2013.05.06\ÐÞ¸Ä¼ÓÃÜ¿¨Á÷³Ì5(Ìn¼Ó»ñÈ¡¿¨ÀàÐÍµÄ½Ó¿Ú)*/


/*+\NEW WM-733\xc\2013.04.23\ÐÞ¸Ä¼ÓÃÜ¿¨Á÷³Ì2(ÓÃopenat½Ó¿Ú´úÌæatÉèÖÃÃÜÔ¿ÐÅÏ¢¡£Ìn¼ÓÐÅºÅÁ¿)*/
BOOL OPENAT_set_enc_data_ok(                         /*mmi × ± ± ¸ºº ¸ ¢ ¢ º º º º ¢ ¢*/
                        void
              );
/*-\NEW WM-733\xc\2013.04.23\ÐÞ¸Ä¼ÓÃÜ¿¨Á÷³Ì2(ÓÃopenat½Ó¿Ú´úÌæatÉèÖÃÃÜÔ¿ÐÅÏ¢¡£Ìn¼ÓÐÅºÅÁ¿)*/
/*+\NEW WM-733\xc\2013.04.19\ÐÞ¸Ä¼ÓÃÜ¿¨Á÷³Ì(Ìn¼Óopenat´æÈ¡½Ó¿Ú)*/



/*+\NEW\xiongjunqun\2014.04.02\µ÷ÕûTTSµÄ´úÂë*/
/*delete TTS¸ÄÓÃ·¢ËÍAT Ö¸ÁîµÄÐÎÊ½*/
/*-\NEW\xiongjunqun\2014.04.02\µ÷ÕûTTSµÄ´úÂë*/
/*+\ New Amopenate-91 \ Zhangyang \ 2013.11.19 \ ôö¼óusb Hid¹¦äü*/
void OPENAT_uhid_open(
        void OPENAT_handler(uint8 *, uint32));



void OPENAT_uhid_close(
        void);


int32 OPENAT_uhid_write(
        uint8 *data_p, 
        uint32 length);
/*-\ New Amopenate-91 \ Zhangyang \ 2013.11.19 \ ôö¼óusb Hid¹¦äü*/


/*+\NUFE\RUFE\2014.8.20\â‼ïgps´Ó»èµ*/
    /*******************************************
    **               RDAGPS                      **
    *******************************************/
BOOL OPENAT_Gps_open(
        T_AMOPENAT_RDAGPS_PARAM *cfg);

BOOL OPENAT_Gps_close(
        T_AMOPENAT_RDAGPS_PARAM *cfg);
/*-\NEW\RUFE\2014.8.20\â‼ngps´Ó¿Êèµ*/
    /*******************************************
    **                 DEBUG                  **
    *******************************************/
VOID OPENAT_print(                                              /*Trace logêä³ö½ó ¿ú*/
                            CHAR * fmt, ...
                 );

VOID OPENAT_openat_print(CHAR * fmt, ...);

VOID OPENAT_lua_print(CHAR * fmt, ...);

VOID OPENAT_openat_dump(char* head, char* hex, UINT32 len);


VOID OPENAT_assert(                                             /*¶ÏÑÔ½Ó¿Ú*/
                            BOOL condition,                     /*Ìõ¼þ*/
                            CHAR *func,                         /*º ïed*/
                            UINT32 line                         /*Đðên*/
                  );



VOID OPENAT_enable_watchdog(BOOL enable);                            /*´ò¿ª¿´ÃÅ¹·*/

/*+\NEW\xiongjunqun\2015.06.11\Ôö¼Ófactory½Ó¿Ú¿â*/
boolean OPENAT_factory_check_calib(void);
/*-\NEW\xiongjunqun\2015.06.11\Ôö¼Ófactory½Ó¿Ú¿â*/


void OPENAT_watchdog_restart(void);
/*+\ New \ Brezen \ 2016.03.03 \ rôö¼ówatchdogê¹äunu¿ú¿ú*/
BOOL OPENAT_watchdog_enable(BOOL enable, UINT16 count);
/*-\ new \ brezen \ 2016.03.03 \ rôö¼ówatchdogêőäunu¿ú¿ú*/


#ifdef HRD_SENSOR_SUPPORT
/***********************************************/
VOID OPENAT_hrd_sensor_start(void);
VOID OPENAT_hrd_sensor_close(void);
int OPENAT_hrd_sensor_getrate(void);
/***********************************************/
#endif

INT64 OPENAT_disk_free(int drvtype);
INT32 OPENAT_disk_volume(int drvtype);


/*+\BUG WM-656\lifei\2013.03.07\[OpenAT] ÐÞ¸ÄcustÇøÓò¼ì²éÌõ¼þ*/
#define OPENAT_CUST_VTBL_DEFUALT_MAGIC 0x87654321
/*-\BUG WM-656\lifei\2013.03.07\[OpenAT] ÐÞ¸ÄcustÇøÓò¼ì²éÌõ¼þ*/

typedef enum E_AMOPENAT_CUST_INIT_RESULT_TAG
{
    OPENAT_CUST_INIT_RES_OK,        /*¿Í »§³ìðò³õê¼» ¯³é¹¦ £ ¬¿Éòôµ ÷ óÃCust_Mainº¯êý*/
    OPENAT_CUST_INIT_RES_ERROR,     /*¿Í »§³ìðò³õê¼» ¯ê§ ° ü £ ¬² »» eµ ÷ óãcust_mainº¯êý*/
    OPENAT_CUST_INIT_RES_MAX
}E_AMOPENAT_CUST_INIT_RESUL;

/*+\BUG:3874\czm\2021.01.03\AT°æ±¾wifiscanÖ¸Áî£¬Ìn¼ÓÒ»¸ö²ÎÊýÖ§³ÖÒì²½É¨ÃèÄ£Ê½£¬Ä¬ÈÏÎªÍ¬²½*/
typedef enum{
	OPENAT_WIFI_SCAN_STOP,
	OPENAT_WIFI_SCAN_OPEN,
}openatWifiScanState;
/*-\BUG:3874\czm\2021.01.03\AT°æ±¾wifiscanÖ¸Áî£¬Ìn¼ÓÒ»¸ö²ÎÊýÖ§³ÖÒì²½É¨ÃèÄ£Ê½£¬Ä¬ÈÏÎªÍ¬²½*/

extern u16 lua_lcd_height;
extern u16 lua_lcd_width;

#define OPENAT_TICKS_TO_MILLSEC(t) ((UINT64)(t)*5)
#define OPENAT_TICKS_TO_SEC(t) ((UINT64)(t)*5/1000)

BOOL OPENAT_set_trace_port(UINT8 port, UINT8 usb_port_diag_output);
UINT8 OPENAT_get_trace_port(void);
/*+\ New \ wj \ 2018.10.10 \ È ¥ µôuses_nor_flashº*/
BOOL OPENAT_is_nor_flash(void);
UINT32 OPENAT_turn_addr(UINT32 addr);
/*-\ new \ wj \ 2018.10.10 \ è ¥ µôuses_nor_flashº*/

/*+\NEUW\NEW\Kingeransyaman\2019.4.19\¬*/
void OPENAT_rtos_sendok(char *src);
/*-labs*/
/*+\ Nw \ she \ sheenyuanyuan \ 2019.11.01 \ ¿¿¢ ¢ rtos.set_lua_info½ó¿ó¿----ºnat+luainfo £ £ üeî*/
void OPENAT_rtos_set_luainfo(char *src);
/*-\ nw \ she \ she \ sheenyuanyuan \ 2019.11.01.*/
/*+\NEW\WANGJERTY\2019.4.28\ôn ¼Ó»ôëëëënhëëëëë plum´ism*/
int OPENAT_set_band();
/*-\NEW\WANGJERTY\2019.4.28\ôn¬´n»ôÖôÔöôm»âââôë¶ëën.*/
/*+\NEW\wangyuan\2020.05.07\BUG_1126:Ö§³Öwifi¶èÎ»¹¦ÄÜ*/
void OPENAT_get_wifiinfo(OPENAT_wifiScanRequest* wifi_info);
/*-\NEW\wangyuan\2020.05.07\BUG_1126:Ö§³Öwifi¨Î»¹¦ÄÜ*/
void OPENAT_get_channel_wifiinfo(OPENAT_wifiScanRequest* wifi_info, uint32 channel);

#ifdef __AM_LUA_TTSPLY_SUPPORT__
BOOL OPENAT_tts_init(TTS_PLAY_CB fCb);
BOOL OPENAT_tts_set_param(OPENAT_TTS_PARAM_FLAG flag,u32 value);
BOOL OPENAT_tts_play(char *text,u32 len);
BOOL OPENAT_tts_stop();
#endif
#endif /* AM_OPENAT_H */

/*+\NEW\zhuwangbin\2020.05.14\Ìn¼Óopenat speex½Ó¿ Ú*/
BOOL openat_speexEncoderInit(void);
int  openat_speexEncode(short decoded[], int decoded_size, char *output, int output_size);
BOOL openat_speexEncoderDestroy(void);

BOOL openat_speexDecoderInit(void);
int openat_speexDecoder(char encoded[], int encoded_size, short output[], int output_size);
BOOL openat_speexDecoderDestroy(void);
/*-\NEW\zhuwangbin\2020.05.14\Ìn¼Óopenat speex½Ó¿¿ Ú*/

/*+\bug2767\zhuwangbin\2020.8.5\Ìn¼ÓÍâ²¿ paÉèÖÃ½Ó¿ Ú*/
BOOL OPENAT_ExPASet(OPENAT_EXPA_T * exPaCtrl);
/*-\bug2767\zhuwangbin\2020.8.5\Ìn¼ÓÍâ²¿½ paÉèÖÃ½Ó¿ Ú*/

/*+\new\zhuwangbin\2020.6.2\Ìn¼ÓÒôÆµ¹¦·ÅÀàÐÍÉèÖÃ½Ó¿ Ú*/
BOOL OPENAT_setpa(OPENAT_SPKPA_TYPE_T type);
OPENAT_SPKPA_TYPE_T OPENAT_getpa(void);
/*-\new\zhuwangbin\2020.6.2\Ìn¼ÓÒôÆµ¹¦·ÅÀàÐÍÉèÖÃ½Ó¿¿ Ú*/
/*+\BUG\ Langyuan\2020.06.10\BUG_1930:LuceOWhice d´¨´«ugâ€™tââ »â »²´¯ug´®¨²´¨ug¹´x*/
BOOL OPENAT_fs_mount_sdcard(void);
BOOL OPENAT_fs_format_sdcard(void);
BOOL OPENAT_fs_umount_sdcard(void);
/*-\BUG\ Lanches\2020.06.10\BUG_1930:S d´¨´«ugâ€™tââ »â »²´¯ug´®¨²´¨ug¹´x*/
/*+\BUG\wgyuan\2020.07.29663:Æââ‼²²²»µ2G CSDK´¢t_sebgg_sebug_mode´¿*/
VOID OPENAT_SetFaultMode(E_OPENAT_FAULT_MODE mode);
/*-\BUG\wgyuan\2020.07.29663:Æââ‼²²»ug2G CSDK¢´¢t_sebggh_sop_mode «»*/

