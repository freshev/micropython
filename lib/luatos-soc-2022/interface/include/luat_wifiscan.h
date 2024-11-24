#ifndef _LUAT_WIFISCAN_H_
#define _LUAT_WIFISCAN_H_

#define Luat_MAX_CHANNEL_NUM     14

#include "luat_base.h"
/**
 * @defgroup luat_wifiscan wifiscan scanning interface
 * @{*/

/// @brief wifiscan scanning priority
typedef enum luat_wifiscan_set_priority
{
    LUAT_WIFISCAN_DATA_PERFERRD=0,/**< Data first*/
    LUAT_WIFISCAN_WIFI_PERFERRD
}luat_wifiscan_set_priority_t;

/// @brief wifiscan control parameter structure
typedef struct luat_wifiscan_set_info
{
    int   maxTimeOut;         //ms, maximum execution time, value range 4000~255000
    uint8_t   round;              //wifiscan total round value range 1~3
    uint8_t   maxBssidNum;        //wifiscan max report num value range 4~40
    uint8_t   scanTimeOut;        //s, max time of each round executed by RRC value range 1~255
    uint8_t   wifiPriority;       //CmiWifiScanPriority
    uint8_t   channelCount;       //channel count; if count is 1 and all channelId are 0, UE will scan all frequecny channel
    uint8_t   rsvd[3];
    uint16_t  channelRecLen;      //ms, max scantime of each channel
    uint8_t   channelId[Luat_MAX_CHANNEL_NUM];          //channel id 1-14: scan a specific channel
}luat_wifiscan_set_info_t;


#define LUAT_MAX_WIFI_BSSID_NUM      40 ///<Maximum number of bssid
#define LUAT_MAX_SSID_HEX_LENGTH     32 ///<Maximum length of SSID

/// @brief wifiscan scan results
typedef struct luat_wifisacn_get_info
{
    uint8_t   bssidNum;                                   /**<wifi number*/
    uint8_t   rsvd;
    uint8_t   ssidHexLen[LUAT_MAX_WIFI_BSSID_NUM];        /**<length of SSID name*/
    uint8_t   ssidHex[LUAT_MAX_WIFI_BSSID_NUM][LUAT_MAX_SSID_HEX_LENGTH]; /**<SSID name*/
    int8_t    rssi[LUAT_MAX_WIFI_BSSID_NUM];           /**<rssi*/
    uint8_t   channel[LUAT_MAX_WIFI_BSSID_NUM];        /**<record channel index of bssid, 2412MHz ~ 2472MHz correspoding to 1 ~ 13*/ 
    uint8_t   bssid[LUAT_MAX_WIFI_BSSID_NUM][6];       /**<mac address is fixed to 6 digits*/ 
}luat_wifisacn_get_info_t;

/**
 * @brief Get wifiscan information
 * @param set_info[in] Set the parameters to control wifiscan
 * @param get_info[out] wifiscan scan results
 * @return int =0 success, others failure*/
int32_t luat_get_wifiscan_cell_info(luat_wifiscan_set_info_t * set_info,luat_wifisacn_get_info_t* get_info);

/**
 * @brief Get wifiscan information
 * @param set_info[in] Set the parameters to control wifiscan
 * @return int =0 success, others failure*/
int luat_wlan_scan_nonblock(luat_wifiscan_set_info_t * set_info);
/** @}*/

#endif
