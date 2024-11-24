#ifndef LUAT_WLAN_H
#define LUAT_WLAN_H

#include "luat_base.h"
#ifdef __LUATOS__
#include "luat_msgbus.h"
#include "luat_mem.h"
#endif
typedef struct luat_wlan_config
{
    uint32_t mode;
}luat_wlan_config_t;

typedef struct luat_wlan_conninfo
{
    char ssid[36];
    char password[64];
    char bssid[8];
    uint32_t authmode;
    uint32_t auto_reconnection;
    uint32_t auto_reconnection_delay_sec;
}luat_wlan_conninfo_t;

typedef struct luat_wlan_apinfo
{
    char ssid[36];
    char password[64];
    uint8_t gateway[4];
    uint8_t netmask[4];
    uint8_t channel;
    uint8_t encrypt;
    uint8_t hidden;
    uint8_t max_conn;
}luat_wlan_apinfo_t;

enum LUAT_WLAN_MODE {
    LUAT_WLAN_MODE_NULL,
    LUAT_WLAN_MODE_STA,
    LUAT_WLAN_MODE_AP,
    LUAT_WLAN_MODE_APSTA,
    LUAT_WLAN_MODE_MAX
};


enum LUAT_WLAN_ENCRYPT_MODE {
    LUAT_WLAN_ENCRYPT_AUTO,
    LUAT_WLAN_ENCRYPT_NONE,
    LUAT_WLAN_ENCRYPT_WPA,
    LUAT_WLAN_ENCRYPT_WPA2
};

typedef struct luat_wlan_scan_result
{
    char ssid[33];
    char bssid[6];
    int16_t rssi;
    uint8_t ch;
}luat_wlan_scan_result_t;

typedef struct luat_wlan_station_info
{
    uint8_t ipv4_addr[4];
    uint8_t ipv4_netmask[4];
    uint8_t ipv4_gateway[4];
    uint8_t dhcp_enable;
}luat_wlan_station_info_t;


int luat_wlan_init(luat_wlan_config_t *conf);
int luat_wlan_mode(luat_wlan_config_t *conf);
int luat_wlan_ready(void);
int luat_wlan_connect(luat_wlan_conninfo_t* info);
int luat_wlan_disconnect(void);
int luat_wlan_scan(void);
int luat_wlan_scan_get_result(luat_wlan_scan_result_t *results, size_t ap_limit);

int luat_wlan_set_station_ip(luat_wlan_station_info_t *info);

// Distribution network related
// --- smartconfig configuration network
enum LUAT_WLAN_SC_TYPE {
    LUAT_SC_TYPE_STOP = 0,
    LUAT_SC_TYPE_ESPTOUCH,
    LUAT_SC_TYPE_AIRKISS,
    LUAT_SC_TYPE_ESPTOUCH_AIRKISS,
    LUAT_SC_TYPE_ESPTOUCH_V2
};

int luat_wlan_smartconfig_start(int tp);
int luat_wlan_smartconfig_stop(void);

//data class
int luat_wlan_get_mac(int id, char* mac);
int luat_wlan_set_mac(int id, const char* mac);
int luat_wlan_get_ip(int type, char* data);
const char* luat_wlan_get_hostname(int id);
int luat_wlan_set_hostname(int id, const char* hostname);

//Set and get power saving mode
int luat_wlan_set_ps(int mode);
int luat_wlan_get_ps(void);

int luat_wlan_get_ap_bssid(char* buff);
int luat_wlan_get_ap_rssi(void);
int luat_wlan_get_ap_gateway(char* buff);

// AP class
int luat_wlan_ap_start(luat_wlan_apinfo_t *apinfo);
int luat_wlan_ap_stop(void);



/**
 * @defgroup luat_wifiscan wifiscan scanning interface
 * @{*/
#define Luat_MAX_CHANNEL_NUM     14
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
