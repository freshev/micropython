#ifndef PARAM_CTRL_H
#define PARAM_CTRL_H

#include "luat_rtos.h"

#define  JT808_PROTOCOL_PARAM "/jt808_param.txt"
#define  JT808_PROTOCOL_DEVICE_TYPE "XXXXXX"

typedef enum
{
    CFG_HEART_INTERVAL,                         //Terminal heartbeat sending interval
    CFG_APN_NAME,                               //Main server APN
    CFG_APN_USER,                               //Main server APN dial-up user name
    CFG_APN_PWD,                                //Main server APN, dial-up password
    CFG_SERVERLOCK,                             //Main server lock
    CFG_SERVERADDR,                             //Main server address
    CFG_SERVERADDR_PORT,                        //Main server port number
    CFG_JT_HBINTERVAL,                          //Report time interval during sleep
    CFG_UPLOADTIME,                             //Default time reporting interval
    CFG_UPLOADTIME_BKP,                         //Default time reporting interval backup
    CFG_TURN_ANGLE,                             //Inflection point supplementary transmission angle
    CFG_SPEED_ALARM_ENABLE,                     //Speed   alarm enable
    CFG_SPEEDTHR,                               //maximum speed
    CFG_SPEED_CHECK_TIME,                       //Overspeed duration
    CFG_JT_MILIAGE,                             //Vehicle odometer reading
    CFG_JT_PROVINCE,                            //The province where the vehicle is located
    CFG_JT_CITY,                                //The city where the vehicle is located
    CFG_JT_OEM_ID,                              //Terminal manufacturer number
    CFG_DEVICETYPE,                             //Terminal model
    CFG_JT_DEVICE_ID,                           //Terminal ID
    CFG_JT_VEHICLE_NUMBER,                      //Motor vehicle number plate issued by the public security, transportation and management department
    CFG_JT_VEHICLE_COLOR,                       //License plate color
    CFG_JT_AUTH_CODE,                           //Terminal authentication data
    CFG_JT_ISREGISTERED,                        //Terminal registration flag
    CFG_LANG,                                   //Response language after receiving instructions sent by the network, (0:   English, 1: Chinese) default Chinese
    CFG_TIME_ZONE,                              //time zone
    CFG_APNC_MCC,                               //Move country code
    CFG_APNC_MNC,                               //mobile network code
    CFG_RELAY,                                  //Oil-electric control circuit
    CFG_GPS,                                    //GPS switch
    CFG_PROTOCOL_NUM,                           //Device protocol number
    CFG_FORTIFICAT_MODE,                        //Fortification mode (0: automatic, 1: manual)
    CFG_BASE_UPLOAD_ENABLE,                     //Base station upload switch
    CFG_BASE_UPLOAD_INTERVAL,                   //Base station upload interval
    CFG_BASE_WAIT_GPS_TIME,                     //Wait for GPS positioning time
}CmdType;

typedef enum
{
    TYPE_BYTE,
    TYPE_SHORT,
    TYPE_BOOL,
    TYPE_STRING,
}ParamType;

typedef enum
{
    UNCHARGED,             //Not charged
    CHARGING,              //Charging
    CHARGEFULL,            //full
}ChargeType;

typedef struct
{
    uint8_t addr[40];                           //server address
    uint32_t port;                              //server port
    uint16_t province;                          //provincial area
    uint16_t city;                              //City area
    uint8_t oemid[5];                           //Terminal manufacturer number
    uint8_t devicetype[8];                      //Terminal model
    uint8_t deviceid[7];                        //Terminal ID
    uint8_t vehiclecolor;                       //License plate color
    uint8_t vehiclenumber[12];                  //Motor vehicle number plate issued by the public security, transportation and management department
    uint16_t uploadtime;                        //Default time reporting interval
    uint16_t heart_interval;                    //Terminal heartbeat sending interval
    uint8_t language;                           //Response language after receiving instructions sent by the network, (0:   English, 1: Chinese) default Chinese
    uint8_t timezone;                           //Time zone, East 8th zone: 8, West 8th zone: 58, West time zone is distinguished by adding 50 in front by default, omitting the symbol
    uint8_t protocol_num;                       //Device protocol number
    uint8_t fortificat_mode;                    //defense mode
    uint8_t base_upload_control;                //Base station upload switch
    uint8_t base_upload_time;                   //Base station upload interval
    uint8_t base_wait_gps_time;                 //Wait for GPS positioning time

    uint32_t miliage;                           //Vehicle odometer reading
    uint8_t apn_name[20];                       //Main server APN
    uint8_t apn_user[30];                       //Main server APN dial-up user name
    uint8_t apn_pwd[20];                        //Main server APN, dial-up password
    uint16_t hbinterval;                        //Report time interval during sleep
    uint16_t turn_angle;                        //Inflection point supplementary transmission angle
    uint8_t speed_enable;                       //Overspeed alarm enable
    uint8_t speedthr;                           //maximum speed
    uint8_t speed_check_time;                   //Overspeed duration
    uint8_t auth_code[33];                      //Authentication data
    uint8_t isregistered;                       //Registration flag
    uint16_t apnc_mcc;                          //Move country code
    uint8_t apnc_mnc[2];                        //mobile network code
    uint8_t relay_control;                      //Oil and electricity control circuit (0: oil and electricity on, 1: oil and electricity off)
    uint8_t gps_control;                        //GPS switch
}Jt808Msg;

void config_service_get(CmdType cmd, ParamType type, uint8_t *data, uint32_t len);
void config_service_set(CmdType cmd, ParamType type, uint8_t *data, uint32_t len);
int config_relay_set(int status);
int config_relay_get(void);
int config_gps_set(int status);
int config_gps_get(void);
int config_vbat_get(void);
int config_input_volt_get(void);
int config_charge_get(void);
int config_accelerated_speed_set(int status);

#endif
