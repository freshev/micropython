#include "luat_network_adapter.h"
#include "common_api.h"
#include "luat_debug.h"

#include "luat_rtos.h"
#include "luat_mobile.h"

#include "libemqtt.h"
#include "luat_mqtt.h"
#include "net_lwip.h"
#include "luat_mem.h"

/*This demo is to demonstrate mqtt docking
1. The mqtt peer version is 3.1.1, which supports TCP and TLS-TCP, but does not support mqtt over websocket.
2. This code supports qos0/1/2, but some cloud servers mqtt do not support qos2, which is not supported by this demo.
3. The implementation of QOS1/2 in this code does not have a resend mechanism. Failure to send will only disconnect the TCP connection.*/

// Whether to use encrypted link, i.e. MQTTS
#define MQTT_USE_SSL 			0
// Whether to use automatic reconnection, the default is to reconnect 3 seconds after disconnection
#define MQTT_USE_AUTORECONNECT 		1

// Fill in the server information below
#if (MQTT_USE_SSL == 1)
#define MQTT_HOST    	"airtest.openluat.com"   				//MQTTS server address and port number
#define MQTT_PORT		8883
#else
#define MQTT_HOST    	"lbsmqtt.airm2m.com"   				//MQTT server address and port number
#define MQTT_PORT		1884
#endif 

// MQTT triplet
static char mqtt_client_id[] = ""; // If left blank here, the following code will be filled in with IMEI
static char mqtt_client_username[] = "myuser";
static char mqtt_client_password[] = "mypassword";

// MQTT sending and receiving topic for demonstration

// Subscribed topic, downstream
static char mqtt_sub_topic[] = "test/csdk/sub";
//Published topic, upstream
static char mqtt_pub_topic[] = "test/csdk/pub";
//Test the test load of the topic
static char mqtt_send_payload[] = "hello mqtt_test!!!";

// static char mqtt_will_topic[] = "test_will"; // Test will
// static char mqtt_will_payload[] = "hello, i was dead";

#if (MQTT_USE_SSL == 1)
static const char *testCaCrt = \
{
    \
    "-----BEGIN CERTIFICATE-----\r\n"
    "MIIDUTCCAjmgAwIBAgIJAPPYCjTmxdt/MA0GCSqGSIb3DQEBCwUAMD8xCzAJBgNV\r\n" \
    "BAYTAkNOMREwDwYDVQQIDAhoYW5nemhvdTEMMAoGA1UECgwDRU1RMQ8wDQYDVQQD\r\n" \
    "DAZSb290Q0EwHhcNMjAwNTA4MDgwNjUyWhcNMzAwNTA2MDgwNjUyWjA/MQswCQYD\r\n" \
    "VQQGEwJDTjERMA8GA1UECAwIaGFuZ3pob3UxDDAKBgNVBAoMA0VNUTEPMA0GA1UE\r\n" \
    "AwwGUm9vdENBMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAzcgVLex1\r\n" \
    "EZ9ON64EX8v+wcSjzOZpiEOsAOuSXOEN3wb8FKUxCdsGrsJYB7a5VM/Jot25Mod2\r\n" \
    "juS3OBMg6r85k2TWjdxUoUs+HiUB/pP/ARaaW6VntpAEokpij/przWMPgJnBF3Ur\r\n" \
    "MjtbLayH9hGmpQrI5c2vmHQ2reRZnSFbY+2b8SXZ+3lZZgz9+BaQYWdQWfaUWEHZ\r\n" \
    "uDaNiViVO0OT8DRjCuiDp3yYDj3iLWbTA/gDL6Tf5XuHuEwcOQUrd+h0hyIphO8D\r\n" \
    "tsrsHZ14j4AWYLk1CPA6pq1HIUvEl2rANx2lVUNv+nt64K/Mr3RnVQd9s8bK+TXQ\r\n" \
    "KGHd2Lv/PALYuwIDAQABo1AwTjAdBgNVHQ4EFgQUGBmW+iDzxctWAWxmhgdlE8Pj\r\n" \
    "EbQwHwYDVR0jBBgwFoAUGBmW+iDzxctWAWxmhgdlE8PjEbQwDAYDVR0TBAUwAwEB\r\n" \
    "/zANBgkqhkiG9w0BAQsFAAOCAQEAGbhRUjpIred4cFAFJ7bbYD9hKu/yzWPWkMRa\r\n" \
    "ErlCKHmuYsYk+5d16JQhJaFy6MGXfLgo3KV2itl0d+OWNH0U9ULXcglTxy6+njo5\r\n" \
    "CFqdUBPwN1jxhzo9yteDMKF4+AHIxbvCAJa17qcwUKR5MKNvv09C6pvQDJLzid7y\r\n" \
    "E2dkgSuggik3oa0427KvctFf8uhOV94RvEDyqvT5+pgNYZ2Yfga9pD/jjpoHEUlo\r\n" \
    "88IGU8/wJCx3Ds2yc8+oBg/ynxG8f/HmCC1ET6EHHoe2jlo8FpU/SgGtghS1YL30\r\n" \
    "IWxNsPrUP+XsZpBJy/mvOhE5QXo6Y35zDqqj8tI7AGmAWu22jg==\r\n" \
    "-----END CERTIFICATE-----"
};
static const char *testclientCert = \
{
	\
	"-----BEGIN CERTIFICATE-----\r\n"
	"MIIDEzCCAfugAwIBAgIBATANBgkqhkiG9w0BAQsFADA/MQswCQYDVQQGEwJDTjER\r\n"
	"MA8GA1UECAwIaGFuZ3pob3UxDDAKBgNVBAoMA0VNUTEPMA0GA1UEAwwGUm9vdENB\r\n"
	"MB4XDTIwMDUwODA4MDY1N1oXDTMwMDUwNjA4MDY1N1owPzELMAkGA1UEBhMCQ04x\r\n"
	"ETAPBgNVBAgMCGhhbmd6aG91MQwwCgYDVQQKDANFTVExDzANBgNVBAMMBkNsaWVu\r\n"
	"dDCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAMy4hoksKcZBDbY680u6\r\n"
	"TS25U51nuB1FBcGMlF9B/t057wPOlxF/OcmbxY5MwepS41JDGPgulE1V7fpsXkiW\r\n"
	"1LUimYV/tsqBfymIe0mlY7oORahKji7zKQ2UBIVFhdlvQxunlIDnw6F9popUgyHt\r\n"
	"dMhtlgZK8oqRwHxO5dbfoukYd6J/r+etS5q26sgVkf3C6dt0Td7B25H9qW+f7oLV\r\n"
	"PbcHYCa+i73u9670nrpXsC+Qc7Mygwa2Kq/jwU+ftyLQnOeW07DuzOwsziC/fQZa\r\n"
	"nbxR+8U9FNftgRcC3uP/JMKYUqsiRAuaDokARZxVTV5hUElfpO6z6/NItSDvvh3i\r\n"
	"eikCAwEAAaMaMBgwCQYDVR0TBAIwADALBgNVHQ8EBAMCBeAwDQYJKoZIhvcNAQEL\r\n"
	"BQADggEBABchYxKo0YMma7g1qDswJXsR5s56Czx/I+B41YcpMBMTrRqpUC0nHtLk\r\n"
	"M7/tZp592u/tT8gzEnQjZLKBAhFeZaR3aaKyknLqwiPqJIgg0pgsBGITrAK3Pv4z\r\n"
	"5/YvAJJKgTe5UdeTz6U4lvNEux/4juZ4pmqH4qSFJTOzQS7LmgSmNIdd072rwXBd\r\n"
	"UzcSHzsJgEMb88u/LDLjj1pQ7AtZ4Tta8JZTvcgBFmjB0QUi6fgkHY6oGat/W4kR\r\n"
	"jSRUBlMUbM/drr2PVzRc2dwbFIl3X+ZE6n5Sl3ZwRAC/s92JU6CPMRW02muVu6xl\r\n"
	"goraNgPISnrbpR6KjxLZkVembXzjNNc=\r\n" \
	"-----END CERTIFICATE-----"

};
static const char *testclientPk= \
{
	\
	"-----BEGIN RSA PRIVATE KEY-----\r\n"
	"MIIEpAIBAAKCAQEAzLiGiSwpxkENtjrzS7pNLblTnWe4HUUFwYyUX0H+3TnvA86X\r\n"
	"EX85yZvFjkzB6lLjUkMY+C6UTVXt+mxeSJbUtSKZhX+2yoF/KYh7SaVjug5FqEqO\r\n"
	"LvMpDZQEhUWF2W9DG6eUgOfDoX2milSDIe10yG2WBkryipHAfE7l1t+i6Rh3on+v\r\n"
	"561LmrbqyBWR/cLp23RN3sHbkf2pb5/ugtU9twdgJr6Lve73rvSeulewL5BzszKD\r\n"
	"BrYqr+PBT5+3ItCc55bTsO7M7CzOIL99BlqdvFH7xT0U1+2BFwLe4/8kwphSqyJE\r\n"
 	"C5oOiQBFnFVNXmFQSV+k7rPr80i1IO++HeJ6KQIDAQABAoIBAGWgvPjfuaU3qizq\r\n"
	"uti/FY07USz0zkuJdkANH6LiSjlchzDmn8wJ0pApCjuIE0PV/g9aS8z4opp5q/gD\r\n"
	"UBLM/a8mC/xf2EhTXOMrY7i9p/I3H5FZ4ZehEqIw9sWKK9YzC6dw26HabB2BGOnW\r\n"
	"5nozPSQ6cp2RGzJ7BIkxSZwPzPnVTgy3OAuPOiJytvK+hGLhsNaT+Y9bNDvplVT2\r\n"
	"ZwYTV8GlHZC+4b2wNROILm0O86v96O+Qd8nn3fXjGHbMsAnONBq10bZS16L4fvkH\r\n"
	"5G+W/1PeSXmtZFppdRRDxIW+DWcXK0D48WRliuxcV4eOOxI+a9N2ZJZZiNLQZGwg\r\n"
	"w3A8+mECgYEA8HuJFrlRvdoBe2U/EwUtG74dcyy30L4yEBnN5QscXmEEikhaQCfX\r\n"
	"Wm6EieMcIB/5I5TQmSw0cmBMeZjSXYoFdoI16/X6yMMuATdxpvhOZGdUGXxhAH+x\r\n"
	"xoTUavWZnEqW3fkUU71kT5E2f2i+0zoatFESXHeslJyz85aAYpP92H0CgYEA2e5A\r\n"
	"Yozt5eaA1Gyhd8SeptkEU4xPirNUnVQHStpMWUb1kzTNXrPmNWccQ7JpfpG6DcYl\r\n"
	"zUF6p6mlzY+zkMiyPQjwEJlhiHM2NlL1QS7td0R8ewgsFoyn8WsBI4RejWrEG9td\r\n"
	"EDniuIw+pBFkcWthnTLHwECHdzgquToyTMjrBB0CgYEA28tdGbrZXhcyAZEhHAZA\r\n"
	"Gzog+pKlkpEzeonLKIuGKzCrEKRecIK5jrqyQsCjhS0T7ZRnL4g6i0s+umiV5M5w\r\n"
	"fcc292pEA1h45L3DD6OlKplSQVTv55/OYS4oY3YEJtf5mfm8vWi9lQeY8sxOlQpn\r\n"
	"O+VZTdBHmTC8PGeTAgZXHZUCgYA6Tyv88lYowB7SN2qQgBQu8jvdGtqhcs/99GCr\r\n"
	"H3N0I69LPsKAR0QeH8OJPXBKhDUywESXAaEOwS5yrLNP1tMRz5Vj65YUCzeDG3kx\r\n"
	"gpvY4IMp7ArX0bSRvJ6mYSFnVxy3k174G3TVCfksrtagHioVBGQ7xUg5ltafjrms\r\n"
	"n8l55QKBgQDVzU8tQvBVqY8/1lnw11Vj4fkE/drZHJ5UkdC1eenOfSWhlSLfUJ8j\r\n"
 	"ds7vEWpRPPoVuPZYeR1y78cyxKe1GBx6Wa2lF5c7xjmiu0xbRnrxYeLolce9/ntp\r\n"
	"asClqpnHT8/VJYTD7Kqj0fouTTZf0zkig/y+2XERppd8k+pSKjUCPQ==\r\n" \
  	"-----END RSA PRIVATE KEY-----"

};
#endif

static luat_rtos_task_handle mqtt_task_handle;
static luat_rtos_queue_t mqtt_queue_handle;
#define MQTT_QUEUE_SIZE 128

typedef struct{
	luat_mqtt_ctrl_t *luat_mqtt_ctrl;
	uint16_t event;
} mqttQueueData;

luat_rtos_mutex_t mutex = NULL;

static void luat_mqtt_cb(luat_mqtt_ctrl_t *luat_mqtt_ctrl, uint16_t event){
	
	int ret = luat_rtos_mutex_lock(mutex,1000);
	if (ret != 0)
	{
		LUAT_DEBUG_PRINT("mqtt sub cb timout or lock open fail eror ret %d", ret);
		return;
	}
	
	mqttQueueData mqtt_cb_event = {.luat_mqtt_ctrl = luat_mqtt_ctrl,.event = event};
	luat_rtos_queue_send(mqtt_queue_handle, &mqtt_cb_event, NULL, 0);
	
	ret = luat_rtos_mutex_unlock(mutex);
	if (ret != 0)
	{
		LUAT_DEBUG_PRINT("unlock fail eror ret %d", ret);
	}
	
	return;
}

static void luat_mqtt_task(void *param)
{
	(void)param;
	// Prepare necessary variables
	int ret = -1;
	// Transmit and receive queue, because the callback function of tcp/ip cannot be blocked
	LUAT_DEBUG_PRINT("1. create task queue");
	mqttQueueData mqttQueueRecv = {0};
	luat_rtos_queue_create(&mqtt_queue_handle, MQTT_QUEUE_SIZE, sizeof(mqttQueueData));
	
	ret = luat_rtos_mutex_create(&mutex);  //Create lock
	LUAT_DEBUG_PRINT("mutex init FAID ret %d", ret);

	if (ret != 0)
	{
		LUAT_DEBUG_PRINT("mutex init FAID ret %d", ret);
		luat_rtos_task_delete(NULL);
		return;
	}
	
	//Create network adapter bearer
	LUAT_DEBUG_PRINT("2. create network ctrl");
	luat_mqtt_ctrl_t *luat_mqtt_ctrl = (luat_mqtt_ctrl_t *)luat_heap_malloc(sizeof(luat_mqtt_ctrl_t));
	memset(luat_mqtt_ctrl, 0, sizeof(luat_mqtt_ctrl_t));
	// The GPRS used here is just an alias, common to cellular networks.
	ret = luat_mqtt_init(luat_mqtt_ctrl, NW_ADAPTER_INDEX_LWIP_GPRS);
	if (ret) {
		LUAT_DEBUG_PRINT("mqtt init FAID ret %d", ret);
		luat_rtos_mutex_delete(mutex); //Destroy the lock
		luat_rtos_task_delete(NULL);
		return;
	}
	//Configure connection parameters
	LUAT_DEBUG_PRINT("3. configure mqtt");
	luat_mqtt_ctrl->ip_addr.type = 0xff;
	luat_mqtt_connopts_t opts = {0};

#if (MQTT_USE_SSL == 1)
	opts.is_tls = 1;
	opts.server_cert = testCaCrt;
	opts.server_cert_len = strlen(testCaCrt);
	opts.client_cert = testclientCert;
	opts.client_cert_len = strlen(testclientCert);
	opts.client_key = testclientPk;
	opts.client_key_len = strlen(testclientPk);
#else
	opts.is_tls = 0;
#endif 
	opts.host = MQTT_HOST;
	opts.port = MQTT_PORT;
	LUAT_DEBUG_PRINT("  host %s port %d", MQTT_HOST, MQTT_PORT);
	ret = luat_mqtt_set_connopts(luat_mqtt_ctrl, &opts);
	if (ret) {
		LUAT_DEBUG_PRINT("mqtt set connopts error ret %d", ret);
		luat_rtos_mutex_delete(mutex); //Destroy the lock
		luat_rtos_task_delete(NULL);
		return;
	}
	char imei[32] = {0};
	ret = luat_mobile_get_imei(0, imei, sizeof(imei)-1);
	
	if (strlen(mqtt_client_id) == 0) {
		LUAT_DEBUG_PRINT("  client id %s", imei);
		mqtt_init(&(luat_mqtt_ctrl->broker), imei);
	}
	else {
		LUAT_DEBUG_PRINT("  client id %s", mqtt_client_id);
		mqtt_init(&(luat_mqtt_ctrl->broker), mqtt_client_id);
	}

	mqtt_init_auth(&(luat_mqtt_ctrl->broker), mqtt_client_username, mqtt_client_password);

	// luat_mqtt_ctrl->netc->is_debug = 1;// debug信息
	luat_mqtt_ctrl->broker.clean_session = 1;
	luat_mqtt_ctrl->keepalive = 240;

	if (MQTT_USE_AUTORECONNECT == 1)
	{
		luat_mqtt_ctrl->reconnect = 1;
		luat_mqtt_ctrl->reconnect_time = 3000;
	}

	// luat_mqtt_set_will(luat_mqtt_ctrl, mqtt_will_topic, mqtt_will_payload, strlen(mqtt_will_payload), 0, 0); // 测试遗嘱
	
	LUAT_DEBUG_PRINT("4. setup mqtt callback");
	luat_mqtt_set_cb(luat_mqtt_ctrl,luat_mqtt_cb);

	LUAT_DEBUG_PRINT("5. start mqtt connect");
	ret = luat_mqtt_connect(luat_mqtt_ctrl);
	if (ret) {
		LUAT_DEBUG_PRINT("mqtt connect ret=%d\n", ret);
		luat_mqtt_close_socket(luat_mqtt_ctrl);
		luat_rtos_mutex_delete(mutex); //Destroy the lock
		return;
	}
	LUAT_DEBUG_PRINT("6. wait mqtt event");

	uint16_t message_id  = 0;
	while(1){
        if (luat_rtos_queue_recv(mqtt_queue_handle, &mqttQueueRecv, NULL, 5000) == 0){
			switch (mqttQueueRecv.event)
			{
			case MQTT_MSG_CONNACK:{
				LUAT_DEBUG_PRINT("7. mqtt connect ok, get conack");

				LUAT_DEBUG_PRINT("8. mqtt subscribe topic");
				LUAT_DEBUG_PRINT("  subscribe %s", mqtt_sub_topic);
				ret = mqtt_subscribe(&(mqttQueueRecv.luat_mqtt_ctrl->broker), mqtt_sub_topic, &message_id, 1);
				LUAT_DEBUG_PRINT("  subscribe %s id %d ret %d", mqtt_sub_topic, message_id, ret);

				LUAT_DEBUG_PRINT("9. mqtt publish one message for test");
				LUAT_DEBUG_PRINT("  publish %s %s", mqtt_pub_topic, mqtt_send_payload);
				ret = mqtt_publish_with_qos(&(mqttQueueRecv.luat_mqtt_ctrl->broker), mqtt_pub_topic, mqtt_send_payload, strlen(mqtt_send_payload), 0, 1, &message_id);
				LUAT_DEBUG_PRINT("  publish %s id %d ret %d", mqtt_pub_topic, message_id, ret);
				break;
			}
			case MQTT_MSG_PUBLISH : {
				const uint8_t* ptr;
				LUAT_DEBUG_PRINT("10. downlink publish message");
				uint16_t topic_len = mqtt_parse_pub_topic_ptr(mqttQueueRecv.luat_mqtt_ctrl->mqtt_packet_buffer, &ptr);
				LUAT_DEBUG_PRINT("  topic: %.*s",topic_len,ptr);
				uint16_t payload_len = mqtt_parse_pub_msg_ptr(mqttQueueRecv.luat_mqtt_ctrl->mqtt_packet_buffer, &ptr);
				LUAT_DEBUG_PRINT("  message len %d", payload_len);
				for (uint16_t i = 0; i < payload_len; i+=512)
				{
					size_t loglen = payload_len - i;
					if (loglen > 512)
						loglen = 512;
					LUAT_DEBUG_PRINT("  message: %.*s",loglen,ptr + i);
				}
				break;
			}
			case MQTT_MSG_PUBACK : 
			case MQTT_MSG_PUBCOMP : {
				LUAT_DEBUG_PRINT("11. message PUBACK/PUBCOMP");
				message_id = mqtt_parse_msg_id(mqttQueueRecv.luat_mqtt_ctrl->mqtt_packet_buffer);
				LUAT_DEBUG_PRINT("  msg_id: %d", message_id);
				break;
			}
			case MQTT_MSG_RELEASE : {
				// Generally does not appear
				LUAT_DEBUG_PRINT("11. message RELEASE");
				break;
			}
			case MQTT_MSG_DISCONNECT : { // mqtt disconnection (will be reported as long as there is a disconnection, regardless of whether it is reconnected)
				LUAT_DEBUG_PRINT("12. mqtt disconnect!!");
				break;
			}
			case MQTT_MSG_CLOSE : { // mqtt closes (will not reconnect) Note: Be sure to pay attention to the difference with MQTT_MSG_DISCONNECT. If you want to do manual reconnection, it is recommended here */
				LUAT_DEBUG_PRINT("13. mqtt close");
				if (MQTT_USE_AUTORECONNECT == 0){
					LUAT_DEBUG_PRINT("14. mqtt manual reconnect");
					while (1) {
						LUAT_DEBUG_PRINT("  wait 3s before connect");
						luat_rtos_task_sleep(3000);
						ret = luat_mqtt_connect(mqttQueueRecv.luat_mqtt_ctrl);
						if (ret) {
							LUAT_DEBUG_PRINT("   mqtt connect ret=%d\n", ret);
							luat_mqtt_close_socket(mqttQueueRecv.luat_mqtt_ctrl);
							continue;
						}
						LUAT_DEBUG_PRINT("  luat_mqtt_connect ret 0");
						break;
					}
				}
				break;
			}
			default:
				break;
			}
        }else{
			if (luat_mqtt_state_get(luat_mqtt_ctrl) == MQTT_STATE_READY){
				LUAT_DEBUG_PRINT("15. wait event timeout , publish demo message");
				mqtt_publish_with_qos(&(luat_mqtt_ctrl->broker), mqtt_pub_topic, mqtt_send_payload, strlen(mqtt_send_payload), 0, 1, &message_id);
			}
		}
	}
	LUAT_DEBUG_PRINT("20. mqtt task done");
	        
	luat_rtos_mutex_delete(mutex); //Destroy the lock
	
	luat_rtos_task_delete(NULL);
}

static void luatos_mobile_event_callback(LUAT_MOBILE_EVENT_E event, uint8_t index, uint8_t status)
{
	if (LUAT_MOBILE_EVENT_NETIF == event)
	{
		if (LUAT_MOBILE_NETIF_LINK_ON == status)
		{
			LUAT_DEBUG_PRINT("luatos_mobile_event_callback  link ...");
			luat_socket_check_ready(index, NULL);
		}
        else if(LUAT_MOBILE_NETIF_LINK_OFF == status || LUAT_MOBILE_NETIF_LINK_OOS == status)
        {
            LUAT_DEBUG_PRINT("luatos_mobile_event_callback  error ...");
        }
	}
}

static void luat_libemqtt_init(void)
{
	luat_mobile_event_register_handler(luatos_mobile_event_callback);
	net_lwip_init();
	net_lwip_register_adapter(NW_ADAPTER_INDEX_LWIP_GPRS);
	network_register_set_default(NW_ADAPTER_INDEX_LWIP_GPRS);
	luat_rtos_task_create(&mqtt_task_handle, 8 * 1024, 10, "libemqtt", luat_mqtt_task, NULL, 16);
}

INIT_TASK_EXPORT(luat_libemqtt_init, "1");
