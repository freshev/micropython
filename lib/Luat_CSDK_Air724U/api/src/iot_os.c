#include "iot_os.h"
#include "am_openat.h"
#include "am_openat_debug.h"

BOOL g_s_traceflag = FALSE;

/*******************************************
**                 SYSTEM                 **
*******************************************/

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
                          )
{
    HANDLE h = NULL;
    OPENAT_create_task(&h, pTaskEntry, pParameter, NULL, 
                nStackSize, nPriority, nCreationFlags, 10, pTaskName);
    if (h == NULL)
    {
        iot_debug_assert(0, __func__, __LINE__);
    }
    return h;
}
             
/**É¾³ýïß³ì
*@param httask: ïß³ìì¾ä ± u
*@Return Ture: It is interesting
* False: It is interesting
**/	
BOOL iot_os_delete_task(                           
                        HANDLE hTask            
                   )
{
    OPENAT_delete_task(hTask);
	return TRUE;
}

/** This¹òðurishes
* @ Param Htask: IUth³ni linguä ± ß
* @ Fourtext "¹ò Maryður³ì³ì
* FIGHTSE: I'm ¹ ¹ üUREß³ioß³ìê§n. Ü
**/
BOOL iot_os_suspend_task(                      
                            HANDLE hTask          
                        )
{
	return IVTBL(suspend_task)(hTask);
}

/**»Öçoß³ì
*@param httask: ïß³ìì¾ä ± u
*@Return Ture: »Öçoß³ì³é¹¦
* FALSE: »Öçoß³ìê§ ° ü
**/
BOOL iot_os_resume_task(                           
                        HANDLE hTask          
                   )
{
    return IVTBL(resume_task)(hTask);
}

/** »Ñè¡μ ± ç ° I had allßuce
* @etot Hande: · AFSTROUNS: · holdt · · DOWNET, · · · · · D'IØ ± ateß³ì3 ± ±
*
**/		
HANDLE iot_os_current_task(                         
                            VOID
                          )
{
    return OPENAT_current_task();
}

/**»Ñè¡ ± ° ïß³ì´´½ ~ ¢åï ¢
*@param httask: ïß³ìì¾ä ± u
*@param ptaskinfo: ïß³ììðåïïï ´æ´ ½O
*@Return Ture: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_os_get_task_info(                         
                            HANDLE hTask,         
                            T_AMOPENAT_TASK_INFO *pTaskInfo 
                         )
{
    return IVTBL(get_task_info)(hTask, pTaskInfo);
}




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
                    )
{
    int msgId;
    return OPENAT_wait_message(hTask, &msgId, ppMessage, 0);
}

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
                    )
{
    return OPENAT_send_message(hTask, 0, pMessage, 0);
}

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
                                  )
{
    return IVTBL(SendHighPriorityMessage)(hTask, 0, pMessage, 0);
}

/**¼ì²âïûï ¢ ¶óeðöðêç · ñóðïûï ¢
*@param httask: ïß³ìì¾ä ± u
*@Return Ture: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_os_available_message(                     
                        HANDLE hTask           
                         )
{
    return IVTBL(available_message)(hTask);
}


/**´´½ ¶¨ê ± æ ÷
*@param pfun: ¶ê ± æ ÷ µ½ê ± ´¦ànºrseý
*@param pparameter: × ÷ îª²îêý´ «µýçãoê ± æ æ µ½ê ± ´¦àn °rseý
*@Return Handle: · µ »Ø¶ê ± æ ÷ ¾ä ± u
*			
**/	
HANDLE iot_os_create_timer(                        
                        PTIMER_EXPFUNC pFunc,   
                        PVOID pParameter        
                      )
{
    return OPENAT_create_timerTask(pFunc, pParameter);
}

/**Æô¶ thousands ± æ ÷
*@param htimer: ¶ê ± æ ÷ ¾ä ± £ ¬Create_timer½ó · · µ »Øööµ
*@param nmillisecondes: ¶ê ± æ ÷ ± ¼ä
*@Return Ture: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_os_start_timer(                         
                        HANDLE hTimer,         
                        UINT32 nMillisecondes   
                   )
{
    return OPENAT_start_timer(hTimer, nMillisecondes);
}

/**In £ Ö¹¶¨Ê ± Æ ÷
*@param htimer: ¶¨Ê ± Æ ÷ ¾Ä ± Ú £ ¬Create_timer½ó¿ú · µ »øöµ
*@Return Ture: ³é¹¦
* FALSE: Ê§ ° ü
**/	
BOOL iot_os_stop_timer(                          
                        HANDLE hTimer
                    )
{
    return OPENAT_stop_timer(hTimer);
}

/**É¾³ýases ± æ ÷
*@param htimer: ¶ê ± æ ÷ ¾ä ± £ ¬Create_timer½ó · · µ »Øööµ
*@Return True: ³é¹¦
* FALSE: § ° ü
**/	
BOOL iot_os_delete_timer(                         
                        HANDLE hTimer          
                    )
{
    return OPENAT_delete_timer(hTimer);
}

/**¼ì²égar ± æ ÷ Êç · ñòñ¾æôrio thousand
*@param htimer: ¶ê ± æ ÷ ¾ä ± £ ¬Create_timer½ó · · µ »Øööµ
*@Return Ture: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_os_available_timer(                      
                        HANDLE hTimer         
                       )
{
    return IVTBL(available_timer)(hTimer);
}

/**»Ñè¡µn³ê ± ¼ä
*@stop pdatetime:
*@Return Ture: ³é¹¦
* FALSE: § ° ü
**/	
BOOL iot_os_get_system_datetime(                   
                        T_AMOPENAT_SYSTEM_DATETIME* pDatetime
                       )
{
    return OPENAT_get_system_datetime(pDatetime);
}

/**Éèöãïµn³ê ± ¼ä
*@stop pdatetime:
*@Return Ture: ³é¹¦
* FALSE: § ° ü
**/	
BOOL iot_os_set_system_datetime(                   
                        T_AMOPENAT_SYSTEM_DATETIME* pDatetime
                       )
{
    return OPENAT_set_system_datetime(pDatetime);
}

/**Äööó³õêmpt
*@param pconfig: äöóóåäöã²îêý
*@Return Ture: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_os_init_alarm(                                       
                        T_AMOPENAT_ALARM_CONFIG *pConfig 
                   )
{
    return IVTBL(InitAlarm)(pConfig);
}
				   
/**S
*@param paranmset: äööóéèöã²îýý
*@Return Ture: ³é¹¦
* FALSE: ê§ ° ü
**/
BOOL iot_os_set_alarm(                                        
                        T_AMOPENAT_ALARM_PARAM *pAlarmSet    
                   )
{
    return IVTBL(SetAlarm)(pAlarmSet);
}

/**...
*@return handle: · µ »ØeÙ½Ç × ÊÔ´ÇØ¾Ä ± Ú £ ¬
**/
HANDLE iot_os_enter_critical_section(            
                        VOID
                                )
{
    return OPENAT_enter_critical_section(); 
}
								
/**ÍË´óöön×n×n×´ôôôâñ´âô´ôýýÓnøôôÚnön×ôâý×
*@params of heSion: Á××××ô×ôôâôâô´tô¬«±ñú
**/
VOID iot_os_exit_critical_section(                
                        HANDLE hSection       
                             )
{
    OPENAT_exit_critical_section(hSection); 
}

/** ́1⁄4.
*@param ninnitCount:
*Oreturn HANDLE: ·
**/
HANDLE iot_os_create_semaphore(                   
                        UINT32 nInitCount     
                          )
{
    return OPENAT_create_semaphore(nInitCount); 
}

/**É¾³ýðåºåe ° ° ° ° °
*@param hse:
*@Return Ture: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_os_delete_semaphore(                      
                        HANDLE hSem            
                        )
{
    return OPENAT_delete_semaphore(hSem);  
}

/**µè´ýðåº¿e ° ° ° °
*@param hse:
*@param ntimeout: µè´ýðåº ° ± ± ¼ä £ ¬if ntimeout <5ms, means forever
*@Return Ture: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_os_wait_semaphore(                  
                        HANDLE hSem,           
                        UINT32 nTimeOut        
                      )
{
    return OPENAT_wait_semaphore(hSem, nTimeOut);
}

/**Ên · åðåº¿e ° ° ° °
*@param hse:
*@Return Ture: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_os_release_semaphore(
                        HANDLE hSem           
                         )
{
    return OPENAT_release_semaphore(hSem);
}

/**»ñÈS
*@param hSem:
*@return:
**/
UINT32 iot_os_get_semaphore_value (                  
                        HANDLE hSem            
                        )
{
    return IVTBL(get_semaphore_value)(hSem);
}

/**ÄÚ´æÉêÇë½Ó¿Úmalloc
*@param		nSize:		 ÉêÇëµÄÄÚ´æ´óÐ¡
*@return	PVOID:       ÄÚ´æÖ¸Õë
**/
PVOID iot_os_malloc(                                
                        UINT32 nSize           
               )
{
    return OPENAT_malloc(nSize);
}

/**ÄÚ´æÉêÇë½Ó¿Úrealloc
*@Param pmemory: äú´æö¸õë £ ¬malloc½óim · µ »Øörapy
*@Param nsize: éêçiltääú´goð¡
*@Return Pvoid: äú´æö¸õë
**/
PVOID iot_os_realloc(                            
                        PVOID pMemory,          
                        UINT32 nSize           
                )
{
    return OPENAT_realloc(pMemory, nSize);
}

/**Äú´æn · å½óoú
*@Param pmemory: äú´æö¸õë £ ¬malloc½óim · µ »Øörapy
**/
VOID iot_os_free(                                  
                        PVOID pMemory          
            )
{
    OPENAT_free(pMemory);
}

/**»ñÈ¡¶Ñ¿Õ¼ä´óÐ¡
*@param		total:	     ×Ü¹²´óÐ¡
*@param   used:        ÒÑ¾­Ê¹ÓÃ
**/
VOID iot_os_mem_used(                                  
                        UINT32* total,
                        UINT32* used   
            )
{
    IVTBL(MemoryUsed)(total, used);
} 

/**Ïµn³ëstone ° iana
*@param nmillisecondes: ërseßê ± ¼ä
*@Return Ture: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_os_sleep(                                 
                        UINT32 nMillisecondes  
             )
{
    return OPENAT_sleep(nMillisecondes);
}
			 
/** »Альнална³tick½óly
* @ Return Tick_num: · о Øïμì³Ê ± ± ± ± ± ± ± ± ± ± ± ф¼rticköμ
**/
UINT64 iot_os_get_system_tick(                   
                        VOID
                         )
{
    return OPENAT_get_system_tick();
}

/** »Ñè¡ëÆÆ» Uêêy½ó¿ú
* @ Return Rand_num: · μ »øëæ» Ueuý
**/
UINT32 iot_os_rand(                                
                        VOID
              )
{
    return OPENAT_rand();
}

/**Éèöãëæ »uêýööö ×‘
*@param seed: ëæ »uêýöö × Ó
**/
VOID iot_os_srand(                              
                        UINT32 seed           
             )
{
    IVTBL(srand)(seed);
}

/**`` `` `
**/
VOID iot_os_shut_down(                             
                        VOID
                 )
{
    IVTBL(shut_down)();
}

/**ÖØÆô½Ó¿Ú
**/
VOID iot_os_restart(                              
                        VOID
               )
{
    IVTBL(restart)();
}

/**Éèöãtrace´òó —ú
*@param port: 0: uart1
                        1: uart2
                        2: Uart3
                        3: USB Modem
                        4: USB AP & UART HOSET × ¥ Log (ä¬èï)
*@Return Ture: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_os_set_trace_port(UINT8 port)
{
	if((port > OPENAT_TRACE_QTY) || port < 0)
		return FALSE;
	if(4 == port)
	{
		g_s_traceflag = FALSE;
		return TRUE;
	}
	else
		g_s_traceflag = TRUE;
    return IVTBL(set_trace_port)(port,0);
}

/**»ñÈ¡wifiscan²ÎÊý½Ó¿Ú
*@param		wifi_info:	wifiscan²ÎÊý     
**/
VOID iot_wifi_scan(OPENAT_wifiScanRequest* wifi_info)
{
	return IVTBL(get_wifiinfo)(wifi_info);
}


