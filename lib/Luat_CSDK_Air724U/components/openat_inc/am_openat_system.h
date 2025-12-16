/*********************************************************
  Copyright (C), AirM2M Tech. Co., Ltd.
  Author: lifei
  Description: AMOPENAT ¿ª·ÅÆ½Ì¨
  Others:
  History: 
    Version£º Date:       Author:   Modification:
    V0.1      2012.12.14  lifei     ´´½¨ÎÄ¼þ
*********************************************************/
#ifndef AM_OPENAT_SYSTEM_H
#define AM_OPENAT_SYSTEM_H

#include "am_openat_common.h"

/****************************** SYSTEM ******************************/
#define OPENAT_CUST_TASKS_PRIORITY_BASE 128
#define OPENAT_SEMAPHORE_TIMEOUT_MIN_PERIOD 5 //5ms
#define OPENAT_MSG_PROC_COUNT (30)

#define OPENAT_OS_SUSPENDED   (0xFFFFFFFF)






typedef enum E_AMOPENAT_OS_CREATION_FLAG_TAG
{
    OPENAT_OS_CREATE_DEFAULT = 0,   /*Ïß³ì´´½ £ ¬e ¢ ¼´æôrio*/
    OPENAT_OS_CREATE_SUSPENDED = 1, /*Ï³³i''le `£ £ ¬ineorð*/
}E_AMOPENAT_OS_CREATION_FLAG;

typedef struct T_AMOPENAT_TASK_INFO_TAG
{
    UINT16 nStackSize;
    UINT16 nPriority;
    CONST UINT8 *pName;
}T_AMOPENAT_TASK_INFO;

/*+\NEW\liweiqiang\2013.7.1\[OpenAt]Ôö¼ÓÏµÍ³Ö÷ÆµÉèÖÃ½Ó¿Ú*/
typedef enum E_AMOPENAT_SYS_FREQ_TAG
{
    OPENAT_SYS_FREQ_32K    = 32768,
    OPENAT_SYS_FREQ_13M    = 13000000,
    OPENAT_SYS_FREQ_26M    = 26000000,
    OPENAT_SYS_FREQ_39M    = 39000000,
    OPENAT_SYS_FREQ_52M    = 52000000,
    OPENAT_SYS_FREQ_78M    = 78000000,
    OPENAT_SYS_FREQ_104M   = 104000000,
    OPENAT_SYS_FREQ_156M   = 156000000,
    OPENAT_SYS_FREQ_208M   = 208000000,
    OPENAT_SYS_FREQ_250M   = 249600000,
    OPENAT_SYS_FREQ_312M   = 312000000,
}E_AMOPENAT_SYS_FREQ;
/*-\NEW\liweiqiang\2013.7.1\[OpenAt]Ôö¼ÓÏµÍ³Ö÷ÆµÉèÖÃ½Ó¿Ú*/

/****************************** TIME ******************************/
typedef struct T_AMOPENAT_SYSTEM_DATETIME_TAG
{
    UINT16 nYear;
    UINT8  nMonth;
    UINT8  nDay;
    UINT8  nHour;
    UINT8  nMin;
    UINT8  nSec;
    UINT8  DayIndex; /* 0=Sunday */
}T_AMOPENAT_SYSTEM_DATETIME;


typedef struct
{
  // uint8 alarminex;  /*Ö »Äüééöã1 cal*/
  bool                alarmOn; /* 1 set,0 clear*/
/*+\NEW administratorship UNFOF55.3.9\é´» REALY*/
  //E_AMOPENAT_ALARM_RECURRENT     alarmRecurrent;/*Ö»Ö§³Ö1¸öÄÖÖÓ*/
/*-\ENEW awards\2015.3.9\é´auté ONOUR REAR ONE*/
  T_AMOPENAT_SYSTEM_DATETIME alarmTime;
}T_AMOPENAT_ALARM_PARAM;

/****************************** TIMER ******************************/
#define OPENAT_TIMER_MIN_PERIOD 5 //5ms

typedef struct T_AMOPENAT_TIMER_PARAMETER_TAG
{
    HANDLE hTimer;      /*Create_timer ½ó¿;*/
    UINT32 period;      /*Start_time ½ó¿ú' ««YES NEXTINESCONS*/
    PVOID  pParameter;  /*create_timer ½Ó¿Ú´«ÈëµÄ pParameter*/
}T_AMOPENAT_TIMER_PARAMETER;

/*¶¨Ê ± Æ ÷ µ½Ê ± »Øµ ÷ º¯Êý £ ¬²ÎÊÝ PParaMeter Îªõ» ± Äe¿ö¸õË £ ¬¿n »§³ìÐòöð²» ÐÈÒªÊÍ · Å¸ÃÖ¸õË*/
typedef VOID (*PTIMER_EXPFUNC)(VOID *pParameter);

typedef VOID (*PMINUTE_TICKFUNC)(VOID);

typedef  void(*openat_msg_proc)(void *pParameter);


typedef VOID (*PTASK_MAIN)(PVOID pParameter);

typedef struct {
    uint8 *buf;
    uint32 size;        
    uint32 head;
    uint32 tail;
    unsigned empty: 1;
    unsigned full:  1;
    unsigned overflow:  1;  
}CycleQueue;

void QueueClean(CycleQueue *Q_ptr);

int QueueInsert(CycleQueue *Q_ptr, uint8 *data, uint32 len);

int QueueDelete(CycleQueue *Q_ptr, uint8 *data, uint32 len);

int QueueGetFreeSpace(CycleQueue *Q_ptr);

int QueueLen(CycleQueue *Q_ptr);



#endif /* AM_OPENAT_SYSTEM_H */

