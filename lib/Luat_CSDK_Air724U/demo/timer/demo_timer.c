#include "string.h"
#include "iot_os.h"
#include "iot_debug.h"

typedef struct 
{
    UINT8 timer_name[20];
    UINT32 count;
    HANDLE timer;
    UINT32 period;
} T_DEMO_TIMER_PARAM;

#define timer_print iot_debug_print
#define TIMER_1S (1000)


T_DEMO_TIMER_PARAM g_timer1Param;
T_DEMO_TIMER_PARAM g_timer2Param;

HANDLE g_demo_timer1;
HANDLE g_demo_timer2;

VOID timer1_handle(void *pParameter);

VOID demo_timer_create(VOID)
{
    /*×¢: ug´¨ug¨ug¨. 
    * pFuncµLYSISISµ×Y”Ø¯º²²²², 
    * pPamamer ¿â€™s*/     
    timer_print("[timer] create timer1");
    g_demo_timer1 = iot_os_create_timer(timer1_handle, (PVOID)&g_timer1Param);

    timer_print("[timer] create timer2");
    g_demo_timer2 = iot_os_create_timer(timer1_handle, (PVOID)&g_timer2Param);
}


VOID demo_timer_start(VOID)
{
    memset(&g_timer1Param, 0, sizeof(T_DEMO_TIMER_PARAM));
    memset(&g_timer2Param, 0, sizeof(T_DEMO_TIMER_PARAM));
    
    memcpy(g_timer1Param.timer_name, "timer1", strlen("timer1"));
    g_timer1Param.count++;
    memcpy(g_timer2Param.timer_name, "timer2", strlen("timer2"));
    g_timer2Param.count++;

    g_timer1Param.timer = g_demo_timer1;
    g_timer2Param.timer = g_demo_timer2;
    g_timer1Param.period = 1*TIMER_1S;
    g_timer2Param.period = 2*TIMER_1S;

    timer_print("[timer] start timer1");
    iot_os_start_timer(g_demo_timer1, 1*TIMER_1S);

    timer_print("[timer] start timer2");
    iot_os_start_timer(g_demo_timer2, 2*TIMER_1S);
}

VOID demo_timer_stop_and_del(HANDLE handle)
{
    iot_os_stop_timer(handle);
    iot_os_delete_timer(handle);
}

VOID timer1_handle(void *pParameter)
{
    T_DEMO_TIMER_PARAM *timerParam = (T_DEMO_TIMER_PARAM *)pParameter;

    timer_print("[timer] name %s, count %d ", timerParam->timer_name, timerParam->count);

    // 4.¶¨Ê ± Æ ÷ 1ºn¶¨Ê ± Æ ÷ 2¸ ÷ Öø¸´5´Î, Í £ Ö¹² ¢ É¾³Ý¶¨Ê ± Æ ÷
    if (timerParam->count < 5) 
    {
        timerParam->count++;
        iot_os_start_timer(timerParam->timer, timerParam->period);
    }
    else
    {   
        timer_print("[timer] stop and del %s ", timerParam->timer_name);
        demo_timer_stop_and_del(timerParam->timer);
    }
}


void demo_timer_init(void)
{
    //1. ´´½¨¶¨Ê±Æ÷1ºÍ¶¨Ê±Æ÷2
    demo_timer_create();

    // 2. ¿.18 Ale ± æ ÷º. Is æ æ ÷ 2
    demo_timer_start();
}

int appimg_enter(void *param)
{    
    timer_print("[timer] appimg_enter");

    demo_timer_init();

    return 0;
}

void appimg_exit(void)
{
    timer_print("[timer] appimg_exit");
}

