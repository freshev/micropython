#ifndef PROTOCOL_JT808_H
#define PROTOCOL_JT808_H

#define JT_DATA_DEPART_FLAG      0x2000   //Subcontracting flag 0010 0000 0000 0000
#define JT_DATA_SEC_FLAG      0x1C00      //Encryption flag 0001 1100 0000 0000

typedef enum
{
    JT_CMD_TERM_ACK = 0x0001,   //Terminal general response
    JT_CMD_HEART_BEAT = 0x0002,   //Terminal heartbeat
    JT_CMD_UNREGISTER = 0x0003,   //Logout
    JT_CMD_PLAT_ACK = 0x8001,   //Platform general response
    JT_CMD_REGISTER = 0x0100,   //Terminal registration
    JT_CMD_REGISTER_ACK = 0x8100,   //Terminal registration response
    JT_CMD_ICCID = 0x0120,       //The terminal reports iccid
    JT_CMD_AUTH = 0x0102,        //Terminal authentication
    JT_CMD_SET_PARAM = 0x8103,   //Set terminal parameters
    JT_CMD_GET_PARAM = 0x8104,   //Query terminal parameters
    JT_CMD_TERM_CTL = 0x8105,   //Terminal control
    JT_CMD_GET_PARAM_ACK = 0x0104,   //Query terminal parameter response
    JT_CMD_LOCATE = 0x0200,   //Location information report
    JT_CMD_LOCATE_REQ = 0x8201,   //Location information query
    JT_CMD_LOCATE_REQ_ACK = 0x0201,   //Location information query response
    JT_CMD_LOCATE_MULTI = 0x704,   //Batch upload of positioning data
    JT_CMD_TEXT_RESULT = 0x6006,  // Terminal text data reporting
    JT_CMD_TEXT = 0x8300        //Send text information
}Jt808CmdType;

typedef enum
{
    JT_PARAM_HEART_INTERVAL = 0x0001,   //Terminal heartbeat sending interval, unit is (s)
    JT_PARAM_APN = 0x0010,   //Main server APN, wireless communication dial-up access point.
    JT_PARAM_APN_USR = 0x0011,   //Main server APN dial-up user name
    JT_PARAM_APN_PWD = 0x0012,   //Main server APN, dial-up password
    JT_PARAM_IP = 0x0013,   //Main server address, IP or domain name
    JT_PARAM_IP_BAK = 0x0017,   //Backup server address, IP or domain name
    JT_PARAM_PORT = 0x0018,   //Server TCP port
    JT_PARAM_LOCATE_TYPE = 0x0020,   //Location reporting strategy, 0: regular reporting; 1: regular interval reporting; 2: regular and regular interval reporting
    JT_PARAM_HEART_LOCATE_INTERVAL = 0x0027,   //Report time interval during sleep, unit is seconds (s), >0
    JT_PARAM_TIME_INTERVAL = 0x0029,   //Default time reporting interval, unit is seconds (s), >0
    JT_PARAM_DISTANCE_INTERVAL = 0x002C,   //Default distance reporting interval, unit is meter (m), >0
    JT_PARAM_TURN_ANGLE = 0x0030,   //Inflection point supplementary transmission angle, <180
    JT_PARAM_MAX_SPEED = 0x0055,   //Maximum speed, unit is kilometers per hour (km/h)
    JT_PARAM_EXCEED_SPEED_TIME = 0x0056,   //Overspeeding duration, unit is seconds (s)
    JT_PARAM_MILIAGE = 0x0080,   //Vehicle odometer reading, 1/10km
    JT_PARAM_PROVINCE = 0x0081,   //The province ID where the vehicle is located, 1~255
    JT_PARAM_CITY = 0x0082,   //City ID where the vehicle is located, 1~255
    JT_PARAM_BRAND = 0x0083,   //Motor vehicle number plate issued by the public security, transportation and management department
    JT_PARAM_BRAND_COLOR = 0x0084,   //License plate color, in accordance with the provisions of 5.4.12 in JT/T415-2006
}Jt808ParamType;

/*Used to record the sequence number of the message sent, the sequence number of the message received, etc.*/
typedef struct
{
    uint16_t msg_serial;  //Upload data sequence number, add one each time you send a message
    uint16_t server_serial;  //data sequence number
    uint16_t msg_id;     // message number
    uint8_t msg_result;     // Message number 0: success/confirmation; 1: failure; 2: wrong message; 3: not supported; 4: alarm processing confirmation
}Jt808MsgSave;

#endif
