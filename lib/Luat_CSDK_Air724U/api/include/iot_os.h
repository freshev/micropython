#ifndef  __IOT_OS_H__
#define  __IOT_OS_H__



#include "am_openat.h"
#include "am_openat_fs.h"
#include "am_openat_system.h"
#include "am_openat_drv.h"


/**
 * @defgroup iot_sdk_os ²Ù×÷ÏµÍ³½Ó¿Ú
 * @{*/

/**
 * @Defgroup Uneß³i½i¯u¿aiêcoêcoêcuıêcuıaiêi¯àðeð ï ïß³o½óliLuL½óliāu
 * @ {*/
/**@Example of/Demo_os.c
* OS½O ° ° ° àý*/ 
/**´´½¨ïß³ì
*@Note NPriRityöµµä · µ »Øôú0-20, Öµô´óóåèrse@
*@param ptaskentry: ïß³ìö ÷ º thousand
*@param pparameter: × ÷ îª²îêý´ «µýçãoß³ìö ÷ ° º thousand
*@param nstacksize: ïß³ìõ »
*@param npriory: ïß³ìóåèrse§ £ ¬thão²îêýô´ó £ ¬ïß³ìóåèrse@
*@param ncreationflags: ïß³ìæô¶ship ± Ex -£ ¬ ¬ Çë²î ‘¼àpenat_os_creation_flag
*@param ptasknama: ïß³ìãû³æ
*@Return Handle: ´´½__³é¹¦ · µ »Øïß³ìì¾ä ± u
**/
HANDLE iot_os_create_task(                         
                            PTASK_MAIN pTaskEntry,  
                            PVOID pParameter,         
                            UINT32 nStackSize,      
                            UINT8 nPriority,       
                            UINT16 nCreationFlags,     
                            PCHAR pTaskName       
						);


/**É¾³ýïß³ì
*@param httask: ïß³ìì¾ä ± u
*@Return Ture: It is interesting
* False: It is interesting
**/	
BOOL iot_os_delete_task(                           
                        HANDLE hTask        
                   );	

/** This¹òðurishes
* @ Param Htask: IUth³ni linguä ± ß
* @ Fourtext "¹ò Maryður³ì³ì
* FIGHTSE: I'm ¹ ¹ üUREß³ioß³ìê§n. Ü
**/
BOOL iot_os_suspend_task(                        
                            HANDLE hTask           
                        );

/**»Öçoß³ì
*@param httask: ïß³ìì¾ä ± u
*@Return Ture: »Öçoß³ì³é¹¦
* FALSE: »Öçoß³ìê§ ° ü
**/
BOOL iot_os_resume_task(                         
                        HANDLE hTask         
                   );

/** »Ñè¡μ ± ç ° I had allßuce
* @etot Hande: · AFSTROUNS: · holdt · · DOWNET, · · · · · D'IØ ± ateß³ì3 ± ±
*
**/				   
HANDLE iot_os_current_task(                
                            VOID
                          );	

/**»Ñè¡ ± ° ïß³ì´´½ ~ ¢åï ¢
*@param httask: ïß³ìì¾ä ± u
*@param ptaskinfo: ïß³ììðåïïï ´æ´ ½O
*@Return Ture: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_os_get_task_info(                        
                            HANDLE hTask,           
                            T_AMOPENAT_TASK_INFO *pTaskInfo 
                         );			  
/** @}*/ 

/**
 * @defroup ïû¿º ïý¿º ïï¢¢¿º ¢¢¿º¢¯¿º
 * @ {*/

/**»Ñè¡ß³ìïûï ¢
*@Note »and × èèû
*@param httask: ïß³ìì¾ä ± u
*@param ppissage: ´æ´ ¢ ïûï ¢ öçoë
*@Return Ture: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_os_wait_message(                         
						HANDLE hTask,          
						PVOID* ppMessage      
					);
					
/**· ¢ ënïß³ìïûï ¢
*@Note ìnà ½ïï ¢ ¶óeðî² °
*@param httask: ïß³ìì¾ä ± u
*@param pmesage: ´æ´ ¢ ïûï ¢ öçoë
*@Return Ture: ³é¹¦
* FALSE: § ° ü
**/					
BOOL iot_os_send_message(                         
						HANDLE hTask,          
						PVOID pMessage         
					);

/**¼ì²âïûï ¢ ¶óeðöðêç · ñóðïûï ¢
*@param httask: ïß³ìì¾ä ± u
*@Return Ture: ³é¹¦
* FALSE: § ° ü
**/								  
BOOL iot_os_available_message(                   
						HANDLE hTask       
						 );

/**· ¢ ëndaßóåèrse§ßß³ìïûï ¢
*@Note ìn¼óµ½ïûï ¢ ¶óeðn · ² ”
*@param httask: ïß³ìì¾ä ± u
*@param pmesage: ´æ´ ¢ ïûï ¢ öçoë
*@Return Ture: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_os_send_high_priority_message(          
                        HANDLE hTask,          
                        PVOID pMessage         
                                  );

/**¼ì²âïûï ¢ ¶óeðöðêç · ñóðïûï ¢
*@param httask: ïß³ìì¾ä ± u
*@Return Ture: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_os_available_message(                     
                        HANDLE hTask           
                         );

/** @}*/ 

/**
 * @Defgroup ê ± ¼Ä¶¨Ê ± Æ ÷ ½Ó¿úº¯ÊýÀÀÐÍ Ê ± ¼Ä¶¨Ê ± Æ ÷ ½Ó¿úº¯Êý
 * @{*/

/**@example timer/demo_timer.c
* timer½Ó¿ÚÊ¾Àý*/ 


/**´´½ ¶¨ê ± æ ÷
*@param pfun: ¶ê ± æ ÷ µ½ê ± ´¦ànºrseý
*@param pparameter: × ÷ îª²îêý´ «µýçãoê ± æ æ µ½ê ± ´¦àn °rseý
*@Return Handle: · µ »Ø¶ê ± æ ÷ ¾ä ± u
*			
**/	
HANDLE iot_os_create_timer(                         
						PTIMER_EXPFUNC pFunc,  
						PVOID pParameter       
					  );
					  
/**Æô¶ thousands ± æ ÷
*@param htimer: ¶ê ± æ ÷ ¾ä ± £ ¬Create_timer½ó · · µ »Øööµ
*@param nmillisecondes: ¶ê ± æ ÷ ± ¼ä
*@Return Ture: ³é¹¦
* FALSE: § ° ü
**/								  
BOOL iot_os_start_timer(                            /*Æô¶¯¶¨Ê±Æ÷½Ó¿Ú*/
						HANDLE hTimer,          /*¶¨Ê ± Æ ÷ ¾Ä ± Ú £ ¬CREATE_TIMER½Ó¿Ú · µ »øöµ*/
						UINT32 nMillisecondes   /*  */
				   );
				   
/**In £ Ö¹¶¨Ê ± Æ ÷
*@param htimer: ¶¨Ê ± Æ ÷ ¾Ä ± Ú £ ¬Create_timer½ó¿ú · µ »øöµ
*@Return Ture: ³é¹¦
* FALSE: Ê§ ° ü
**/						   
BOOL iot_os_stop_timer(                             
						HANDLE hTimer   
				  );
				  
/**É¾³ýases ± æ ÷
*@param htimer: ¶ê ± æ ÷ ¾ä ± £ ¬Create_timer½ó · · µ »Øööµ
*@Return True: ³é¹¦
* FALSE: § ° ü
**/					  
BOOL iot_os_delete_timer(                           
						HANDLE hTimer           
					);
					
/**¼ì²égar ± æ ÷ Êç · ñòñ¾æôrio thousand
*@param htimer: ¶ê ± æ ÷ ¾ä ± £ ¬Create_timer½ó · · µ »Øööµ
*@Return Ture: ³é¹¦
* FALSE: § ° ü
**/					
BOOL iot_os_available_timer(            
						HANDLE hTimer          
					   );
					   
					   
/**»Ñè¡µn³ê ± ¼ä
*@stop pdatetime:
*@Return Ture: ³é¹¦
* FALSE: § ° ü
**/	
BOOL iot_os_get_system_datetime(                  
						T_AMOPENAT_SYSTEM_DATETIME* pDatetime
					   );
					   
/**Éèöãïµn³ê ± ¼ä
*@stop pdatetime:
*@Return Ture: ³é¹¦
* FALSE: § ° ü
**/						   
BOOL iot_os_set_system_datetime(                    
						T_AMOPENAT_SYSTEM_DATETIME* pDatetime
					   );
/** @}*/  

/**
 * @Defgroup äööó½ó¿úº¯êýààðn äööó½ó¿úº¯êý
 * @{*/
/**@example demo_alarm/src/demo_alarm.c
* alarm½Ó¿ÚÊ¾Àý*/

/**Äööó³õêmpt
*@param pconfig: äöóóåäöã²îêý
*@Return Ture: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_os_init_alarm(                                      
                        T_AMOPENAT_ALARM_CONFIG *pConfig  
                   ); 

/**S
*@param paranmset: äööóéèöã²îýý
*@Return Ture: ³é¹¦
* FALSE: ê§ ° ü
**/
BOOL iot_os_set_alarm(                                        
                        T_AMOPENAT_ALARM_PARAM *pAlarmSet    
                   );
/** @}*/ 

/**
 * @Defgroup eù½Ç × ÊÔ´½Ó¿úº¯Êýààðn eÙ½Ç × ÊÔ´½Ó¿Úº¯ÊÝ
 * @{*/
 
/**...
*@return handle: · µ »ØeÙ½Ç × ÊÔ´ÇØ¾Ä ± Ú £ ¬
**/
HANDLE iot_os_enter_critical_section(               
                        VOID
                                );

/**ÍË´óöön×n×n×´ôôôâñ´âô´ôýýÓnøôôÚnön×ôâý×
*@params of heSion: Á××××ô×ôôâôâô´tô¬«±ñú
**/
VOID iot_os_exit_critical_section(             
                        HANDLE hSection        
                             );
 
/** ́1⁄4.
*@param ninnitCount:
*Oreturn HANDLE: ·
**/
HANDLE iot_os_create_semaphore(                     
                        UINT32 nInitCount       
                          );

/**É¾³ýðåºåe ° ° ° ° °
*@param hse:
*@Return Ture: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_os_delete_semaphore(                       
                        HANDLE hSem            
                        );

/**µè´ýðåº¿e ° ° ° °
*@param hse:
*@param ntimeout: µè´ýðåº ° ± ± ¼ä £ ¬if ntimeout <5ms, means forever
*@Return Ture: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_os_wait_semaphore(                        
                        HANDLE hSem,           
                        UINT32 nTimeOut         
                      );

/**Ên · åðåº¿e ° ° ° °
*@param hse:
*@Return Ture: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_os_release_semaphore(
                        HANDLE hSem            
                         );

/**»ñÈS
*@param hSem:
*@return:
**/
UINT32 iot_os_get_semaphore_value           
                        (
                        HANDLE hSem             
                        );
/** @}*/ 


/**
 * @Defgroup äú´æ½ó¿úº¯êýààðn äú´æ½ó¿úº¯êý
 * @{*/

/**ÄÚ´æÉêÇë½Ó¿Úmalloc
*@param		nSize:		 ÉêÇëµÄÄÚ´æ´óÐ¡
*@return	PVOID:       ÄÚ´æÖ¸Õë
**/
PVOID iot_os_malloc(                              
                        UINT32 nSize           
               );

/**ÄÚ´æÉêÇë½Ó¿Úrealloc
*@Param pmemory: äú´æö¸õë £ ¬malloc½óim · µ »Øörapy
*@Param nsize: éêçiltääú´goð¡
*@Return Pvoid: äú´æö¸õë
**/
PVOID iot_os_realloc(                               
                        PVOID pMemory,          
                        UINT32 nSize       
                );

/**Äú´æn · å½óoú
*@Param pmemory: äú´æö¸õë £ ¬malloc½óim · µ »Øörapy
**/
VOID iot_os_free(                                  
                        PVOID pMemory     
            );

/**»ñÈ¡¶Ñ¿Õ¼ä´óÐ¡
*@param		total:	     ×Ü¹²´óÐ¡
*@param   used:        ÒÑ¾­Ê¹ÓÃ
**/
VOID iot_os_mem_used(                                  
                        UINT32* total,
                        UINT32* used   
            );  

/** @}*/ 

/**
 * @defgroup æäëû½ó¿úº¯êýààðn æäëû½ó¿úº¯êý
 * @{*/

/**Ïµn³ëstone ° iana
*@param nmillisecondes: ërseßê ± ¼ä
*@Return Ture: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_os_sleep(                              
                        UINT32 nMillisecondes   
             );

/** »Альнална³tick½óly
* @ Return Tick_num: · о Øïμì³Ê ± ± ± ± ± ± ± ± ± ± ± ф¼rticköμ
**/
UINT64 iot_os_get_system_tick(                    
                        VOID
                         );

/** »Ñè¡ëÆÆ» Uêêy½ó¿ú
* @ Return Rand_num: · μ »øëæ» Ueuý
**/
UINT32 iot_os_rand(                              
                        VOID
              );

/**Éèöãëæ »uêýööö ×‘
*@param seed: ëæ »uêýöö × Ó
**/
VOID iot_os_srand(                                  
                        UINT32 seed            
             );

/**`` `` `
**/
VOID iot_os_shut_down(                            
                        VOID
                 );

/**ÖØÆô½Ó¿Ú
**/
VOID iot_os_restart(                              
                        VOID
               );

/**Éèöãtrace´òó —ú
*@param port: 0: uart1
                        1: uart2
                        2: Uart3
                        3: USB Modem
                        4: USB AP & UART HOSET × ¥ Log (ä¬èï)
*@Return Ture: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_os_set_trace_port(UINT8 port);


/**»ñÈ¡wifiscan²ÎÊý½Ó¿Ú
*@param		wifi_info:	wifiscan²ÎÊý     
**/
VOID iot_wifi_scan(OPENAT_wifiScanRequest* wifi_info);

/** @}*/ 

/** @}*/  //Ä£¿é½eÎ²


#endif

