#ifndef __MQTT_TASK__
#define __MQTT_TASK__


typedef enum LUAT_MQTT_PUBLISH_TOPIC_ID
{
    TOPIC_1 = 1,
    TOPIC_2,
    TOPIC_3
}LUAT_MQTT_PUBLISH_TOPIC_ID_E;



typedef enum LUAT_MQTT_PUBLISH_ERROR_CODE
{
    /* data */
    MQTT_PUBLISH_SUCCESS = 0,
    MQTT_PUBLISH_FAIL,
    NETWORK_NOT_READY,
    MQTT_NOT_READY
}LUAT_MQTT_PUBLISH_ERROR_CODE_E;

typedef struct mqtt_publish_para
{
    uint8_t qos;
    uint8_t retain;
    char *msg;
    uint32_t msg_len;
    void (*callback)(LUAT_MQTT_PUBLISH_ERROR_CODE_E code);
} mqtt_publish_para_t;

#endif
