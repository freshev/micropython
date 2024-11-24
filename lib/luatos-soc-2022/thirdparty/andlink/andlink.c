#include "luat_base.h"
#include "luat_network_adapter.h"
#include "luat_rtos.h"
#include "common_api.h"
#include "luat_debug.h"
#include "luat_mobile.h"
#include "luat_http.h"
#include "libemqtt.h"
#include "luat_mqtt.h"

#include "cJSON.h"
#include "andlink.h"

#define ANDLINK_GET_URL     "https://andlink.komect.com/espapi/m2m/rest/devices/cgw"
#define ANDLINK_REG_URL     "%s/device/inform/bootstrap"
#define ANDLINK_ONLINE_URL  "%s/device/inform/boot"
#define ANDLINK_MQTT_DOWM   "/device/%s/downward"
#define ANDLINK_MQTT_UP     "/device/%s/upward"

#define MQTT_HOST_LEN       256
#define MQTT_TOPIC_LEN      256

#define ANDLINK_QUEUE_SIZE 	 64

static luat_rtos_queue_t andlink_queue_handle;
typedef enum{
    ADL_MQTT_MSG_CONNACK = 1,
    ADL_MQTT_MSG_PUBLISH,
    ADL_MQTT_MSG_CLOSE,
    ADL_MQTT_MSG_RELEASE,
    ADL_TIMER_REPORT,
    ADL_ERROR,
} andlink_event_t;
typedef struct{
	luat_mqtt_ctrl_t *luat_mqtt_ctrl;
	andlink_event_t event;
} andlink_queue_t;

typedef struct{
    adl_dev_attr_t devAttr;
    uint8_t *gwAddress2;
    uint8_t User_Key[64];
    uint8_t *deviceId;                      // device id
    uint8_t *deviceToken;                   //Device key, currently undefined purpose
    uint8_t *dmToken;                       // Terminal management data reporting interface authentication token
    uint8_t *gwToken;                       //Gateway Token
    uint8_t *deviceManageUrl;               //Gateway Token
    uint8_t *mqttClientId;                  // mqttClientId
    uint8_t *mqttUser;                      //mqtt connection username
    uint8_t *mqttPassword;                  // mqtt connection password
    uint8_t mqttHost[MQTT_HOST_LEN];        // Mqtt Host
    uint16_t mqttPort;                      // Mqtt Port
    uint32_t mqttKeepalive;                 //Heartbeat period (unit seconds)
    uint8_t mqtt_pub_topic[MQTT_TOPIC_LEN]; // Mqtt pub_topic
    uint8_t device_ipaddr[64];              // device_ipaddr
    uint8_t device_sn[32];                  // device_sn
    uint8_t device_os[16];                  // device_os
    uint8_t *http_post_data;
    luat_http_ctrl_t *andlink_http_client;
    luat_mqtt_ctrl_t *andlink_mqtt_client;
    adl_dev_callback_t* devCbs;
    void* report_timer;
} andlink_client_t;

static andlink_client_t* andlink_client = NULL;

static void andlink_http_cb(int status, void *data, uint32_t len, void *param){
	if (status < 0){
		LUAT_DEBUG_PRINT("http failed! %d", status);
		luat_rtos_event_send(param, -1, 0, 0, 0, 0);
		return;
	}
	switch(status)
    {
	case HTTP_STATE_GET_BODY:
		if (data){
            // LUAT_DEBUG_PRINT("HTTP_STATE_GET_BODY %s", data);
            andlink_client->http_post_data = calloc(1, len+1);
            memcpy(andlink_client->http_post_data, data, len);
		} else {
            luat_rtos_event_send(param, 0, 0, 0, 0, 0);
		}
		break;
	case HTTP_STATE_GET_HEAD:
		break;
	case HTTP_STATE_IDLE:
		break;
	case HTTP_STATE_SEND_BODY_START:
        luat_http_client_post_body(andlink_client->andlink_http_client, andlink_client->http_post_data, strlen(andlink_client->http_post_data));
        free(andlink_client->http_post_data);
        andlink_client->http_post_data = NULL;
		break;
	case HTTP_STATE_SEND_BODY:
		break;
	default:
		break;
	}
}

static void andlink_mqtt_cb(luat_mqtt_ctrl_t *luat_mqtt_ctrl, uint16_t event){
    andlink_queue_t andlink_queue = {.luat_mqtt_ctrl = luat_mqtt_ctrl};
	switch (event){
	case MQTT_MSG_CONNACK:{
        andlink_queue.event = ADL_MQTT_MSG_CONNACK;
        luat_rtos_queue_send(andlink_queue_handle, &andlink_queue, NULL, 0);
		break;
	}
	case MQTT_MSG_PUBLISH : {
        andlink_queue.event = ADL_MQTT_MSG_PUBLISH;
        luat_rtos_queue_send(andlink_queue_handle, &andlink_queue, NULL, 0);
		break;
	}
	case MQTT_MSG_PUBACK : 
	case MQTT_MSG_PUBCOMP : {
		// LUAT_DEBUG_PRINT("msg_id: %d",mqtt_parse_msg_id(luat_mqtt_ctrl->mqtt_packet_buffer));
		break;
	}
	case MQTT_MSG_CLOSE : {
        andlink_queue.event = ADL_MQTT_MSG_CLOSE;
        luat_rtos_queue_send(andlink_queue_handle, &andlink_queue, NULL, 0);
		break;
	}
	case MQTT_MSG_RELEASE : {
        andlink_queue.event = ADL_MQTT_MSG_RELEASE;
        luat_rtos_queue_send(andlink_queue_handle, &andlink_queue, NULL, 0);
		break;
	}
	default:
		break;
	}
	return;
}

static void andlink_manage_report(void){
    luat_event_t event;
    cJSON* device_manage_json = cJSON_CreateObject();
	cJSON_AddStringToObject(device_manage_json, "deviceId", andlink_client->deviceId);
	cJSON_AddStringToObject(device_manage_json, "deviceType", andlink_client->devAttr.deviceType);
    cJSON_AddNumberToObject(device_manage_json, "timestamp", (double)time(NULL));
    cJSON_AddStringToObject(device_manage_json, "firmwareVersion", andlink_client->devAttr.firmWareVersion);
    cJSON_AddStringToObject(device_manage_json, "softwareVersion", andlink_client->devAttr.softWareVersion);
    cJSON_AddStringToObject(device_manage_json, "mac", andlink_client->devAttr.deviceMac);
    cJSON_AddStringToObject(device_manage_json, "sn", andlink_client->device_sn);
    cJSON_AddStringToObject(device_manage_json, "OS", andlink_client->device_os);
    cJSON* cmccVersion_json = cJSON_CreateObject();
    cJSON_AddStringToObject(cmccVersion_json, "andlinkVersion", "protocol");
    cJSON_AddItemToObject(device_manage_json, "cmccVersion", cmccVersion_json);
	cJSON_AddStringToObject(device_manage_json, "productToken", andlink_client->devAttr.productToken);
    cJSON_AddStringToObject(device_manage_json, "cpuModel", "Cortex-M3");
    cJSON_AddStringToObject(device_manage_json, "romStorageSize", "4MB");
    cJSON_AddStringToObject(device_manage_json, "ramStorageSize", "1MB");
    cJSON_AddStringToObject(device_manage_json, "networkType", "4G");
    cJSON_AddStringToObject(device_manage_json, "locationInfo", "NONE");
    cJSON_AddStringToObject(device_manage_json, "deviceVendor", "airm2m");
    cJSON_AddStringToObject(device_manage_json, "deviceBrand", "airm2m");
    cJSON_AddStringToObject(device_manage_json, "deviceModel", "air780");
    cJSON_AddStringToObject(device_manage_json, "powerSupplyMode", "battery");
    cJSON_AddStringToObject(device_manage_json, "wifiRssi", "-50");
    cJSON_AddStringToObject(device_manage_json, "deviceIP", andlink_client->device_ipaddr);
    cJSON* deviceManageExtInfo = cJSON_CreateObject();
    cJSON_AddItemToObject(device_manage_json, "deviceManageExtInfo", deviceManageExtInfo);
	char* device_manage_data = cJSON_Print(device_manage_json);
    cJSON_Delete(device_manage_json);
	// LUAT_DEBUG_PRINT("device_manage_data:%s",device_manage_data);

    andlink_client->http_post_data = calloc(1, strlen(device_manage_data)+1);
    memcpy(andlink_client->http_post_data, device_manage_data, strlen(device_manage_data));
    cJSON_free(device_manage_data);
    char body_len[6] = {0};
    memset(body_len,0,6);
    snprintf_(body_len, 6, "%d", strlen(andlink_client->http_post_data));
    luat_http_client_clear(andlink_client->andlink_http_client);
    luat_http_client_set_user_head(andlink_client->andlink_http_client, "Content-Type", "application/json");
    luat_http_client_set_user_head(andlink_client->andlink_http_client, "Accept-Charset", "utf-8");
    luat_http_client_set_user_head(andlink_client->andlink_http_client, "User-Key", andlink_client->User_Key);
    luat_http_client_set_user_head(andlink_client->andlink_http_client, "dmToken", andlink_client->dmToken);
    luat_http_client_set_user_head(andlink_client->andlink_http_client, "Content-Length", body_len);
    luat_http_client_start(andlink_client->andlink_http_client, andlink_client->deviceManageUrl, 1, 0, 0);
    luat_rtos_event_recv(luat_rtos_get_current_handle(), 0, &event, NULL, LUAT_WAIT_FOREVER);
    luat_http_client_close(andlink_client->andlink_http_client);

    if (andlink_client->http_post_data){
        free(andlink_client->http_post_data);
        andlink_client->http_post_data = NULL;
    }
}

static LUAT_RT_RET_TYPE andlink_timer_report(LUAT_RT_CB_PARAM){
    andlink_queue_t andlink_queue = {.event = ADL_TIMER_REPORT};
    luat_rtos_queue_send(andlink_queue_handle, &andlink_queue, NULL, 0);
}

int andlink_init(adl_dev_attr_t *devAttr, adl_dev_callback_t *devCbs){
    int ret = -1;
    uint32_t all,now_free_block,min_free_block;
    cJSON * http_post_data_json = NULL;
    cJSON* resultCode_json = NULL;
    char body_len[6] = {0};
    luat_event_t event;
    if (devAttr->cfgNetMode != NETWOKR_MODE_4G){
        LUAT_DEBUG_PRINT("only support NETWOKR_MODE_4G");
        goto error;
    }
    if (andlink_client){
        LUAT_DEBUG_PRINT("andlink_client already initialized");
        goto error;
    }
    andlink_client = malloc(sizeof(andlink_client_t));
    memset(andlink_client, 0, sizeof(andlink_client_t));
    memcpy(&andlink_client->devAttr, devAttr, sizeof(adl_dev_attr_t));
    andlink_client->devCbs = devCbs;

    if (andlink_client->devCbs->get_device_ipaddr){
        andlink_client->devCbs->get_device_ipaddr(andlink_client->device_ipaddr,NULL);
        // LUAT_DEBUG_PRINT("andlink_client->device_ipaddr: %s", andlink_client->device_ipaddr);
    }else{
        LUAT_DEBUG_PRINT("no get_device_ipaddr");
        goto error;
    }
    
    andlink_client->andlink_http_client = luat_http_client_create(andlink_http_cb, luat_rtos_get_current_handle(), -1);
    luat_http_client_base_config(andlink_client->andlink_http_client, 30*1000, 0, 0);
    luat_http_client_ssl_config(andlink_client->andlink_http_client, 0, NULL, 0,NULL, 0,NULL, 0,NULL, 0);
    luat_http_client_clear(andlink_client->andlink_http_client);
    luat_http_client_set_user_head(andlink_client->andlink_http_client, "Content-Type", "application/json");
    luat_http_client_set_user_head(andlink_client->andlink_http_client, "Accept-Charset", "utf-8");

    andlink_client->http_post_data = calloc(1, 96);
    snprintf_((char*)andlink_client->http_post_data, 96,  "{\"deviceType\":\"%s\",\"productToken\":\"%s\"}", andlink_client->devAttr.deviceType,andlink_client->devAttr.productToken);
    snprintf_(body_len, 6, "%d", strlen(andlink_client->http_post_data));
    luat_http_client_set_user_head(andlink_client->andlink_http_client, "Content-Length", body_len);
    luat_http_client_start(andlink_client->andlink_http_client, ANDLINK_GET_URL, 1, 0, 0);

    luat_rtos_event_recv(luat_rtos_get_current_handle(), 0, &event, NULL, LUAT_WAIT_FOREVER);
    luat_http_client_close(andlink_client->andlink_http_client);
    if (event.id){ goto error; }
    cJSON_Minify(andlink_client->http_post_data);
    http_post_data_json = cJSON_Parse(andlink_client->http_post_data);
    resultCode_json = cJSON_GetObjectItemCaseSensitive(http_post_data_json, "resultCode");
    if(cJSON_IsNumber(resultCode_json)) {
        int resultCode = (int)resultCode_json->valuedouble;
        if (resultCode == 0){
            cJSON* gwAddress2_json = cJSON_GetObjectItemCaseSensitive(http_post_data_json, "gwAddress2");
            andlink_client->gwAddress2 = calloc(1, strlen(gwAddress2_json->valuestring)+1);
            memcpy(andlink_client->gwAddress2, gwAddress2_json->valuestring, strlen(gwAddress2_json->valuestring));
        } else {
            LUAT_DEBUG_PRINT("andlink error :%s",andlink_client->http_post_data);
            goto error;
        }
    }else{
        LUAT_DEBUG_PRINT("andlink error :%s",andlink_client->http_post_data);
        goto error;
    }

    if (http_post_data_json){
        cJSON_Delete(http_post_data_json);
        http_post_data_json = NULL;
    }

    if (andlink_client->http_post_data){
        free(andlink_client->http_post_data);
        andlink_client->http_post_data = NULL;
    }
    
    cJSON * extInfo_json = cJSON_Parse(andlink_client->devAttr.extInfo);
    cJSON* device_sn = cJSON_GetObjectItemCaseSensitive(extInfo_json, "sn");
    cJSON* device_os = cJSON_GetObjectItemCaseSensitive(extInfo_json, "OS");
    memcpy(andlink_client->device_sn, device_sn->valuestring, strlen(device_sn->valuestring));
    memcpy(andlink_client->device_os, device_os->valuestring, strlen(device_os->valuestring));
    cJSON* chips = cJSON_GetObjectItemCaseSensitive(extInfo_json, "chips");
    cJSON *chips_json_item1 = cJSON_GetArrayItem(chips, 0);
    cJSON* factory = cJSON_GetObjectItemCaseSensitive(chips_json_item1, "factory");
    sprintf_(andlink_client->User_Key, "CMCC-%s-%s", factory->valuestring,andlink_client->devAttr.deviceType);
    cJSON_AddStringToObject(extInfo_json, "mac", andlink_client->devAttr.deviceMac);
	cJSON* register_json = cJSON_CreateObject();
    cJSON_AddStringToObject(register_json, "deviceMac", andlink_client->devAttr.deviceMac);
    cJSON_AddStringToObject(register_json, "productToken", andlink_client->devAttr.productToken);
    cJSON_AddStringToObject(register_json, "deviceType", andlink_client->devAttr.deviceType);
    cJSON_AddNumberToObject(register_json, "timestamp", (double)time(NULL));
    cJSON_AddStringToObject(register_json, "protocolVersion", PROTOCOL_VERSION);
    cJSON_AddItemToObject(register_json, "deviceExtInfo", extInfo_json);

	char* register_data = cJSON_Print(register_json);
    cJSON_Delete(register_json);
	// LUAT_DEBUG_PRINT("register_data:%s",register_data);

    andlink_client->http_post_data = calloc(1, strlen(register_data)+1);
    memcpy(andlink_client->http_post_data, register_data, strlen(register_data));
    cJSON_free(register_data);
    // LUAT_DEBUG_PRINT("andlink_client->http_post_data:%s",andlink_client->http_post_data);

    memset(body_len,0,6);
    snprintf_(body_len, 6, "%d", strlen(andlink_client->http_post_data));
    luat_http_client_clear(andlink_client->andlink_http_client);
    luat_http_client_set_user_head(andlink_client->andlink_http_client, "Content-Type", "application/json");
    luat_http_client_set_user_head(andlink_client->andlink_http_client, "Accept-Charset", "utf-8");
    luat_http_client_set_user_head(andlink_client->andlink_http_client, "User-Key", andlink_client->User_Key);
    luat_http_client_set_user_head(andlink_client->andlink_http_client, "Content-Length", body_len);
    char andlink_reg_url[64] = {0};
    snprintf_(andlink_reg_url, 64, ANDLINK_REG_URL, andlink_client->gwAddress2);
    // LUAT_DEBUG_PRINT("andlink_reg_url:%s",andlink_reg_url);
    if (andlink_client->devCbs->set_led_callback){
        andlink_client->devCbs->set_led_callback(ADL_BOOTSTRAP);
    }
    luat_http_client_start(andlink_client->andlink_http_client, andlink_reg_url, 1, 0, 0);
    luat_rtos_event_recv(luat_rtos_get_current_handle(), 0, &event, NULL, LUAT_WAIT_FOREVER);
    luat_http_client_close(andlink_client->andlink_http_client);
    if (event.id){ 
        if (andlink_client->devCbs->set_led_callback){
            andlink_client->devCbs->set_led_callback(ADL_BOOTSTRAP_FAIL);
        }
        goto error; 
    }
    // LUAT_DEBUG_PRINT("andlink_client->http_post_data:%s",andlink_client->http_post_data);
    
    http_post_data_json = cJSON_Parse(andlink_client->http_post_data);
    resultCode_json = cJSON_GetObjectItemCaseSensitive(http_post_data_json, "resultCode");
    if(cJSON_IsNumber(resultCode_json)) {
        if (andlink_client->devCbs->set_led_callback){
            andlink_client->devCbs->set_led_callback(ADL_BOOTSTRAP_FAIL);
        }
        LUAT_DEBUG_PRINT("andlink error :%s",andlink_client->http_post_data);
        goto error;
    }
    if (andlink_client->devCbs->set_led_callback){
        andlink_client->devCbs->set_led_callback(ADL_BOOTSTRAP_SUC);
    }
    cJSON* deviceId_json = cJSON_GetObjectItemCaseSensitive(http_post_data_json, "deviceId");
    cJSON* deviceToken_json = cJSON_GetObjectItemCaseSensitive(http_post_data_json, "deviceToken");
    cJSON* dmToken_json = cJSON_GetObjectItemCaseSensitive(http_post_data_json, "dmToken");
    cJSON* gwToken_json = cJSON_GetObjectItemCaseSensitive(http_post_data_json, "gwToken");

    andlink_client->deviceId = calloc(1, strlen(deviceId_json->valuestring)+1);
    memcpy(andlink_client->deviceId, deviceId_json->valuestring, strlen(deviceId_json->valuestring));

    andlink_client->deviceToken = calloc(1, strlen(deviceToken_json->valuestring)+1);
    memcpy(andlink_client->deviceToken, deviceToken_json->valuestring, strlen(deviceToken_json->valuestring));

    andlink_client->dmToken = calloc(1, strlen(dmToken_json->valuestring)+1);
    memcpy(andlink_client->dmToken, dmToken_json->valuestring, strlen(dmToken_json->valuestring));

    andlink_client->gwToken = calloc(1, strlen(gwToken_json->valuestring)+1);
    memcpy(andlink_client->gwToken, gwToken_json->valuestring, strlen(gwToken_json->valuestring));
    // LUAT_DEBUG_PRINT("andlink_client->deviceId:%s",andlink_client->deviceId);
    // LUAT_DEBUG_PRINT("andlink_client->deviceToken:%s",andlink_client->deviceToken);
    // LUAT_DEBUG_PRINT("andlink_client->dmToken:%s",andlink_client->dmToken);
    // LUAT_DEBUG_PRINT("andlink_client->gwToken:%s",andlink_client->gwToken);

    if (http_post_data_json){
        cJSON_Delete(http_post_data_json);
        http_post_data_json = NULL;
    }

    if (andlink_client->http_post_data){
        free(andlink_client->http_post_data);
        andlink_client->http_post_data = NULL;
    }

    snprintf_(andlink_client->mqtt_pub_topic, MQTT_TOPIC_LEN, ANDLINK_MQTT_UP, andlink_client->deviceId);

    http_post_data_json = cJSON_CreateObject();
	cJSON_AddStringToObject(http_post_data_json, "deviceId", andlink_client->deviceId);
	cJSON_AddStringToObject(http_post_data_json, "deviceType", andlink_client->devAttr.deviceType);
	cJSON_AddStringToObject(http_post_data_json, "productToken", andlink_client->devAttr.productToken);
	cJSON_AddStringToObject(http_post_data_json, "firmwareVersion", andlink_client->devAttr.firmWareVersion);
    cJSON_AddStringToObject(http_post_data_json, "softwareVersion", andlink_client->devAttr.softWareVersion);
    cJSON_AddStringToObject(http_post_data_json, "protocolVersion", PROTOCOL_VERSION);
    cJSON_AddNumberToObject(http_post_data_json, "userBind", 2);
    cJSON_AddNumberToObject(http_post_data_json, "bootType", 0);
    cJSON_AddNumberToObject(http_post_data_json, "timestamp", (double)time(NULL));

	char* http_post_json_data = cJSON_Print(http_post_data_json);
    cJSON_Delete(http_post_data_json);
	// LUAT_DEBUG_PRINT("http_post_json_data:%s",http_post_json_data);

    andlink_client->http_post_data = calloc(1, strlen(http_post_json_data)+1);
    memcpy(andlink_client->http_post_data, http_post_json_data, strlen(http_post_json_data));
    cJSON_free(http_post_json_data);

    memset(body_len,0,6);
    snprintf_(body_len, 6, "%d", strlen(andlink_client->http_post_data));
    luat_http_client_clear(andlink_client->andlink_http_client);
    luat_http_client_set_user_head(andlink_client->andlink_http_client, "Content-Type", "application/json");
    luat_http_client_set_user_head(andlink_client->andlink_http_client, "Accept-Charset", "utf-8");
    luat_http_client_set_user_head(andlink_client->andlink_http_client, "User-Key", andlink_client->User_Key);
    luat_http_client_set_user_head(andlink_client->andlink_http_client, "Content-Length", body_len);

    char andlink_online_url[64] = {0};
    snprintf_(andlink_online_url, 64, ANDLINK_ONLINE_URL, andlink_client->gwAddress2);
    if (andlink_client->devCbs->set_led_callback){
        andlink_client->devCbs->set_led_callback(ADL_BOOT);
    }
    luat_http_client_start(andlink_client->andlink_http_client, andlink_online_url, 1, 0, 0);
    luat_rtos_event_recv(luat_rtos_get_current_handle(), 0, &event, NULL, LUAT_WAIT_FOREVER);
    luat_http_client_close(andlink_client->andlink_http_client);
    if (event.id){ 
        if (andlink_client->devCbs->set_led_callback){
            andlink_client->devCbs->set_led_callback(ADL_BOOT_FAIL);
        }
        goto error; 
    }
    // LUAT_DEBUG_PRINT("andlink_client->http_post_data:%s",andlink_client->http_post_data);

    http_post_data_json = cJSON_Parse(andlink_client->http_post_data);
    cJSON* respCode_json = cJSON_GetObjectItemCaseSensitive(http_post_data_json, "respCode");
    if(cJSON_IsInvalid(respCode_json) == cJSON_False) {
        LUAT_DEBUG_PRINT("andlink error :%s",andlink_client->http_post_data);
        if (andlink_client->devCbs->set_led_callback){
            andlink_client->devCbs->set_led_callback(ADL_BOOT_FAIL);
        }
        goto error;
    }
    if (andlink_client->devCbs->set_led_callback){
        andlink_client->devCbs->set_led_callback(ADL_BOOT_SUC);
    }
    cJSON* deviceManageUrl_json = cJSON_GetObjectItemCaseSensitive(http_post_data_json, "deviceManageUrl");
    cJSON* mqttUrl_json = cJSON_GetObjectItemCaseSensitive(http_post_data_json, "mqttUrl");
    cJSON* mqttClientId_json = cJSON_GetObjectItemCaseSensitive(http_post_data_json, "mqttClientId");
    cJSON* mqttUser_json = cJSON_GetObjectItemCaseSensitive(http_post_data_json, "mqttUser");
    cJSON* mqttPassword_json = cJSON_GetObjectItemCaseSensitive(http_post_data_json, "mqttPassword");
    cJSON* mqttKeepalive_json = cJSON_GetObjectItemCaseSensitive(http_post_data_json, "mqttKeepalive");

    andlink_client->deviceManageUrl = calloc(1, strlen(deviceManageUrl_json->valuestring)+1);
    memcpy(andlink_client->deviceManageUrl, deviceManageUrl_json->valuestring, strlen(deviceManageUrl_json->valuestring));
    // LUAT_DEBUG_PRINT("mqttUrl_json->valuestring :%s",mqttUrl_json->valuestring);
    for (size_t i = 6; i < strlen(mqttUrl_json->valuestring); i++){
        if (mqttUrl_json->valuestring[i] == ':') {
            memcpy(andlink_client->mqttHost, mqttUrl_json->valuestring+6, i-6);
            andlink_client->mqttPort = atoi(mqttUrl_json->valuestring + i + 1);
            // LUAT_DEBUG_PRINT("andlink_client->mqttHost :%s:%d",andlink_client->mqttHost,andlink_client->mqttPort);
            break;
        }
    }
    andlink_client->mqttClientId = calloc(1, strlen(mqttClientId_json->valuestring)+1);
    memcpy(andlink_client->mqttClientId, mqttClientId_json->valuestring, strlen(mqttClientId_json->valuestring));
    andlink_client->mqttUser = calloc(1, strlen(mqttUser_json->valuestring)+1);
    memcpy(andlink_client->mqttUser, mqttUser_json->valuestring, strlen(mqttUser_json->valuestring));
    andlink_client->mqttPassword = calloc(1, strlen(mqttPassword_json->valuestring)+1);
    memcpy(andlink_client->mqttPassword, mqttPassword_json->valuestring, strlen(mqttPassword_json->valuestring));
    andlink_client->mqttKeepalive = (uint32_t)mqttKeepalive_json->valuedouble;

    if (http_post_data_json){
        cJSON_Delete(http_post_data_json);
        http_post_data_json = NULL;
    }

    if (andlink_client->http_post_data){
        free(andlink_client->http_post_data);
        andlink_client->http_post_data = NULL;
    }
    andlink_manage_report();
    andlink_client->report_timer = luat_create_rtos_timer(andlink_timer_report, andlink_client, NULL);
    luat_start_rtos_timer(andlink_client->report_timer, 20*60*60*1000, 1);

	luat_mqtt_ctrl_t *andlink_mqtt_client = (luat_mqtt_ctrl_t *)luat_heap_malloc(sizeof(luat_mqtt_ctrl_t));
	ret = luat_mqtt_init(andlink_mqtt_client, NW_ADAPTER_INDEX_LWIP_GPRS);
	if (ret) {
		LUAT_DEBUG_PRINT("mqtt init FAID ret %d", ret);
        goto error;
	}
    andlink_mqtt_client->userdata = luat_rtos_get_current_handle();
	andlink_mqtt_client->ip_addr.type = 0xff;
	luat_mqtt_connopts_t opts = {0};
    opts.is_tls = 1;
    opts.host = andlink_client->mqttHost;
	opts.port = andlink_client->mqttPort;
	ret = luat_mqtt_set_connopts(andlink_mqtt_client, &opts);
    mqtt_init(&(andlink_mqtt_client->broker), andlink_client->mqttClientId);
    mqtt_init_auth(&(andlink_mqtt_client->broker), andlink_client->mqttUser, andlink_client->mqttPassword);
	andlink_mqtt_client->broker.clean_session = 1;
	andlink_mqtt_client->keepalive = andlink_client->mqttKeepalive;

	andlink_mqtt_client->reconnect = 1;
	andlink_mqtt_client->reconnect_time = 3000;

	luat_mqtt_set_cb(andlink_mqtt_client,andlink_mqtt_cb);

	cJSON* will_json = cJSON_CreateObject();
    cJSON_AddStringToObject(will_json, "eventType", "Offline");
    cJSON_AddStringToObject(will_json, "deviceId", andlink_client->deviceId);
    cJSON_AddNumberToObject(will_json, "timestamp", (double)time(NULL));
	char* will_data = cJSON_Print(will_json);
    cJSON_Delete(will_json);
    luat_mqtt_set_will(andlink_mqtt_client, andlink_client->mqtt_pub_topic, will_data, strlen(will_data), 0, 0);
    cJSON_free(will_data);
	// luat_rtos_task_sleep(3000);
    luat_rtos_queue_create(&andlink_queue_handle, ANDLINK_QUEUE_SIZE, sizeof(andlink_queue_t));
	ret = luat_mqtt_connect(andlink_mqtt_client);
	if (ret) {
		LUAT_DEBUG_PRINT("mqtt connect ret=%d\n", ret);
		luat_mqtt_close_socket(andlink_mqtt_client);
		goto error;
	}

    andlink_queue_t andlink_queue;
    dn_dev_ctrl_frame_t ctrlFrame = {0};
    RESP_MODE_e mode;
    const uint8_t* ptr;
    char eventType[32]={0};
    char respData[256]={0};
    int respBufSize = 0;
    uint16_t message_id,topic_len,payload_len;
    while (1){
        if (luat_rtos_queue_recv(andlink_queue_handle, &andlink_queue, NULL, LUAT_WAIT_FOREVER) == 0){
            switch (andlink_queue.event){
                case ADL_MQTT_MSG_CONNACK:
                    // LUAT_DEBUG_PRINT("mqtt_connect ok");
                    if (andlink_client->devCbs->set_led_callback){
                        andlink_client->devCbs->set_led_callback(ADL_ONLINE);
                    }
                    // LUAT_DEBUG_PRINT("mqtt_subscribe");
                    uint16_t msgid = 0;
                    char andlink_sub_topic[128] = {0};
                    snprintf_(andlink_sub_topic, 128, ANDLINK_MQTT_DOWM, andlink_client->deviceId);
                    // LUAT_DEBUG_PRINT("andlink_sub_topic:%s",andlink_sub_topic);
                    mqtt_subscribe(&(andlink_queue.luat_mqtt_ctrl->broker), andlink_sub_topic, &msgid, 1);
                    // LUAT_DEBUG_PRINT("publish");
                    uint16_t message_id  = 0;
                    cJSON* online_json = cJSON_CreateObject();
                    cJSON_AddStringToObject(online_json, "deviceId", andlink_client->deviceId);
                    cJSON_AddStringToObject(online_json, "eventType", "MBoot");
                    cJSON_AddNumberToObject(online_json, "timestamp", (double)time(NULL));
                    cJSON_AddStringToObject(online_json, "deviceType", andlink_client->devAttr.deviceType);
                    char* online_data = cJSON_Print(online_json);
                    cJSON_Delete(online_json);
                    mqtt_publish_with_qos(&(andlink_queue.luat_mqtt_ctrl->broker), andlink_client->mqtt_pub_topic, online_data, strlen(online_data), 0, 1, &message_id);
                    cJSON_free(online_data);
                    break;
                case ADL_MQTT_MSG_PUBLISH:
                    topic_len = mqtt_parse_pub_topic_ptr(andlink_queue.luat_mqtt_ctrl->mqtt_packet_buffer, &ptr);
                    // LUAT_DEBUG_PRINT("pub_topic: %.*s",topic_len,ptr);
                    payload_len = mqtt_parse_pub_msg_ptr(andlink_queue.luat_mqtt_ctrl->mqtt_packet_buffer, &ptr);
                    // LUAT_DEBUG_PRINT("pub_msg: %.*s",payload_len,ptr);

                    cJSON* payload_json = cJSON_Parse(ptr);
                    cJSON* function_json = cJSON_GetObjectItemCaseSensitive(payload_json, "function");
                    cJSON* deviceId_json = cJSON_GetObjectItemCaseSensitive(payload_json, "deviceId");
                    cJSON* seqId_json = cJSON_GetObjectItemCaseSensitive(payload_json, "seqId");
                    cJSON* data_json = cJSON_GetObjectItemCaseSensitive(payload_json, "data");

                    memcpy(ctrlFrame.function, function_json->valuestring, strlen(function_json->valuestring));
                    memcpy(ctrlFrame.deviceId, deviceId_json->valuestring, strlen(deviceId_json->valuestring));
                    if (cJSON_IsObject(data_json)){
                        ctrlFrame.data = cJSON_Print(data_json);
                        ctrlFrame.dataLen = strlen(ctrlFrame.data);
                    }
                    if (strncmp("Unbind", function_json->valuestring, strlen(function_json->valuestring)) == 0){
                        mode = NORSP_MODE;
                        cJSON* unbind_json = cJSON_CreateObject();
                        cJSON_AddStringToObject(unbind_json, "eventType", "Response_Unbind");
                        cJSON_AddStringToObject(unbind_json, "deviceId", deviceId_json->valuestring);
                        cJSON_AddStringToObject(unbind_json, "seqId", seqId_json->valuestring);
                        char* unbind_data = cJSON_Print(unbind_json);
                        cJSON_Delete(unbind_json);
                        mqtt_publish_with_qos(&(andlink_queue.luat_mqtt_ctrl->broker), andlink_client->mqtt_pub_topic, unbind_data, strlen(unbind_data), 0, 1, &message_id);
                        cJSON_free(unbind_data);
                    }else{
                        mode = ASYNC_MODE;
                    }
                    if (andlink_client->devCbs->dn_send_cmd_callback){
                        andlink_client->devCbs->dn_send_cmd_callback(mode,&ctrlFrame,eventType,respData,respBufSize);
                        if (respBufSize){
                            //The synchronization is not used and will not be done.
                        }
                    }
                    if (ctrlFrame.data){
                        cJSON_free(ctrlFrame.data);
                    }
                    cJSON_Delete(payload_json);
                    break;
                case ADL_MQTT_MSG_CLOSE:
                    LUAT_DEBUG_PRINT("luat_mqtt_cb mqtt close");
                    if (andlink_client->devCbs->set_led_callback){
                        andlink_client->devCbs->set_led_callback(ADL_OFFLINE);
                    }
                    break;
                case ADL_MQTT_MSG_RELEASE:
                    LUAT_DEBUG_PRINT("luat_mqtt_cb mqtt release");
                    break;
                case ADL_TIMER_REPORT:
                    andlink_manage_report();
                    break;
                default:
                    break;
			}
        }
    }
    


    return 0;
error:
    if (http_post_data_json){
        cJSON_Delete(http_post_data_json);
        http_post_data_json = NULL;
    }
    andlink_destroy();
    return -1;
}


int devReset(void){
    return 0;
}

int devDataReport(char *childDevId, char *eventType, char *data, int dataLen){
    char seqId[9] = {0};
    uint16_t message_id  = 0;
    srand((unsigned) time(NULL));
    snprintf_(seqId, 8, "%d", rand());
    cJSON* report_json = cJSON_CreateObject();
    cJSON_AddStringToObject(report_json, "deviceId", andlink_client->deviceId);
    cJSON_AddStringToObject(report_json, "eventType", eventType); 
    cJSON_AddNumberToObject(report_json, "timestamp", (double)time(NULL));
    cJSON_AddStringToObject(report_json, "deviceType", andlink_client->devAttr.deviceType);
    cJSON_AddStringToObject(report_json, "seqId", seqId);
    cJSON_AddStringToObject(report_json, "data", data);
    char* report_data = cJSON_Print(report_json);
    cJSON_Delete(report_json);
    mqtt_publish_with_qos(&(andlink_client->andlink_mqtt_client->broker), andlink_client->mqtt_pub_topic, report_data, strlen(report_data), 0, 1, &message_id);
    cJSON_free(report_data);
    return 0;
}

int andlink_destroy(void){
    if (andlink_client == NULL){
        LUAT_DEBUG_PRINT("andlink_client already destroyed");
        return -1;
    }
    luat_rtos_queue_delete(andlink_queue_handle);
	if (andlink_client->report_timer){
        luat_stop_rtos_timer(andlink_client->report_timer);
		luat_release_rtos_timer(andlink_client->report_timer);
    	andlink_client->report_timer = NULL;
	}
    if (andlink_client->gwAddress2){
        free(andlink_client->gwAddress2);
        andlink_client->gwAddress2 = NULL;
    }
    if (andlink_client->deviceId){
        free(andlink_client->deviceId);
        andlink_client->deviceId = NULL;
    }
    if (andlink_client->dmToken){
        free(andlink_client->dmToken);
        andlink_client->dmToken = NULL;
    }
    if (andlink_client->gwToken){
        free(andlink_client->gwToken);
        andlink_client->gwToken = NULL;
    }
    if (andlink_client->deviceManageUrl){
        free(andlink_client->deviceManageUrl);
        andlink_client->deviceManageUrl = NULL;
    }
    if (andlink_client->mqttClientId){
        free(andlink_client->mqttClientId);
        andlink_client->mqttClientId = NULL;
    }
    if (andlink_client->mqttUser){
        free(andlink_client->mqttUser);
        andlink_client->mqttUser = NULL;
    }
    if (andlink_client->mqttPassword){
        free(andlink_client->mqttPassword);
        andlink_client->mqttPassword = NULL;
    }
    if (andlink_client->http_post_data){
        free(andlink_client->http_post_data);
        andlink_client->http_post_data = NULL;
    }
    if (andlink_client->andlink_http_client){
        luat_http_client_close(andlink_client->andlink_http_client);
        luat_http_client_destroy(&andlink_client->andlink_http_client);
        andlink_client->andlink_http_client = NULL;
    }
    if (andlink_client->andlink_mqtt_client){
        luat_mqtt_close_socket(andlink_client->andlink_mqtt_client);
        luat_mqtt_release_socket(andlink_client->andlink_mqtt_client);
        free(andlink_client->andlink_mqtt_client);
        andlink_client->andlink_mqtt_client = NULL;
    }
    free(andlink_client);
    andlink_client = NULL;
    return 0;
}

// Query device-specific information (called when obtaining device-specific information)
char *getDeviceInfoStr(EXPORT_DEVICE_ATTRS_e attr){
    if (attr == ADL_USER_KEY){
        return andlink_client->User_Key;
    }
    return NULL;
}




