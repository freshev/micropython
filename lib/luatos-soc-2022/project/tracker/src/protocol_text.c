#include <stdio.h>
#include <string.h>
#include "osasys.h"
#include "luat_fs.h"
#include "luat_rtos.h"
#include "luat_debug.h"
#include "luat_mem.h"
#include "protocol_text.h"
#include "param_ctrl.h"
#include "gpsmsg.h"

#define LUAT_HARD_SUPPORT 1

extern void soc_mobile_get_imsi(uint8_t *buf);
extern void soc_mobile_get_iccid(uint8_t *buf);

static protocol_text_t protocol_text[] =
{
    {"DEVICETYPE",DEVICETYPE},
    {"APN",APN},
    {"GETSERVER",GETSERVER},
    {"TIMER",TIMER},
    {"RESET",RESET},
    {"RELAY",RELAY},
    {"DYD",DYD},
    {"HFYD",HFYD},
    {"GPSON",GPSON},
    {"GPSOFF",GPSOFF},
    {"PARAM",PARAM},
    {"IMEI",IMEI},
    {"ICCID",ICCID},
    {"IMSI",IMSI},
    {"WHERE",WHERE},
    {"STATUS",STATUS},
    {"LBSON", LBSON},
    {"LBS",LBS},
    {"LANG",LANG},
    {"HBTS",HBTS},
    {"HBT",HBT},
    {"SPEED",SPEED},
    {"VERSION",VERSION},
    {"PROTOCOL",PROTOCOL},
    {"APNC", APNC},
    {"SENALM", SENALM},
};

//GPS positioning information
extern nmea_msg gpsx;

bool back_is_chinese = true;

protocol_text_cmd protocol_text_receive_data(uint8_t* src, uint16_t len, uint8_t* dest, protocol_text_type flag)
{
    uint8_t i = 0;
    protocol_text_cmd cmd = PROTOCOL_MAX;
    bool run_fail = false;
    LUAT_DEBUG_PRINT("protocol_text_receive_data data:%s ,len:%d", src, len);

    for (i=0; i<sizeof(protocol_text)/sizeof(protocol_text[0]); i++)
    {
        if(strstr(src,protocol_text[i].data))
        {
            cmd = protocol_text[i].cmd;
            break;
        }
    }
    LUAT_DEBUG_PRINT("protocol_text_receive_data cmd:%d", cmd);
    switch(cmd)
    {
        case RESET:
            if(0 == strcmp(src,"RESET#"))
                luat_pm_reboot(); //Restart the device
            break;
        case LANG:
            if(0 == strcmp(src,"LANG,0#"))//Set to English echo
            {
                uint8_t lang = 0;
                config_service_set(CFG_LANG, TYPE_BYTE, &lang, sizeof(lang));
                back_is_chinese = false;
                uint8_t data[] = "Exec Success";
                memcpy(dest,data,strlen(data));
            }
            else if (0 == strcmp(src,"LANG,1#"))//Set to Chinese echo
            {
                uint8_t lang = 1;
                config_service_set(CFG_LANG, TYPE_BYTE, &lang, sizeof(lang));
                back_is_chinese = true;
                uint8_t data[] = "Setup successful";
                memcpy(dest,data,strlen(data));
            }
            break;
        case IMEI:
            if(0 == strcmp(src,"IMEI#"))
            {
                uint8_t data[22] = {0};
                char imei[17] = {0};
                int result = 0;
                result = luat_mobile_get_imei(0, imei, 15);
                if (result)
                {
                    sprintf_(data, "IMEI:%s", imei);
                    memcpy(dest,data,strlen(data));
                }
            }
            break;
        case ICCID:
            if(0 == strcmp(src,"ICCID#"))
            {
                uint8_t pinState;
                uint16_t result = simGetPinStateSync(&pinState);
                if ((result == 0) && (pinState == 0))
                {
                    uint8_t data[31] = {0};
                    char iccid[24] = {0};
                    int result = 0;
                    soc_mobile_get_iccid(iccid);
                    if (iccid[0])
                    {
                        sprintf_(data, "ICCID:%s", iccid);
                        memcpy(dest,data,strlen(data));
                        LUAT_DEBUG_PRINT("ICCID get");
                    }
                }
            }
            break;
        case IMSI:
            if(0 == strcmp(src,"IMSI#"))
            {
                uint8_t pinState;
                uint16_t result = simGetPinStateSync(&pinState);
                if ((result == 0) && (pinState == 0))
                {
                    uint8_t data[26] = {0};
                    char imsi[20] = {0};
                    int result = 0;
                    soc_mobile_get_imsi(imsi);
                    if (imsi[0])
                    {
                        sprintf_(data, "IMSI:%s", imsi);
                        memcpy(dest,data,strlen(data));
                        LUAT_DEBUG_PRINT("IMSI get");
                    }
                }
            }
            break;
        case DEVICETYPE:
            if(0 == strcmp(src,"DEVICETYPE#"))
            {
                if(flag == ENGLISH)
                {
                    uint8_t data[30] = {0};
                    sprintf_(data, "Device type:%s", JT808_PROTOCOL_DEVICE_TYPE);
                    memcpy(dest,data,strlen(data));
                }
                else
                {
                    uint8_t data[30] = {0};
                    sprintf_(data, "Device model: %s", JT808_PROTOCOL_DEVICE_TYPE);
                    memcpy(dest,data,strlen(data));
                }
            }
            break;
        case GETSERVER:
            if(0 == strcmp(src,"GETSERVER#"))
            {
                uint8_t data[60] = {0};
                uint8_t server_addr[40] = {0};
                uint32_t server_port = 80;
                config_service_get(CFG_SERVERADDR, TYPE_STRING, server_addr, sizeof(server_addr));
                config_service_get(CFG_SERVERADDR_PORT, TYPE_BYTE, (uint8_t *)(&server_port), sizeof(uint32_t));

                if(flag == ENGLISH)
                {
                    sprintf_(data, "Server:%s", server_addr);
                    sprintf_(data + strlen(data), ":%d", server_port);
                    memcpy(dest,data,strlen(data));
                }
                else
                {
                    sprintf_(data, "Platform: %s", server_addr);
                    sprintf_(data + strlen(data), ":%d", server_port);
                    memcpy(dest,data,strlen(data));
                }
            }
            break;
        case RELAY:
        case DYD:
        case HFYD:
            if(0 == strcmp(src,"RELAY#"))
            {
                //Initialize to cut off oil and electricity (true: cut off oil and electricity/false: restore oil and electricity)
                bool relay_discnt = true;
                
                //HARD gets the oil and electricity status
                uint8_t relay = 0;
                #if LUAT_HARD_SUPPORT
                    int relay_t = config_relay_get();
                    relay = relay_t;
                #else
                    config_service_get(CFG_RELAY, TYPE_BYTE, &relay, sizeof(relay));
                #endif

                if (relay == 0)
                {
                    relay_discnt = false;
                }
                
                if(flag == ENGLISH)
                {
                    if (relay_discnt)
                    {
                        uint8_t data[] = "Relay:ON";
                        memcpy(dest,data,strlen(data));
                    }
                    else
                    {
                        uint8_t data[] = "Relay:OFF";
                        memcpy(dest,data,strlen(data));
                    }
                }
                else
                {
                    if (relay_discnt)
                    {
                        uint8_t data[] = "Oil and electricity status: disconnected";
                        memcpy(dest,data,strlen(data));
                    }
                    else
                    {
                        uint8_t data[] = "Oil and electricity status: connected";
                        memcpy(dest,data,strlen(data));
                    }
                }
            }
            else if (strstr(src, "RELAY,1#") || strstr(src, "RELAY,0#") || (0 == strcmp(src,"DYD#")) || (0 == strcmp(src,"HFYD#")))
            {
                //Initialize to cut off oil and electricity (true: cut off oil and electricity/false: restore oil and electricity)
                bool relay_discnt = true;
                if ((src[6] == '1') || (0 == strcmp(src,"DYD#")))
                {
                    relay_discnt = true;
                    #if LUAT_HARD_SUPPORT
                        //HARD sets the oil and electricity status
                        config_relay_set(1);
                    #endif
                }
                else if ((src[6] == '0') || (0 == strcmp(src,"HFYD#")))
                {
                    relay_discnt = false;
                    #if LUAT_HARD_SUPPORT
                        //HARD sets the oil and electricity status
                        config_relay_set(0);
                    #endif
                }
                
                uint8_t relay = 0;
                if (relay_discnt)
                {
                    relay = 1;
                }
                config_service_set(CFG_RELAY, TYPE_BYTE, &relay, sizeof(relay));
                
                if (flag == ENGLISH)
                {
                    if (relay_discnt)
                    {
                        uint8_t data[] = "Succeed to cut off the oil and circut.";
                        memcpy(dest,data,strlen(data));
                    }
                    else
                    {
                        uint8_t data[] = "Succeed to connect on the oil and circut.";
                        memcpy(dest,data,strlen(data));
                    }
                }
                else
                {
                    if (relay_discnt)
                    {
                        uint8_t data[] = "Oil and electricity cut off successfully";
                        memcpy(dest,data,strlen(data));
                    }
                    else
                    {
                        uint8_t data[] = "Oil and electricity successful";
                        memcpy(dest,data,strlen(data));
                    }
                }
            }
            break;
        case GPSON:
            if(0 == strcmp(src,"GPSON#"))
            {
                // HARD turns on GPS service
                uint8_t gps = 1;
                #if LUAT_HARD_SUPPORT
                    config_gps_set(1);
                #else
                    config_service_set(CFG_GPS, TYPE_BYTE, &gps, sizeof(gps));
                #endif

                if(flag == ENGLISH)
                {
                    uint8_t data[] = "Succeed to turn on GPS";
                    memcpy(dest,data,strlen(data));
                }
                else
                {
                    uint8_t data[] = "Successfully opened GPS";
                    memcpy(dest,data,strlen(data));
                }
            }
            break;
        case GPSOFF:
            if(0 == strcmp(src,"GPSOFF#"))
            {
                // HARD turns off GPS service
                uint8_t gps = 0;
                #if LUAT_HARD_SUPPORT
                    config_gps_set(0);
                #else
                    config_service_set(CFG_GPS, TYPE_BYTE, &gps, sizeof(gps));
                #endif

                if(flag == ENGLISH)
                {
                    uint8_t data[] = "Succeed to turn off GPS";
                    memcpy(dest,data,strlen(data));
                }
                else
                {
                    uint8_t data[] = "GPS turned off successfully";
                    memcpy(dest,data,strlen(data));
                }
            }
            break;
        case SPEED://TODO
            if(0 == strcmp(src,"SPEED#"))
            {
                // TODO Get overspeed alarm parameters
                uint8_t data[80] = {0};
                uint8_t cfg_speed_call = 1;
                uint8_t cfg_max_speed = 120;
                uint8_t cfg_speed_check_time = 15;

                config_service_get(CFG_SPEED_ALARM_ENABLE, TYPE_BYTE, &cfg_speed_call, sizeof(cfg_speed_call));
                config_service_get(CFG_SPEEDTHR, TYPE_BYTE, &cfg_max_speed, sizeof(cfg_max_speed));
                config_service_get(CFG_SPEED_CHECK_TIME, TYPE_BYTE, &cfg_speed_check_time, sizeof(cfg_speed_check_time));

                // LUAT_DEBUG_PRINT("call [%d] max [%d] check [%d]", cfg_speed_call, cfg_max_speed, cfg_speed_check_time);
                if(flag == ENGLISH)
                {
                    if (cfg_speed_call == 0)
                    {
                        sprintf_(data, "Speed alarm:disable,Check time:%d seconds", cfg_speed_check_time);
                    }
                    else
                    {
                        sprintf_(data, "Speed alarm:enable,Check time:%d seconds", cfg_speed_check_time);
                    }
                    sprintf_(data + strlen(data), ",Speed threshold:%dkm/h", cfg_max_speed);
                    memcpy(dest,data,strlen(data));
                }
                else
                {
                    if (cfg_speed_call == 0)
                    {
                        sprintf_(data, "Overspeed alarm switch: off, detection time: %d seconds", cfg_speed_check_time);
                    }
                    else
                    {
                        sprintf_(data, "Overspeed alarm switch: open, detection time: %d seconds", cfg_speed_check_time);
                    }
                    sprintf_(data + strlen(data), ",speeding threshold:%d kilometers/hour", cfg_max_speed);
                    memcpy(dest,data,strlen(data));
                }
            }
            else if(strstr(src, "SPEED,"))
            {
                //SPEED,1,15,120#
                //Speed   alarm
                bool speed_call = false;
                //Overspeed detection time
                int speed_check_time = 0;
                //speeding threshold
                int max_speed = 0;

                if (src[strlen(src) - 1] != '#')
                {
                    goto SETING_FAIL;
                }

                if (src[6] == '1')
                {
                    speed_call = true;
                }
                else if (src[6] == '0')
                {
                    speed_call = false;
                }

                char *token;
                char comma[] = ",";
                int index = 0;
                token = strtok(src, comma);
                while ( token != NULL )
                {
                    index += 1;
                    if (index == 3)
                    {
                        int token_len = strlen(token);
                        int ct_index = 0;
                        while (token[ct_index] != '\0')
                        {
                            int sum = 1;
                            for (int i = 0; (ct_index + i + 1) < token_len; i++)
                            {
                                sum *= 10;
                            }
                            speed_check_time += (token[ct_index++] - '0') * sum;
                        }
                    }
                    if (index == 4)
                    {
                        int token_len = strlen(token) - 1;
                        int ct_index = 0;
                        while ((token[ct_index] != '\0') && (token[ct_index] != '#'))
                        {
                            int sum = 1;
                            for (int i = 0; (ct_index + i + 1) < token_len; i++)
                            {
                                sum *= 10;
                            }
                            max_speed += (token[ct_index++] - '0') * sum;
                        }
                    }
                    token = strtok(NULL, comma);
                }

                uint8_t cfg_max_speed = max_speed;
                uint8_t cfg_speed_call = (speed_call ? 1 : 0);
                uint8_t cfg_speed_check_time = speed_check_time;

                config_service_set(CFG_SPEED_ALARM_ENABLE, TYPE_BYTE, &cfg_speed_call, sizeof(cfg_speed_call));
                config_service_set(CFG_SPEEDTHR, TYPE_BYTE, &cfg_max_speed, sizeof(cfg_max_speed));
                config_service_set(CFG_SPEED_CHECK_TIME, TYPE_BYTE, &cfg_speed_check_time, sizeof(cfg_speed_check_time));

                //TODO set speed alarm configuration

                if(flag == ENGLISH)
                {
                    uint8_t data[] = "Exec Success";
                    memcpy(dest,data,strlen(data));
                }
                else
                {
                    uint8_t data[] = "Setup successful";
                    memcpy(dest,data,strlen(data));
                }
            }
            break;
        case APNC:
            if (strstr(src, "APNC,"))
            {
                //APNC,460,02#
                uint16_t mcc = 0;
                int mcc_t = 0;
                uint8_t mnc[2] = {0};
                int str_len = strlen(src);

                if (str_len != 12)
                {
                    goto SETING_FAIL;
                }

                if (src[str_len - 1] != '#')
                {
                    goto SETING_FAIL;
                }

                char *token;
                char comma[] = ",";
                int index = 0;
                token = strtok(src, comma);
                while ( token != NULL )
                {
                    index += 1;
                    if (index == 2)
                    {
                        int token_len = strlen(token);
                        int ct_index = 0;
                        while (token[ct_index] != '\0')
                        {
                            int sum = 1;
                            for (int i = 0; (ct_index + i + 1) < token_len; i++)
                            {
                                sum *= 10;
                            }
                            mcc_t += (token[ct_index++] - '0') * sum;
                        }
                    }
                    if (index == 3)
                    {
                        memcpy(mnc, token, 2);
                    }
                    token = strtok(NULL, comma);
                }

                mcc = mcc_t;
                LUAT_DEBUG_PRINT("mcc: [%d], mnc: [%s]", mcc, mnc);

                config_service_set(CFG_APNC_MCC, TYPE_BYTE, &mcc, sizeof(mcc));
                config_service_set(CFG_APNC_MNC, TYPE_STRING, &mnc, sizeof(mnc));
                
                if(flag == ENGLISH)
                {
                    uint8_t data[] = "Exec Success";
                    memcpy(dest,data,strlen(data));
                }
                else
                {
                    uint8_t data[] = "Setup successful";
                    memcpy(dest,data,strlen(data));
                }
            }
            break;
        case WHERE:
            if (0 == strcmp(src, "WHERE#"))
            {
                uint8_t data[100] = {0};
                //direction
                float course = gpsx.course;
                //speed
                float speed = (gpsx.speed / 1000);
                //longitude
                float latitude = (gpsx.latitude / 100000);
                //latitude
                float longitude = (gpsx.longitude / 100000);
                //time
                nmea_utc_time time = gpsx.utc;
                if(flag == ENGLISH)
                {
                    sprintf_(data, "Latitude:%f,", latitude);
                    sprintf_(data + strlen(data), "Longitude:%f,", longitude);
                    sprintf_(data + strlen(data), "Time:%d-%02d-%02d %02d:%02d:%02d,", 
                    time.year, time.month, time.date, time.hour, time.min, time.sec);
                    sprintf_(data + strlen(data), "Speed:%fkm/h,", speed);
                    sprintf_(data + strlen(data), "Course:%.3f,", course);
                    memcpy(dest,data,strlen(data));
                }
                else
                {
                    sprintf_(data, "Latitude:%f,", gpsx.latitude);
                    sprintf_(data + strlen(data), "Longitude:%f,", longitude);
                    sprintf_(data + strlen(data), "Time:%d-%02d-%02d %02d:%02d:%02d,", 
                    time.year, time.month, time.date, time.hour, time.min, time.sec);
                    sprintf_(data + strlen(data), "Speed: %f kilometers/hour,", speed);
                    sprintf_(data + strlen(data), "Direction:%.3f degree,", course);
                    memcpy(dest,data,strlen(data));
                }
            }
            break;
        case APN:
            if(0 == strcmp(src,"APN#"))
            {
                uint8_t *data = (uint8_t *)LUAT_MEM_MALLOC(120);
                // uint32_t server_port = 80;

                uint8_t *apn_user = (uint8_t*)LUAT_MEM_MALLOC(30);
                uint8_t *apn_passwd = (uint8_t*)LUAT_MEM_MALLOC(20);
                uint8_t *apn_name = (uint8_t*)LUAT_MEM_MALLOC(20);

                config_service_get(CFG_APN_USER, TYPE_STRING, apn_user, sizeof(apn_user));
                config_service_get(CFG_APN_PWD, TYPE_STRING, apn_passwd, sizeof(apn_passwd));
                config_service_get(CFG_APN_NAME, TYPE_STRING, apn_name, sizeof(apn_name));
                // config_service_get(CFG_SERVERADDR_PORT, TYPE_BYTE, server_port, sizeof(server_port));

                if(flag == ENGLISH)
                {
                    sprintf_(data, "APN:%s", apn_name);
                    // sprintf_(data + strlen(data), ":%d", server_port);
                    sprintf_(data + strlen(data), "User Name:%s", apn_user);
                    sprintf_(data + strlen(data), "Password:%s", apn_passwd);
                    memcpy(dest,data,strlen(data));
                }
                else
                {
                    sprintf_(data, "APN name:%s", apn_name);
                    // sprintf_(data + strlen(data), ":%d", server_port);
                    sprintf_(data + strlen(data), "Username: %s", apn_user);
                    sprintf_(data + strlen(data), "Password: %s", apn_passwd);
                    memcpy(dest,data,strlen(data));
                }

                LUAT_MEM_FREE(data);
                LUAT_MEM_FREE(apn_user);
                LUAT_MEM_FREE(apn_passwd);
                LUAT_MEM_FREE(apn_name);
            }
            else if(strstr(src,"APN,"))
            {
                uint8_t apn_user[30] = {0};
                uint8_t apn_passwd[20] = {0};
                uint8_t apn_name[20] = {0};

                if (src[strlen(src) - 1] != '#')
                {
                    goto SETING_FAIL;
                }

                char *token;
                char comma[] = ",";
                int index = 0;
                token = strtok(src, comma);
                while ( token != NULL )
                {
                    index += 1;
                    if (index == 2)
                    {
                        if (strlen(token) > 40)
                        {
                            goto SETING_FAIL;
                        }
                        memcpy(apn_name, token, strlen(token));
                    }
                    if (index == 3)
                    {
                        if (strlen(token) > 30)
                        {
                            goto SETING_FAIL;
                        }
                        memcpy(apn_user, token, strlen(token));
                    }
                    if (index == 4)
                    {
                        if (strlen(token) > 20)
                        {
                            goto SETING_FAIL;
                        }
                        memcpy(apn_passwd, token, strlen(token) - 1);
                    }
                    token = strtok(NULL, comma);
                }

                LUAT_DEBUG_PRINT("apn_name : [%s]", apn_name);
                LUAT_DEBUG_PRINT("apn_user : [%s], apn_passwd: [%s]", apn_user, apn_passwd);

                config_service_set(CFG_APN_USER, TYPE_STRING, apn_user, sizeof(apn_user));
                config_service_set(CFG_APN_PWD, TYPE_STRING, apn_passwd, sizeof(apn_passwd));
                config_service_set(CFG_APN_NAME, TYPE_STRING, apn_name, sizeof(apn_name));

                if(flag == ENGLISH)
                {
                    uint8_t data[] = "Exec Success";
                    memcpy(dest,data,strlen(data));
                }
                else
                {
                    uint8_t data[] = "Setup successful";
                    memcpy(dest,data,strlen(data));
                }
            }
            break;
        case TIMER:
            if(0 == strcmp(src,"TIMER#"))
            {
                uint16_t upload_time = 0;
                config_service_get(CFG_UPLOADTIME, TYPE_SHORT, &upload_time, sizeof(upload_time));
                LUAT_DEBUG_PRINT("dio TIMER %d", upload_time);

                if(flag == ENGLISH)
                {
                    uint8_t data[30] = {0};
                    sprintf_(data, "Upload Interval:%d Seconds", upload_time);
                    memcpy(dest,data,strlen(data));
                }
                else
                {
                    uint8_t data[30] = {0};
                    sprintf_(data, "Upload interval: %d seconds", upload_time);
                    memcpy(dest,data,strlen(data));
                }
            }
            else if (strstr(src, "TIMER,"))
            {
                //upload interval
                int upload_tiem_t = 0;
                uint16_t upload_time = 0;

                if (src[strlen(src) - 1] != '#')
                {
                    goto SETING_FAIL;
                }

                char *token;
                char comma[] = ",";
                int index = 0;
                token = strtok(src, comma);
                while ( token != NULL )
                {
                    LUAT_DEBUG_PRINT("TIMER token [%s]", token);
                    index += 1;
                    if (index == 2)
                    {
                        int token_len = strlen(token) - 1;
                        int ct_index = 0;
                        while (token[ct_index] != '#')
                        {
                            int sum = 1;
                            for (int i = 0; (ct_index + i + 1) < token_len; i++)
                            {
                                sum *= 10;
                            }
                            LUAT_DEBUG_PRINT("TIMER token sum [%d]", sum);
                            upload_tiem_t += ((token[ct_index++] - '0') * sum);
                        }
                    }
                    token = strtok(NULL, comma);
                }
                LUAT_DEBUG_PRINT("TIMER %d", upload_tiem_t);

                upload_time = upload_tiem_t;
                LUAT_DEBUG_PRINT("TIMER %d", upload_time);

                config_service_set(CFG_UPLOADTIME, TYPE_SHORT, &upload_time, sizeof(upload_time));

                if(flag == ENGLISH)
                {
                    uint8_t data[] = "Exec Success";
                    memcpy(dest,data,strlen(data));
                }
                else
                {
                    uint8_t data[] = "Setup successful";
                    memcpy(dest,data,strlen(data));
                }
            }
            break;
        case PARAM:
            if(0 == strcmp(src,"PARAM#"))
            {
                uint8_t protocol_num = 0;
                uint16_t upload_time = 0;
                uint32_t server_port = 80;
                uint16_t heart_interval = 0;
                uint8_t apn_name[20] = {0};
                uint8_t server_addr[40] = {0};

                config_service_get(CFG_APN_NAME, TYPE_BYTE, apn_name, sizeof(apn_name));
                config_service_get(CFG_UPLOADTIME, TYPE_SHORT, &upload_time, sizeof(upload_time));
                config_service_get(CFG_SERVERADDR, TYPE_STRING, server_addr, sizeof(server_addr));
                config_service_get(CFG_SERVERADDR_PORT, TYPE_BYTE, (uint8_t *)&server_port, sizeof(server_port));
                config_service_get(CFG_PROTOCOL_NUM, TYPE_BYTE, &protocol_num, sizeof(protocol_num));
                config_service_get(CFG_HEART_INTERVAL, TYPE_SHORT, &heart_interval, sizeof(heart_interval));

                LUAT_DEBUG_PRINT("protocol_num: [%d] upload_time: [%d] server_port[%d]", 
                protocol_num, upload_time, server_port);
                LUAT_DEBUG_PRINT("apn_name: [%s]", apn_name);
                LUAT_DEBUG_PRINT("server_addr: [%s]", server_addr);

                uint8_t *data = (uint8_t *)LUAT_MEM_MALLOC(200);
                memset(data, 0, 200);

                if(flag == ENGLISH)
                {
                    sprintf_(data, "Server:%s", server_addr);
                    sprintf_(data + strlen(data), ":%d,", server_port);
                    if (protocol_num == 2)
                    {
                        sprintf_(data + strlen(data), "Protocol:CONCOX,version:%d,", protocol_num);
                    }
                    else
                    {
                        sprintf_(data + strlen(data), "Protocol:Ministry of Communications 808-2013 Agreement,version:%d,", protocol_num);
                    }
                    sprintf_(data + strlen(data), "Upload interval:%d Seconds,", upload_time);
                    sprintf_(data + strlen(data), "Heartbeat interval:%d Seconds,", heart_interval);
                    sprintf_(data + strlen(data), "APN:%s,", apn_name);
                    memcpy(dest,data,strlen(data));
                }
                else
                {
                    sprintf_(data, "Server: %s", server_addr);
                    sprintf_(data + strlen(data), ":%d,", server_port);
                    if (protocol_num == 2)
                    {
                        sprintf_(data + strlen(data), "Protocol: Conkeys protocol, protocol version: %d,", protocol_num);
                    }
                    else
                    {
                        sprintf_(data + strlen(data), "Agreement: Ministry of Communications 808-2013 version agreement, agreement version: %d,", protocol_num);
                    }
                    sprintf_(data + strlen(data), "Upload interval: %d seconds,", upload_time);
                    sprintf_(data + strlen(data), "Heartbeat interval: %d seconds,", heart_interval);
                    sprintf_(data + strlen(data), "APN:%s,", apn_name);
                    memcpy(dest,data,strlen(data));
                }
                LUAT_MEM_FREE(data);
            }
            break;
        case STATUS:
            if(0 == strcmp(src,"STATUS#"))
            {
                uint8_t acc = 0;
                uint8_t relay = 0;
                uint8_t car_move_state = 0;
                uint8_t fortificat_mode = 0;
                uint8_t gps_state = gpsx.gpssta;
                uint8_t bat_range[] = "9-36";

                int gprs = 0;
                float vbat = 4.2;
                uint8_t csq = 0;
                float input_volt = 17.7;
                uint8_t g_sersor[10] = {0};

                #if LUAT_HARD_SUPPORT
                gprs = config_gps_get();
                vbat = config_vbat_get();
                csq = soc_mobile_get_csq();
                input_volt = (config_input_volt_get() / 1000);
                #endif
                memcpy(g_sersor, "static", 7);

                config_service_get(CFG_RELAY, TYPE_BYTE, &relay, sizeof(relay));
                config_service_get(CFG_FORTIFICAT_MODE, TYPE_BYTE, &fortificat_mode, sizeof(fortificat_mode));


                uint8_t *data = (uint8_t *)LUAT_MEM_MALLOC(256);
                memset(data, 0, 256);

                if(flag == ENGLISH)
                {
                    sprintf_(data, "GSM:%d,", csq);
                    sprintf_(data + strlen(data), "GPRS:%d,", gprs);
                    sprintf_(data + strlen(data), "GPS:%s,", gps_state == 1 ? "fixed" : "unfixed");
                    sprintf_(data + strlen(data), "Voltage:%.2fV,", input_volt);
                    sprintf_(data + strlen(data), "Battery:%.2fV,", vbat);
                    sprintf_(data + strlen(data), "ACC:%d,", acc);
                    sprintf_(data + strlen(data), "G_sensor:%s,", g_sersor);
                    sprintf_(data + strlen(data), "Relay:%d,", relay);
                    sprintf_(data + strlen(data), "Defence mode:%d,", fortificat_mode);
                    sprintf_(data + strlen(data), "Defence state:%d,", car_move_state);
                    sprintf_(data + strlen(data), "Voltage range:%sV", bat_range);
                    memcpy(dest,data,strlen(data));
                }
                else
                {
                    sprintf_(data, "GSM signal strength:%d,", csq);
                    sprintf_(data + strlen(data), "GPRS status:%d,", gprs);
                    sprintf_(data + strlen(data), "GPS:%s,", gps_state == 1 ? "fixed" : "unfixed");
                    sprintf_(data + strlen(data), "Power supply voltage:%.2fV,", input_volt);
                    sprintf_(data + strlen(data), "Battery voltage:%.2fV,", vbat);
                    sprintf_(data + strlen(data), "ACC:%d,", acc);
                    sprintf_(data + strlen(data), "Motion sensor: %s,", g_sersor);
                    sprintf_(data + strlen(data), "Oil and electricity status: %d,", relay);
                    sprintf_(data + strlen(data), "Defense mode:%d,", fortificat_mode);
                    sprintf_(data + strlen(data), "Defense status: %d,", car_move_state);
                    sprintf_(data + strlen(data), "Voltage range:%sV", bat_range);
                    memcpy(dest,data,strlen(data));
                }
                LUAT_DEBUG_PRINT("data: [%s]", data);
                LUAT_MEM_FREE(data);
            }
            break;
        case LBSON:
            if (0 == strcmp(src,"LBSON#"))
            {
                uint8_t cfg_base_wait_time = 0;
                uint8_t cfg_base_update_time = 0;
                uint8_t cfg_base_update = 0;

                config_service_get(CFG_BASE_UPLOAD_ENABLE, TYPE_BYTE, &cfg_base_update, sizeof(cfg_base_update));
                config_service_get(CFG_BASE_WAIT_GPS_TIME, TYPE_BYTE, &cfg_base_wait_time, sizeof(cfg_base_wait_time));
                config_service_get(CFG_BASE_UPLOAD_INTERVAL, TYPE_BYTE, &cfg_base_update_time, sizeof(cfg_base_update_time));

                uint8_t *data = (uint8_t *)LUAT_MEM_MALLOC(256);
                memset(data, 0, 256);

                if(flag == ENGLISH)
                {
                    sprintf_(data, "LBS:%s,", (cfg_base_update == 0 ? "disable" : "enable"));
                    sprintf_(data + strlen(data), "Upload interval:%d seconds,", cfg_base_update_time);
                    sprintf_(data + strlen(data), "Wait for GPS %d seconds,", cfg_base_wait_time);
                    memcpy(dest,data,strlen(data));
                }
                else
                {
                    sprintf_(data, "Upload to base station: %s,", (cfg_base_update == 0 ? "closure" : "Open"));
                    sprintf_(data + strlen(data), "Base station upload interval: %d seconds,", cfg_base_update_time);
                    sprintf_(data + strlen(data), "Waiting for GPS positioning for %d seconds,", cfg_base_wait_time);
                    memcpy(dest,data,strlen(data));
                }

                LUAT_DEBUG_PRINT("data: [%s]", data);
                LUAT_MEM_FREE(data);
            }
            else if (strstr(src, "LBSON,"))
            {
                bool base_update = false;
                //Base station upload time
                int base_update_time = 0;
                //Wait for GPS positioning time
                int base_wait_time = 0;

                if (src[strlen(src) - 1] != '#')
                {
                    goto SETING_FAIL;
                }

                if (src[6] == '1')
                {
                    base_update = true;
                }
                else if (src[6] == '0')
                {
                    base_update = false;
                }

                char *token;
                char comma[] = ",";
                int index = 0;
                token = strtok(src, comma);
                while ( token != NULL )
                {
                    index += 1;
                    if (index == 3)
                    {
                        int token_len = strlen(token);
                        int ct_index = 0;
                        while (token[ct_index] != '\0')
                        {
                            int sum = 1;
                            for (int i = 0; (ct_index + i + 1) < token_len; i++)
                            {
                                sum *= 10;
                            }
                            base_update_time += (token[ct_index++] - '0') * sum;
                        }
                    }
                    if (index == 4)
                    {
                        int token_len = strlen(token) - 1;
                        int ct_index = 0;
                        while ((token[ct_index] != '\0') && (token[ct_index] != '#'))
                        {
                            int sum = 1;
                            for (int i = 0; (ct_index + i + 1) < token_len; i++)
                            {
                                sum *= 10;
                            }
                            base_wait_time += (token[ct_index++] - '0') * sum;
                        }
                    }
                    token = strtok(NULL, comma);
                }

                uint8_t cfg_base_wait_time = base_wait_time;
                uint8_t cfg_base_update_time = base_update_time;
                uint8_t cfg_base_update = (base_update ? 1 : 0);

                config_service_set(CFG_BASE_UPLOAD_ENABLE, TYPE_BYTE, &cfg_base_update, sizeof(cfg_base_update));
                config_service_set(CFG_BASE_WAIT_GPS_TIME, TYPE_BYTE, &cfg_base_wait_time, sizeof(cfg_base_wait_time));
                config_service_set(CFG_BASE_UPLOAD_INTERVAL, TYPE_BYTE, &cfg_base_update_time, sizeof(cfg_base_update_time));

                LUAT_DEBUG_PRINT("[%d][%d][%d]", cfg_base_update, cfg_base_update_time, cfg_base_wait_time);
                
                if(flag == ENGLISH)
                {
                    uint8_t data[] = "Exec Success";
                    memcpy(dest,data,strlen(data));
                }
                else
                {
                    uint8_t data[] = "Setup successful";
                    memcpy(dest,data,strlen(data));
                }
            }
            break;
        case SENALM:
            if (0 == strcmp(src,"SENALM#"))
            {
                uint8_t fortificat_mode = 0;
                
                config_service_get(CFG_FORTIFICAT_MODE, TYPE_BYTE, &fortificat_mode, sizeof(fortificat_mode));

                if(flag == ENGLISH)
                {
                    uint8_t data[30] = {0};
                    sprintf_(data, "%s Vibration Alarm", (fortificat_mode == 0 ? "Close" : "Open"));
                    memcpy(dest,data,strlen(data));
                }
                else
                {
                    uint8_t data[30] = {0};
                    sprintf_(data, "%sVibration alarm", (fortificat_mode == 0 ? "closure" : "Open"));
                    memcpy(dest,data,strlen(data));
                }
            }
            else if (strstr(src, "SENALM,"))
            {
                uint8_t fortificat_mode = 0;

                if (src[strlen(src) - 1] != '#')
                {
                    goto SETING_FAIL;
                }

                if (strcmp(src,"SENALM,1#") == 0)
                {
                    fortificat_mode = 1;
                    #if LUAT_HARD_SUPPORT
                        //Turn on vibration alarm
                    #endif
                }
                else if (strcmp(src,"SENALM,0#") == 0)
                {
                    fortificat_mode = 0;
                    #if LUAT_HARD_SUPPORT
                        //Turn off vibration alarm
                    #endif
                }

                config_service_set(CFG_FORTIFICAT_MODE, TYPE_BYTE, &fortificat_mode, sizeof(fortificat_mode));
                
                if(flag == ENGLISH)
                {
                    uint8_t data[] = "Exec Success";
                    memcpy(dest,data,strlen(data));
                }
                else
                {
                    uint8_t data[] = "Setup successful";
                    memcpy(dest,data,strlen(data));
                }
            }
            break;
        case HBT:
            if (0 == strcmp(src,"HBT#"))
            {
                uint16_t heart_time = 180;
                config_service_get(CFG_HEART_INTERVAL, TYPE_SHORT, &heart_time, sizeof(heart_time));
                
                LUAT_DEBUG_PRINT("HBT# heart_time : %d", heart_time);

                int heart_time_tem = heart_time;
                LUAT_DEBUG_PRINT("HBT# heart_time : %d", heart_time_tem);

                float heart_dis;
                heart_dis = (float)heart_time_tem / 60;
                LUAT_DEBUG_PRINT("HBT# heart_time : %.2f", heart_dis);

                if(flag == ENGLISH)
                {
                    uint8_t data[50] = {0};
                    sprintf_(data, "Heartbeat Interval: %.2f min", heart_dis);
                    memcpy(dest,data,strlen(data));
                }
                else
                {
                    uint8_t data[50] = {0};
                    sprintf_(data, "Heartbeat interval: %.2f minutes", heart_dis);
                    memcpy(dest,data,strlen(data));
                }
            }
            else if (strstr(src,"HBT,"))
            {
                size_t len = strlen(src);
                if ((src[len - 1] != '#') || (len != 6) || (src[4] < '1') || (src[4] > '6'))
                {
                    goto SETING_FAIL;
                }

                int heart_time_tem = (src[4] - '0') * 60;

                uint16_t heart_time = heart_time_tem;
                config_service_set(CFG_HEART_INTERVAL, TYPE_SHORT, &heart_time, sizeof(heart_time));

                if(flag == ENGLISH)
                {
                    uint8_t data[] = "Exec Success";
                    memcpy(dest,data,strlen(data));
                }
                else
                {
                    uint8_t data[] = "Setup successful";
                    memcpy(dest,data,strlen(data));
                }
            }
            break;
        case HBTS:
            if (0 == strcmp(src,"HBTS#"))
            {
                uint16_t heart_time = 180;
                config_service_get(CFG_HEART_INTERVAL, TYPE_SHORT, &heart_time, sizeof(heart_time));

                if(flag == ENGLISH)
                {
                    uint8_t data[50] = {0};
                    sprintf_(data, "Heartbeat Interval: %d seconds", heart_time);
                    memcpy(dest,data,strlen(data));
                }
                else
                {
                    uint8_t data[50] = {0};
                    sprintf_(data, "Heartbeat interval: %d seconds", heart_time);
                    memcpy(dest,data,strlen(data));
                }
            }
            else if (strstr(src,"HBTS,"))
            {
                size_t len = strlen(src);
                if ((src[len - 1] != '#') || (len > 9) || (len < 7))
                {
                    goto SETING_FAIL;
                }

                int heart_time = 0;

                char *token;
                char comma[] = ",";
                int index = 0;
                token = strtok(src, comma);
                while ( token != NULL )
                {
                    index += 1;
                    if (index == 2)
                    {
                        int token_len = strlen(token) - 1;
                        int ct_index = 0;
                        while ((token[ct_index] != '\0') && (token[ct_index] != '#'))
                        {
                            int sum = 1;
                            for (int i = 0; (ct_index + i + 1) < token_len; i++)
                            {
                                sum *= 10;
                            }
                            heart_time += (token[ct_index++] - '0') * sum;
                        }
                    }
                    token = strtok(NULL, comma);
                }

                if ((heart_time < 3) || (heart_time > 180))
                {
                    goto SETING_FAIL;
                }

                uint16_t heart_time_set = heart_time;
                config_service_set(CFG_HEART_INTERVAL, TYPE_SHORT, &heart_time_set, sizeof(heart_time_set));

                if(flag == ENGLISH)
                {
                    uint8_t data[] = "Exec Success";
                    memcpy(dest,data,strlen(data));
                }
                else
                {
                    uint8_t data[] = "Setup successful";
                    memcpy(dest,data,strlen(data));
                }
            }
            break;
        case LBS:
            if (0 == strcmp(src,"LBS#"))
            {
                lbs_info_t info = {0};
                luat_lbs_get_info(&info);
                
                uint8_t *data = (uint8_t *)LUAT_MEM_MALLOC(256);
                memset(data, 0, 256);

                int hour = (int)(info.hour);
                int minute = (int)(info.minute);
                int second = (int)(info.second);

                if(flag == ENGLISH)
                {
                    sprintf_(data, "Longitude:%s, Latitude:%s", info.latitude, info.longitude);
                    sprintf_(data + strlen(data), "Time: %d-%.2d-%.2d %.2d:%.2d:%.2d", 
                            info.year, info.month, info.day, hour, minute, second);
                    memcpy(dest,data,strlen(data));
                }
                else
                {
                    sprintf_(data, "Latitude:%s, longitude:%s", info.latitude, info.longitude);
                    sprintf_(data + strlen(data), "time: %d-%.2d-%.2d %.2d:%.2d:%.2d", 
                            info.year, info.month, info.day, hour, minute, second);
                    memcpy(dest,data,strlen(data));
                }

                LUAT_MEM_FREE(data);
            }
            break;
        default:
            break;
    }

    if (run_fail)
    {
        SETING_FAIL:
        //Failed to send SMS settings
        if (flag == ENGLISH)
        {
            uint8_t data[] = "Exec Failed";
            memcpy(dest,data,strlen(data));
        }
        else
        {
            uint8_t data[] = "Setup failed";
            memcpy(dest,data,strlen(data));
        }
    }
    return cmd;
}
