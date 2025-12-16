/**************************************************************************
 *              Copyright (C), AirM2M Tech. Co., Ltd.
 *
 * Name:    platform_gps.c
 * Author:  zhutianhua
 * Version: V0.1
 * Date:    2014/8/6
 *
 * Description:
 *          lua gpscoreÆ½Ì¨½Ó¿ÚÊµÏÖ
 **************************************************************************/
#ifdef LUA_GPS_LIB

#include <stdio.h>
#include <string.h>
#include "lplatform.h"
#include "platform_gps.h"
#include "cycle_queue.h"
#include "am_openat_drv.h"


// take open ´ «èë¹ × ÷ ä £ ê½
//lua close
//lua write
//lua read
//lua dataind µ÷ÓÃ read


#define GPS_MAX_NMEA 30
#define GPS_NMEA_HEAD 2
#define GPS_MAX_NMEA_LEN (300)
#define GPS_DATA_IND_PERIOD (500)
#define GPS_DATA_IND_COUNT (sizeof(g_s_gps_indInfo)/sizeof(g_s_gps_indInfo[0]))
static CycleQueue g_s_gpsDataQueue;
static UINT8 g_s_gpsDataBuf[(GPS_MAX_NMEA_LEN+GPS_NMEA_HEAD)*GPS_MAX_NMEA];

static const char* g_s_gps_indInfo[] = 
{
  "PMTK010",
  "GGA",
  "RMC",
  "GSV",
  "GSA"
};

/*-\NEW\zhuwangbinning\2016.12.12\ Iâ JAS² ‘OWµµµµ*/
static UINT32 g_s_gps_indInfo_count[GPS_DATA_IND_COUNT] = {0};
/*-\NEW\zhuwangbinning\2016.12.12\ Iâ JAS² ‘OWµµµµ*/


BOOL gps_buf_write(VOID* buf, UINT32 len)
{
	/*----------------------------------------------------------------*/
  /* Local Variables                                                */
  /*----------------------------------------------------------------*/
  UINT16 head_flag = (UINT16)len;
  UINT16 freeSpace;
  UINT16 i;
  /*----------------------------------------------------------------*/
  /* Code Body                                                      */
  /*----------------------------------------------------------------*/
  ASSERT(GPS_MAX_NMEA_LEN >= len);

  for(i = 0; i < GPS_DATA_IND_COUNT; i++)
  {
		if(strstr(buf, g_s_gps_indInfo[i]) != NULL)
		{
			/*-\NEW\zhuwangbinning\2016.12.12\ Iâ JAS² ‘OWµµµµ*/
			g_s_gps_indInfo_count[i]++;
			/*-\NEW\zhuwangbinning\2016.12.12\ Iâ JAS² ‘OWµµµµ*/

			break;
		}
  }
  if(i == GPS_DATA_IND_COUNT)
  {
    OPENAT_print("platform_gps discard data %d:%s",
          len, buf);
    return FALSE;
  }

	/*-\NEW\zhuwangbinning\2016.12.12\ Iâ JAS² ‘OWµµµµ*/
  if (!((i == 0) || ((g_s_gps_indInfo_count[i] % 5) == 0)))
  {
		return;
  } 
	/*-\NEW\zhuwangbinning\2016.12.12\ Iâ JAS² ‘OWµµµµ*/

  freeSpace = QueueGetFreeSpace(&g_s_gpsDataQueue);

  if(freeSpace >= (len + sizeof(head_flag)))
  {
    QueueInsert(&g_s_gpsDataQueue, (UINT8*)&head_flag, 2);
    QueueInsert(&g_s_gpsDataQueue, buf, len);
  }
  else
  {
    OPENAT_print("platform_gps buffer has no free space = %d, ind len = %d ERROR",
          freeSpace, len);
  }
  return TRUE;
  
}
static void gps_buf_clean()
{
  QueueClean(&g_s_gpsDataQueue);
  g_s_gpsDataQueue.buf = g_s_gpsDataBuf;
  g_s_gpsDataQueue.size = sizeof(g_s_gpsDataBuf);
}

typedef struct OPENAT_INTERNAL_MSG_TAG
{
    void* message_body;
}OPENAT_INTERNAL_MSG;


/*-\ NEN„‐gunangbin\2010.10.12 \d Ö½t*/	
void platform_gps_open_ind_to_lua(void *buffer)
{
	PlatformMsgData msgData;

	msgData.gpsOpenInd.success = (UINT8)(UINT32)buffer;
	platform_rtos_send(MSG_ID_GPS_OPEN_IND, &msgData);
}

/*-\ NEN„‐gunangbin\2010.10.12 \d Ö½t*/
void mdi_gps_gps_open_ind_hdler(void *param)
{
	/*----------------------------------------------------------------*/
	/* Local Variables                                                */
	/*----------------------------------------------------------------*/
	OPENAT_INTERNAL_MSG* msg = param; 
  kal_uint32 result = (kal_uint32)(msg->message_body);

	OPENAT_print("gps_test result = %d, %x", (kal_uint32)result, (kal_uint32)msg->message_body);

	platform_gps_open_ind_to_lua((void *)result); 
}
/*-\ NEN„‐gunangbin\2010.10.12 \d Ö½t*/




static void gps_ind(E_AMOPENAT_DATA_TYPE type, VOID *buffer, UINT32 length)
{
  PlatformMsgData msgData;
#if 1  
  switch(type)
  {
    case OPENAT_GPS_PARSER_RAW_DATA:
    case OPENAT_GPS_UART_RAW_DATA:
      //»º´æ
      if(gps_buf_write(buffer, length))
      {
      / By · _ "ë¢ è ¢¢¢ ¢
      msgData.gpsData.dataMode = (UINT16)type;
      msgData.gpsData.dataLen = length;
      platform_rtos_send(MSG_ID_GPS_DATA_IND, &msgData);
      }
      break;
    case OPENAT_GPS_OPEN_IND:
	  break;
    default:
      // TODO::
      break;
  }
#else

  gps_buf_write(buffer, length);
  / By · _ "ë¢ è ¢¢¢ ¢
  msgData.gpsData.dataMode = (UINT16)type;
  msgData.gpsData.dataLen = length;
  platform_rtos_send(MSG_ID_GPS_DATA_IND, &msgData);

#endif
}
int platform_gps_open(UINT8 mode){

  E_AMOPENAT_GPS_WORK_MODE workMode;
  switch(mode)
  {
    case GPS_MODE_RAW_DATA:
      workMode = OPENAT_GPS_UART_MODE_RAW_DATA;
      break;
    case GPS_MODE_LOCATION:
      workMode = OPENAT_GPS_UART_MODE_LOCATION;
      break;
    default:

      workMode = OPENAT_GPS_UART_MODE_LOCATION_WITH_QOP;
      break;
  }
  gps_buf_clean();
  
  if(TRUE == OPENAT_OpenGPS(workMode, gps_ind))
  {
    return PLATFORM_OK;
  }
  else
  {
    return PLATFORM_ERR;
  }
} 

int platform_gps_close(void){
  if(TRUE == OPENAT_CloseGPS())
  {
    return PLATFORM_OK;
  }
  else
  {
    return PLATFORM_ERR;
  }
}

int platform_gps_read(UINT8* data)
{


  UINT16 dataValid;
  if(g_s_gpsDataQueue.empty)
  {
    return 0;
  }

  
  dataValid = g_s_gpsDataQueue.size - QueueGetFreeSpace(&g_s_gpsDataQueue);
  ASSERT(dataValid > GPS_NMEA_HEAD);

  // è obstruction
  QueueDelete(&g_s_gpsDataQueue, (UINT8*)&dataValid, 2);
  // è¡³öêýmber question
// assert (Len> = Data Alid); // Å
  ASSERT(dataValid <= GPS_MAX_NMEA_LEN);
  QueueDelete(&g_s_gpsDataQueue, (UINT8*)data, dataValid);
  
  return dataValid;
}


int platform_gps_write(VOID* buf, UINT32 len){
  UINT32 writen;
  if(TRUE == OPENAT_WriteGPS(buf, len, &writen))
  {
    return writen;
  }
  else
  {
    return PLATFORM_ERR;
  }
}


void platform_gps_init(void)
{
  OPENAT_InitGPS();
}


int platform_gps_cmd(T_PLATFORM_GPS_CMD cmd)
{
  if(TRUE == OPENAT_SendCmdGPS((T_OPENAT_GPS_CMD)cmd))
  {
    return PLATFORM_OK;
  }
  else
  {
    return PLATFORM_ERR;
  }
}
#endif

