#ifndef __ANDLINK_H__
#define __ANDLINK_H__

#ifdef __cplusplus
extern "C"
{
#endif

#define PROTOCOL_VERSION    "V3.2" 

#define MAX_SSID_LEN        256
#define MAX_PASSWD_LEN      256
#define MAX_ENCRYPT_LEN     256

#define MAX_FUNCSTR_LEN     256
#define MAX_DEV_ID_LEN      256

// Distribution network mode
typedef enum{
    NETWOKR_MODE_WIRED = 1,     //Indicates wired distribution network equipment
    NETWOKR_MODE_WIFI = 2,      //Indicates WIFI distribution network equipment
    NETWOKR_MODE_4G = 3,        //Indicates 4G distribution network equipment
    NETWOKR_MODE_BT = 4,        // Represents Bluetooth distribution network device
    NETWOKR_MODE_OTHERS = 5,    //Indicates other distribution network equipment
    NETWOKR_MODE_MAX
}CFG_NET_MODE_e;

// WIFI control options (not used)
typedef enum{
    WIFI_OPT_STA_START = 1,     // Indicates turning on STA mode
    WIFI_OPT_STA_STOP = 2,      // Indicates turning off STA mode
    WIFI_OPT_AP_START = 3,      // Indicates turning on AP mode
    WIFI_OPT_AP_STOP = 4        // Indicates turning off AP mode
}WIFI_CTRL_OPT_e;

//Response type for device management and control operations
typedef enum{
    NORSP_MODE = 0,             // No response required
    ASYNC_MODE = 1,             // Asynchronous response, using devDataReport to respond
    SYNC_MODE = 2,              // Synchronous response, respond with the output parameters of the downlink control function. Generally, user equipment is connected to the local gateway, which is not commonly used.
}RESP_MODE_e;

//Device andlink status
typedef enum{
    ADL_BOOTSTRAP = 0,          // 0 device starts registration status
    ADL_BOOTSTRAP_SUC,          // 1 Device registration success status
    ADL_BOOTSTRAP_FAIL,         // 2 Device registration failed status
    ADL_BOOT,                   // 3 The device starts to go online
    ADL_BOOT_SUC,               // 4 Device online success status
    ADL_BOOT_FAIL,              // 5 Device online failure status
    ADL_ONLINE,                 // 6 Device online status
    ADL_RESET,                  // 7 device reset status
    ADL_OFFLINE,                // 8 Device offline status
    ADL_STATE_MAX
} ADL_DEV_STATE_e;

//The attribute information type of the device that is open to external query
typedef enum{
    ADL_AUTH_CODE,              // Get the device working secret key generated by one machine and one password;
    ADL_AUTH_DEVICE_ID,         // Get the unique device ID generated by one machine and one secret;
    ADL_USER_KEY                // Get userkey, the default value is 16 zeros
}EXPORT_DEVICE_ATTRS_e;

// WiFi control information, used to turn on STA mode; special instructions: when the ssid is "CMCC-QLINK", it means switching to a hidden, open qlink hotspot (not used)
typedef struct{
    char ssid[MAX_SSID_LEN];
    char password[MAX_PASSWD_LEN];
    char encrypt[MAX_ENCRYPT_LEN];
    int channel;
} wifi_cfg_info_t;

//Device properties
typedef struct{
    CFG_NET_MODE_e cfgNetMode;
    char *deviceType;           // The product type code of the device or sub-device registered in the developer portal (actually it is the product ID, yes, you can ignore the previous words, it is misleading)
    char *deviceMac;            //Device MAC, one-to-one correspondence with deviceId, format AABBCCDDEEFF, letters must be all capital letters
    char *andlinkToken;         //The platform token corresponding to the product type registered by the sub-device on the developer portal
    char *productToken;         // Product type token registered by the device on the developer portal
    char *firmWareVersion;      //Device firmware version number
    char *softWareVersion;      //Device software version number
    char *cfgPath;              // The file system path for SDK to store configuration files and logs. This path must be readable and writable.
    char *extInfo;              // Device extension information (DM information), required, json format is as follows
    /*{
        "cmei": "", // Unique identifier of the device, required
        "authMode": 0, // 0 represents type authentication, 1 represents device authentication, and authId and authKey must be used for device authentication.
        "authId": "", // Used to generate a working secret key, required for device authentication
        "authKey": "", // Used to generate a working secret key, required for device authentication
        "sn": "", // Device SN, required
        "reserve": "", // mark field, optional
        "manuDate": "yyyy-mm", // Equipment production date, format is year-month
        "OS": "", // operating system
        "chips": [ // Chip information, can include multiple groups
            {
                "type": "", // Chip type, such as Main/WiFi/Zigbee/BLE, etc.
                "factory": "", // chip manufacturer
                "model": "" // Chip model
            }
        ]
    }*/

}adl_dev_attr_t;

// Downlink control frame structure
typedef struct{
    char function[MAX_FUNCSTR_LEN];     //Download control command type
    char deviceId[MAX_DEV_ID_LEN];      // device ID
    char childDeviceId[MAX_DEV_ID_LEN]; //Subdevice ID
    int dataLen;                        //Control data length
    char *data;                         // Manage data content
} dn_dev_ctrl_frame_t;

typedef struct{
    //Control STA (not used)
    int (*ctrl_wifi_callback)(WIFI_CTRL_OPT_e opt, wifi_cfg_info_t *wificfg, char *outMsg, int msgBufSize);
    // Notify device status (important)
    int (*set_led_callback)(ADL_DEV_STATE_e state);
    //Downward control (important)
    int (*dn_send_cmd_callback)(RESP_MODE_e mode, dn_dev_ctrl_frame_t *ctrlFrame, char *eventType, char *respData, int respBufSize);
    // Download and upgrade version (unused)
    int (*download_upgrade_version_callback)(char *downloadrul, char *filetype, int chkfilesize);
    // Get the device IP (important, just follow the demo implementation)
    int (*get_device_ipaddr)(char *ip, char *broadAddr);
    //Reset device IP (unused)
    int (*reset_device_Ipaddr)(void);
    // Enable or disable listening to 802.11 broadcast frames (not used)
    int (*set_listen80211_callback)(int enable);
}adl_dev_callback_t;

//andlink initialization
int andlink_init(adl_dev_attr_t *devAttr, adl_dev_callback_t *devCbs);

//Device reset (called when restoring factory settings, used to clear the distribution network information and make the device re-enter the distribution network state) (not used)
int devReset(void);

// Message event reporting (active message reporting or receiving downstream control instructions for asynchronous response)
int devDataReport(char *childDevId, char *eventType, char *data, int dataLen);// eventType: Inform、ParamChange、File、Unbind

// Program dies (assumed to be called when the SDK needs to be uninstalled to release the corresponding resources of the SDK)
int andlink_destroy(void);

// // Log writing function
// int printLog(int fid, int logLevel, const char * fmt, ...);

// Query device-specific information (called when obtaining device-specific information) (generally not used)
char *getDeviceInfoStr(EXPORT_DEVICE_ATTRS_e attr);

#ifdef __cplusplus
}
#endif

#endif