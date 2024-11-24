/*@Modules mqtt
@summary mqtt client
@version 1.0
@date 2022.08.25
@demo mqtt
@tagLUAT_USE_NETWORK
@usage
-- Please check the demo for specific usage.
-- This library supports mqtt 3.1.1, other versions such as 3.1 or 5 are not supported
-- Only supports pure MQTT/MQTTS communication, does not support mqtt over websocket

-- Several major premises:
-- This library is based on TCP link and supports encrypted TCP and non-encrypted TCP.
-- Any communication failure will disconnect the connection. If automatic reconnection is turned on, automatic reconnection will start after an interval of N seconds.
--The upstream data is all one-time, there is no caching mechanism, and there is no upstream retry/resend mechanism.
-- How to know that the sending is successful: trigger the event == "sent" in mqttc:on

--Explanation on the QOS value when publishing, the behavior of the special Modules uplinking to the cloud/server:
-- QOS0, pushed into the underlying TCP sending stack, considered successful
-- QOS1, receiving a PUBACK response from the server is considered successful.
-- QOS2, after receiving the server response PUBREC, the upstream PUBCOMP is immediately pushed into the TCP sending queue, which is regarded as successful.

-- Say important things three times: no resend mechanism, no resend mechanism, no resend mechanism
-- 1. The MQTT protocol stipulates a retransmission mechanism, but that is a mechanism that can only be implemented on the cloud/server side, not on the Modules side.
-- 2. If the uplink fails, the only possibility is that there is a problem with the TCP link, and the solution to the problem with the TCP link is to reconnect.
-- 3. The Modules will not save any upstream data, and retransmission cannot be achieved after reconnection.

--The business needs to determine whether the uplink is successful and how to solve it:
-- It is recommended to use QOS1 first, then monitor/judge the sent event, and select a timeout, which can meet 99.9% of the needs.
-- Using QOS2, there is a theoretical possibility that PUBCOMP uplink failure may cause the server to not broadcast data.
-- There is code in the demo that demonstrates waiting for the sent event, similar to sys.waitUntil("mqtt_sent", 3000). Search for the mqtt_sent keyword*/

#include "luat_base.h"

#include "luat_network_adapter.h"
#include "libemqtt.h"
#include "luat_rtos.h"
#include "luat_zbuff.h"
#include "luat_mem.h"
#include "luat_mqtt.h"

#define LUAT_LOG_TAG "mqtt"
#include "luat_log.h"

#define LUAT_MQTT_CTRL_TYPE "MQTTCTRL*"

static luat_mqtt_ctrl_t * get_mqtt_ctrl(lua_State *L){
	if (luaL_testudata(L, 1, LUAT_MQTT_CTRL_TYPE)){
		return ((luat_mqtt_ctrl_t *)luaL_checkudata(L, 1, LUAT_MQTT_CTRL_TYPE));
	}else{
		return ((luat_mqtt_ctrl_t *)lua_touserdata(L, 1));
	}
}

int32_t luatos_mqtt_callback(lua_State *L, void* ptr){
	(void)ptr;
    rtos_msg_t* msg = (rtos_msg_t*)lua_topointer(L, -1);
    luat_mqtt_ctrl_t *mqtt_ctrl =(luat_mqtt_ctrl_t *)msg->ptr;
    switch (msg->arg1) {
//		case MQTT_MSG_TCP_TX_DONE:
//			break;
		case MQTT_MSG_TIMER_PING : {
			luat_mqtt_ping(mqtt_ctrl);
			break;
		}
		case MQTT_MSG_RECONNECT : {
			luat_mqtt_reconnect(mqtt_ctrl);
			break;
		}
		case MQTT_MSG_PUBLISH : {
			luat_mqtt_msg_t *mqtt_msg =(luat_mqtt_msg_t *)msg->arg2;
			if (mqtt_ctrl->mqtt_cb) {
				luat_mqtt_msg_t *mqtt_msg =(luat_mqtt_msg_t *)msg->arg2;
				lua_geti(L, LUA_REGISTRYINDEX, mqtt_ctrl->mqtt_cb);
				if (lua_isfunction(L, -1)) {
					lua_geti(L, LUA_REGISTRYINDEX, mqtt_ctrl->mqtt_ref);
					lua_pushstring(L, "recv");
					lua_pushlstring(L, (const char*)(mqtt_msg->data),mqtt_msg->topic_len);
					lua_pushlstring(L, (const char*)(mqtt_msg->data+mqtt_msg->topic_len),mqtt_msg->payload_len);
					
					//Add a return value meta, type is table, including qos, retain and dup
					// 	mqttc:on(function(mqtt_client, event, data, payload, meta)
            		// 		if event == "recv" then
            		//     	log.info("mqtt recv", "topic", data)
            		//     	log.info("mqtt recv", 'payload', payload)
            		//     	log.info("mqtt recv", 'meta.qos', meta.qos)
            		//     	log.info("mqtt recv", 'meta.retain', meta.retain)
            		//     	log.info("mqtt recv", 'meta.dup', meta.dup)
					lua_createtable(L, 0, 3);

					lua_pushliteral(L, "qos"); 
					lua_pushinteger(L, MQTTParseMessageQos(mqtt_ctrl->mqtt_packet_buffer));
					lua_settable(L, -3);

					lua_pushliteral(L, "retain"); 
					lua_pushinteger(L, MQTTParseMessageRetain(mqtt_ctrl->mqtt_packet_buffer));
					lua_settable(L, -3);

					lua_pushliteral(L, "dup"); 
					lua_pushinteger(L, MQTTParseMessageDuplicate(mqtt_ctrl->mqtt_packet_buffer));
					lua_settable(L, -3);

					// lua_call(L, 4, 0);
					lua_call(L, 5, 0);
				}
            }
			luat_heap_free(mqtt_msg);
            break;
        }
        case MQTT_MSG_CONNACK: {
			if (mqtt_ctrl->mqtt_cb) {
				lua_geti(L, LUA_REGISTRYINDEX, mqtt_ctrl->mqtt_cb);
				if (lua_isfunction(L, -1)) {
					lua_geti(L, LUA_REGISTRYINDEX, mqtt_ctrl->mqtt_ref);
					lua_pushstring(L, "conack");
					lua_call(L, 2, 0);
				}
				lua_getglobal(L, "sys_pub");
				if (lua_isfunction(L, -1)) {
					lua_pushstring(L, "MQTT_CONNACK");
					lua_geti(L, LUA_REGISTRYINDEX, mqtt_ctrl->mqtt_ref);
					lua_call(L, 2, 0);
				}
            }
            break;
        }
		case MQTT_MSG_PUBACK:
		case MQTT_MSG_PUBCOMP: {
			if (mqtt_ctrl->mqtt_cb) {
				lua_geti(L, LUA_REGISTRYINDEX, mqtt_ctrl->mqtt_cb);
				if (lua_isfunction(L, -1)) {
					lua_geti(L, LUA_REGISTRYINDEX, mqtt_ctrl->mqtt_ref);
					lua_pushstring(L, "sent");
					lua_pushinteger(L, msg->arg2);
					lua_call(L, 3, 0);
				}
            }
            break;
        }
		case MQTT_MSG_RELEASE: {
			if (mqtt_ctrl->mqtt_ref) {
				luaL_unref(L, LUA_REGISTRYINDEX, mqtt_ctrl->mqtt_ref);
				mqtt_ctrl->mqtt_ref = 0;
            }
            break;
        }
		case MQTT_MSG_CLOSE: {
			if (mqtt_ctrl->mqtt_ref) {
				lua_geti(L, LUA_REGISTRYINDEX, mqtt_ctrl->mqtt_cb);
				if (lua_isfunction(L, -1)) {
					lua_geti(L, LUA_REGISTRYINDEX, mqtt_ctrl->mqtt_ref);
					lua_pushstring(L, "close");
					lua_call(L, 2, 0);
				}
            }
            break;
        }
		case MQTT_MSG_DISCONNECT: {
			if (mqtt_ctrl->mqtt_cb) {
				lua_geti(L, LUA_REGISTRYINDEX, mqtt_ctrl->mqtt_cb);
				if (lua_isfunction(L, -1)) {
					lua_geti(L, LUA_REGISTRYINDEX, mqtt_ctrl->mqtt_ref);
					lua_pushstring(L, "disconnect");
					lua_pushnumber(L, mqtt_ctrl->error_state);
					lua_call(L, 3, 0);
				}
				lua_getglobal(L, "sys_pub");
				if (lua_isfunction(L, -1)) {
					lua_pushstring(L, "MQTT_DISCONNECT");
					lua_geti(L, LUA_REGISTRYINDEX, mqtt_ctrl->mqtt_ref);
					lua_pushnumber(L, mqtt_ctrl->error_state);
					lua_call(L, 3, 0);
				}
            }
            break;
        }
		case MQTT_MSG_SUBACK:
			break;
		case MQTT_MSG_UNSUBACK:
			break;
		default : {
			LLOGD("l_mqtt_callback error arg1:%d",msg->arg1);
            break;
        }
    }
    // lua_pushinteger(L, 0);
    return 0;
}

/*Subscribe to topic
@api mqttc:subscribe(topic, qos)
@string/table theme
@int Effective when topic is string 0/1/2 Default 0
@return int message id, valid when qos is 1/2, if the bottom layer fails to return, nil will be returned
@usage
-- Subscribe to a single topic, and qos=0
mqttc:subscribe("/luatos/123456", 0)
-- Subscribe to a single topic, and qos=1
mqttc:subscribe("/luatos/12345678", 1)
--Subscribe to multiple topics and use different qos
mqttc:subscribe({["/luatos/1234567"]=1,["/luatos/12345678"]=2})*/
static int l_mqtt_subscribe(lua_State *L) {
	size_t len = 0;
	int ret = 1;
	uint16_t msgid = 0;
	luat_mqtt_ctrl_t * mqtt_ctrl = (luat_mqtt_ctrl_t *)lua_touserdata(L, 1);
	if (lua_isstring(L, 2)){
		const char * topic = luaL_checklstring(L, 2, &len);
		uint8_t qos = luaL_optinteger(L, 3, 0);
		ret = mqtt_subscribe(&(mqtt_ctrl->broker), topic, &msgid, qos);
	}else if(lua_istable(L, 2)){
		lua_pushnil(L);
		while (lua_next(L, 2) != 0) {
			ret &= mqtt_subscribe(&(mqtt_ctrl->broker), lua_tostring(L, -2), &msgid, luaL_optinteger(L, -1, 0)) == 1 ? 1 : 0;
			lua_pop(L, 1);
		}
	}
	if (ret == 1) {
		lua_pushinteger(L, msgid);
		return 1;
	}
	else {
		return 0;
	}
}

/*Unsubscribe from topic
@api mqttc:unsubscribe(topic)
@string/table theme
@usage
mqttc:unsubscribe("/luatos/123456")
mqttc:unsubscribe({"/luatos/1234567","/luatos/12345678"})*/
static int l_mqtt_unsubscribe(lua_State *L) {
	size_t len = 0;
	int ret = 0;
	uint16_t msgid = 0;
	luat_mqtt_ctrl_t * mqtt_ctrl = (luat_mqtt_ctrl_t *)lua_touserdata(L, 1);
	if (lua_isstring(L, 2)){
		const char * topic = luaL_checklstring(L, 2, &len);
		ret = mqtt_unsubscribe(&(mqtt_ctrl->broker), topic, &msgid);
	}else if(lua_istable(L, 2)){
		size_t count = lua_rawlen(L, 2);
		for (size_t i = 1; i <= count; i++){
			lua_geti(L, 2, i);
			const char * topic = luaL_checklstring(L, -1, &len);
			ret &= mqtt_unsubscribe(&(mqtt_ctrl->broker), topic, &msgid) == 1 ? 1 : 0;
			lua_pop(L, 1);
		}
	}
	if (ret == 1) {
		lua_pushinteger(L, msgid);
		return 1;
	}
	return 0;
}

/*Configure whether to turn on debug information
@api mqttc:debug(onoff)
@boolean whether to turn on the debug switch
@return nil no return value
@usage mqttc:debug(true)*/
static int l_mqtt_set_debug(lua_State *L){
	luat_mqtt_ctrl_t * mqtt_ctrl = get_mqtt_ctrl(L);
	if (lua_isboolean(L, 2)){
		mqtt_ctrl->netc->is_debug = lua_toboolean(L, 2);
	}
	return 0;
}

/*mqtt client creation
@api mqtt.create(adapter,host,port,ssl,isipv6)
@int Adapter serial number, which can only be socket.ETH0, socket.STA, socket.AP. If not filled in, the platform's own method will be selected, and then the last registered adapter will be selected.
@string server address, which can be a domain name or an IP address
@int port number
@bool/table Whether to use SSL encrypted connection, the default is not encryption, true is the simplest encryption without certificate, table is encryption with certificate<br>server_cert server ca certificate data<br>client_cert client certificate data<br>client_key client Client private key encrypted data<br>client_password Client private key password data<br>verify Whether to force verification 0 No verification/1 Optional verification/2 Mandatory verification Default 2
@bool/table bool whether it is ipv6, the default is not table mqtt extended parameters, ipv6 whether it is ipv6, rxSize receive buffer size
@return userdata If successful, the mqtt client instance will be returned, otherwise nil will be returned.
@usage
-- Ordinary TCP link
mqttc = mqtt.create(nil,"120.55.137.106", 1884)
-- Ordinary TCP link, mqtt receiving buffer 4096
mqttc = mqtt.create(nil,"120.55.137.106", 1884, nil, {rxSize = 4096})
-- Encrypt TCP link, do not verify server certificate
mqttc = mqtt.create(nil,"120.55.137.106", 8883, true)
-- Encrypted TCPTCP link, single server certificate verification
mqttc = mqtt.create(nil,"120.55.137.106", 8883, {server_cert=io.readFile("/luadb/ca.crt")})
-- Encrypted TCPTCP link, single server certificate verification, but optional authentication
mqttc = mqtt.create(nil,"120.55.137.106", 8883, {server_cert=io.readFile("/luadb/ca.crt"), verify=1})
-- Encrypted TCPTCP link, two-way certificate verification
mqttc = mqtt.create(nil,"120.55.137.106", 8883, {
server_cert=io.readFile("/luadb/ca.crt"),
client_cert=io.readFile("/luadb/client.pem"),
client_key="123456",
client_password="123456",
})*/
static int l_mqtt_create(lua_State *L) {
	int ret = 0;
	int adapter_index = luaL_optinteger(L, 1, network_get_last_register_adapter());
	if (adapter_index < 0 || adapter_index >= NW_ADAPTER_QTY){
		LLOGE("No network adapter registered yet");
		return 0;
	}
	luat_mqtt_ctrl_t *mqtt_ctrl = (luat_mqtt_ctrl_t *)lua_newuserdata(L, sizeof(luat_mqtt_ctrl_t));
	if (!mqtt_ctrl){
		LLOGE("out of memory when malloc mqtt_ctrl");
		return 0;
	}

	ret = luat_mqtt_init(mqtt_ctrl, adapter_index);
	if (ret) {
		LLOGE("mqtt init FAID ret %d", ret);
		return 0;
	}

	luat_mqtt_connopts_t opts = {0};

	//Related to connection parameters
	// const char *ip;
	size_t ip_len = 0;
	network_set_ip_invaild(&mqtt_ctrl->ip_addr);
	if (lua_isinteger(L, 2)){
		network_set_ip_ipv4(&mqtt_ctrl->ip_addr, lua_tointeger(L, 2));
		// ip = NULL;
		ip_len = 0;
	}else{
		ip_len = 0;
		opts.host = luaL_checklstring(L, 2, &ip_len);
		// TODO determines the length of the host. It will not work if it exceeds 191.
	}

	opts.port = luaL_checkinteger(L, 3);

	// Encryption related
	if (lua_isboolean(L, 4)){
		opts.is_tls = lua_toboolean(L, 4);
	}

	if (lua_istable(L, 4)){
		opts.is_tls = 1;
		opts.verify = 2;
		lua_pushstring(L, "verify");
		if (LUA_TNUMBER == lua_gettable(L, 4)) {
			opts.verify = luaL_checknumber(L, -1);
		}
		lua_pop(L, 1);

		lua_pushstring(L, "server_cert");
		if (LUA_TSTRING == lua_gettable(L, 4)) {
			opts.server_cert = luaL_checklstring(L, -1, &opts.server_cert_len);
		}
		lua_pop(L, 1);

		lua_pushstring(L, "client_cert");
		if (LUA_TSTRING == lua_gettable(L, 4)) {
			opts.client_cert = luaL_checklstring(L, -1, &opts.client_cert_len);
		}
		lua_pop(L, 1);

		lua_pushstring(L, "client_key");
		if (LUA_TSTRING == lua_gettable(L, 4)) {
			opts.client_key = luaL_checklstring(L, -1, &opts.client_key_len);
		}
		lua_pop(L, 1);

		lua_pushstring(L, "client_password");
		if (LUA_TSTRING == lua_gettable(L, 4)) {
			opts.client_password = luaL_checklstring(L, -1, &opts.client_password_len);
		}
		lua_pop(L, 1);
	}
	
	if (lua_isboolean(L, 5)){
		opts.is_ipv6 = lua_toboolean(L, 5);
	}else if(lua_istable(L, 5)){
		lua_pushstring(L, "ipv6");
		if (LUA_TBOOLEAN == lua_gettable(L, 5)) {
			opts.is_ipv6 = lua_toboolean(L, -1);
		}
		lua_pop(L, 1);

		lua_pushstring(L, "rxSize");
		if (LUA_TNUMBER == lua_gettable(L, 5)) {
			uint32_t len = luaL_checknumber(L, -1);
			if(len > 0)
				luat_mqtt_set_rxbuff_size(mqtt_ctrl, len);
		}
		lua_pop(L, 1);
	}

	ret = luat_mqtt_set_connopts(mqtt_ctrl, &opts);
	if (ret){
		LLOGE("Failed to set mqtt parameters");
		luat_mqtt_release_socket(mqtt_ctrl);
		return 0;
	}
	luaL_setmetatable(L, LUAT_MQTT_CTRL_TYPE);
	lua_pushvalue(L, -1);
	mqtt_ctrl->mqtt_ref = luaL_ref(L, LUA_REGISTRYINDEX);
	return 1;
}

/*mqtt triplet configuration and cleanSession
@api mqttc:auth(client_id,username,password,cleanSession)
@string device identification id. For the same mqtt server, it is usually required to be unique. The same client_id will kick each other offline.
@string account optional
@string password optional
@bool Clear session, default true, optional
@return nil no return value
@usage
-- Login without account and password, only clientId
mqttc:auth("123456789")
-- Log in with account and password
mqttc:auth("123456789","username","password")
--Additional configuration of cleanSession, not cleared
mqttc:auth("123456789","username","password", false)
-- No clientId mode, the server randomly generates the id, cleanSession is not configurable
mqttc:auth()*/
static int l_mqtt_auth(lua_State *L) {
	luat_mqtt_ctrl_t * mqtt_ctrl = get_mqtt_ctrl(L);
	const char *client_id = luaL_optstring(L, 2, "");
	const char *username = luaL_optstring(L, 3, "");
	const char *password = luaL_optstring(L, 4, "");
	int cleanSession = 1;
	if (lua_isboolean(L, 5) && !lua_toboolean(L, 5)) {
		cleanSession = 0;
	}
	mqtt_init(&(mqtt_ctrl->broker), client_id);
	mqtt_init_auth(&(mqtt_ctrl->broker), username, password);
	mqtt_ctrl->broker.clean_session = cleanSession;
	return 0;
}

/*mqtt heartbeat settings
@api mqttc:keepalive(time)
@int Optional unit s Default 240s. First 15, highest 600
@return nil no return value
@usage
mqttc:keepalive(30)*/
static int l_mqtt_keepalive(lua_State *L) {
	luat_mqtt_ctrl_t * mqtt_ctrl = get_mqtt_ctrl(L);
	int timeout = luaL_optinteger(L, 2, 240);
	if (timeout < 15)
		timeout = 15;
	if (timeout > 600)
		timeout = 600;
	mqtt_ctrl->keepalive = timeout;
	return 0;
}

/*Register mqtt callback
@api mqttc:on(cb)
@function cb mqtt callback, parameters include mqtt_client, event, data, payload
@return nil no return value
@usage
mqttc:on(function(mqtt_client, event, data, payload, metas)
--User-defined code
log.info("mqtt", "event", event, mqtt_client, data, payload)
end)
--[[
Possible values   for event are
  conack -- Server authentication is completed, the mqtt connection has been established, and data can be subscribed and published. There is no additional data.
  recv -- Received data, sent by the server, data is topic value (string), payload is business data (string). metas is metadata (table), generally not processed.
             -- metas contains the following content
-- qos value range 0,1,2
-- retain value range 0,1
-- dup value range 0,1
  sent -- the sending is completed, qos0 will notify immediately, qos1/qos2 will call back when the server responds, data is the message id
  disconnect -- The server disconnects, network problems or the server kicks the client, for example, clientId is repeated, business data is not reported after timeout
]]*/
static int l_mqtt_on(lua_State *L) {
	luat_mqtt_ctrl_t * mqtt_ctrl = get_mqtt_ctrl(L);
	if (mqtt_ctrl->mqtt_cb != 0) {
		luaL_unref(L, LUA_REGISTRYINDEX, mqtt_ctrl->mqtt_cb);
		mqtt_ctrl->mqtt_cb = 0;
	}
	if (lua_isfunction(L, 2)) {
		lua_pushvalue(L, 2);
		mqtt_ctrl->mqtt_cb = luaL_ref(L, LUA_REGISTRYINDEX);
	}
	return 0;
}

/*Connect to server
@api mqttc:connect()
@return boolean Returns true if the initiation is successful, otherwise returns false
@usage
-- Start establishing connection
mqttc:connect()
-- This function only represents successful initiation. Subsequently, it is still necessary to determine whether the mqtt connection is normal based on the ready function.*/
static int l_mqtt_connect(lua_State *L) {
	luat_mqtt_ctrl_t * mqtt_ctrl = get_mqtt_ctrl(L);
	int ret = luat_mqtt_connect(mqtt_ctrl);
	if (ret) {
		LLOGE("socket connect ret=%d\n", ret);
		luat_mqtt_close_socket(mqtt_ctrl);
		lua_pushboolean(L, 0);
		return 1;
	}
	lua_pushboolean(L, 1);
	return 1;
}

/*Disconnect from the server (resources will not be released)
@api mqttc:disconnect()
@return boolean Returns true if the initiation is successful, otherwise returns false
@usage
-- Disconnect
mqttc:disconnect()*/
static int l_mqtt_disconnect(lua_State *L) {
	luat_mqtt_ctrl_t * mqtt_ctrl = get_mqtt_ctrl(L);
	mqtt_disconnect(&(mqtt_ctrl->broker));
	luat_mqtt_close_socket(mqtt_ctrl);
	lua_pushboolean(L, 1);
	return 1;
}

/*Automatically reconnect
@api mqttc:autoreconn(reconnect, reconnect_time)
@bool Whether to automatically reconnect
@int Automatic reconnection period unit ms Default 3000ms
@usage
mqttc:autoreconn(true)*/
static int l_mqtt_autoreconn(lua_State *L) {
	luat_mqtt_ctrl_t * mqtt_ctrl = get_mqtt_ctrl(L);
	if (lua_isboolean(L, 2)){
		mqtt_ctrl->reconnect = lua_toboolean(L, 2);
	}
	mqtt_ctrl->reconnect_time = luaL_optinteger(L, 3, 3000);
	if (mqtt_ctrl->reconnect && mqtt_ctrl->reconnect_time < 1000)
		mqtt_ctrl->reconnect_time = 1000;
	return 0;
}

/*Post a message
@api mqttc:publish(topic, data, qos, retain)
@string topic, required
@string message, required, but the length can be 0
@int message level 0/1 default 0
@int Whether to archive, 0/1, default 0
@return int message id, it will be a valid value when qos is 1 or 2. If the bottom layer returns no, nil will be returned
@usage
mqttc:publish("/luatos/123456", "123")*/
static int l_mqtt_publish(lua_State *L) {
	uint16_t message_id  = 0;
	size_t payload_len = 0;
	luat_mqtt_ctrl_t * mqtt_ctrl = get_mqtt_ctrl(L);
	const char * topic = luaL_checkstring(L, 2);
	const char * payload = NULL;
	luat_zbuff_t *buff = NULL;
	if (lua_isstring(L, 3)){
		payload = luaL_checklstring(L, 3, &payload_len);
	}else if (luaL_testudata(L, 3, LUAT_ZBUFF_TYPE)){
		buff = ((luat_zbuff_t *)luaL_checkudata(L, 3, LUAT_ZBUFF_TYPE));
		payload = (const char*)buff->addr;
		payload_len = buff->used;
	}else{
		LLOGD("only support string or zbuff");
	}
	// LLOGD("payload_len:%d",payload_len);
	uint8_t qos = luaL_optinteger(L, 4, 0);
	uint8_t retain = luaL_optinteger(L, 5, 0);
	int ret = mqtt_publish_with_qos(&(mqtt_ctrl->broker), topic, payload, payload_len, retain, qos, &message_id);
	if (ret != 1){
		return 0;
	}
	if (qos == 0){
		rtos_msg_t msg = {0};
    	msg.handler = luatos_mqtt_callback;
		msg.ptr = mqtt_ctrl;
		msg.arg1 = MQTT_MSG_PUBACK;
		msg.arg2 = message_id;
		luat_msgbus_put(&msg, 0);
	}
	lua_pushinteger(L, message_id);
	return 1;
}

/*The mqtt client is closed (resources are released and can no longer be used after closing)
@api mqttc:close()
@usage
mqttc:close()*/
static int l_mqtt_close(lua_State *L) {
	luat_mqtt_ctrl_t * mqtt_ctrl = get_mqtt_ctrl(L);
	mqtt_disconnect(&(mqtt_ctrl->broker));
	luat_mqtt_close_socket(mqtt_ctrl);
	if (mqtt_ctrl->mqtt_cb != 0) {
		luaL_unref(L, LUA_REGISTRYINDEX, mqtt_ctrl->mqtt_cb);
		mqtt_ctrl->mqtt_cb = 0;
	}
	luat_mqtt_release_socket(mqtt_ctrl);
	return 0;
}

/*Is the mqtt client ready?
@api mqttc:ready()
@return bool whether the client is ready
@usage
local error = mqttc:ready()*/
static int l_mqtt_ready(lua_State *L) {
	luat_mqtt_ctrl_t * mqtt_ctrl = get_mqtt_ctrl(L);
	lua_pushboolean(L, mqtt_ctrl->mqtt_state == MQTT_STATE_READY ? 1 : 0);
	return 1;
}

/*mqtt client status
@api mqttc:state()
@return number client status
@usage
local state = mqttc:state()
-- Known status:
-- 0: MQTT_STATE_DISCONNECT
-- 1: MQTT_STATE_CONNECTING
-- 2: MQTT_STATE_CONNECTED
-- 3: MQTT_STATE_READY
-- 4: MQTT_STATE_ERROR*/
static int l_mqtt_state(lua_State *L) {
	luat_mqtt_ctrl_t * mqtt_ctrl = get_mqtt_ctrl(L);
	lua_pushinteger(L, mqtt_ctrl->mqtt_state);
	return 1;
}

/*Set up a will message
@api mqttc:will(topic, payload, qos, retain)
@string topic of will message
@string payload of will message
@string qos of will message, default is 0, can be left blank
@string retain of will message, default 0, can be left blank
@return bool returns true if successful, otherwise returns false
@usage
-- To be called before connect
mqttc:will("/xxx/xxx", "xxxxxx")*/
static int l_mqtt_will(lua_State *L) {
	luat_mqtt_ctrl_t * mqtt_ctrl = get_mqtt_ctrl(L);
	size_t payload_len = 0;
	const char* topic = luaL_checkstring(L, 2);
	const char* payload = luaL_checklstring(L, 3, &payload_len);
	int qos = luaL_optinteger(L, 4, 0);
	int retain = luaL_optinteger(L, 5, 0);
	lua_pushboolean(L, luat_mqtt_set_will(mqtt_ctrl, topic, payload, payload_len, qos, retain) == 0 ? 1 : 0);
	return 1;
}

static int _mqtt_struct_newindex(lua_State *L);

void luat_mqtt_struct_init(lua_State *L) {
    luaL_newmetatable(L, LUAT_MQTT_CTRL_TYPE);
    lua_pushcfunction(L, _mqtt_struct_newindex);
    lua_setfield( L, -2, "__index" );
    lua_pop(L, 1);
}

#include "rotable2.h"
static const rotable_Reg_t reg_mqtt[] =
{
	{"create",			ROREG_FUNC(l_mqtt_create)},
	{"auth",			ROREG_FUNC(l_mqtt_auth)},
	{"keepalive",		ROREG_FUNC(l_mqtt_keepalive)},
	{"on",				ROREG_FUNC(l_mqtt_on)},
	{"connect",			ROREG_FUNC(l_mqtt_connect)},
	{"autoreconn",		ROREG_FUNC(l_mqtt_autoreconn)},
	{"publish",			ROREG_FUNC(l_mqtt_publish)},
	{"subscribe",		ROREG_FUNC(l_mqtt_subscribe)},
	{"unsubscribe",		ROREG_FUNC(l_mqtt_unsubscribe)},
	{"disconnect",		ROREG_FUNC(l_mqtt_disconnect)},
	{"close",			ROREG_FUNC(l_mqtt_close)},
	{"ready",			ROREG_FUNC(l_mqtt_ready)},
	{"will",			ROREG_FUNC(l_mqtt_will)},
	{"debug",			ROREG_FUNC(l_mqtt_set_debug)},
	{"state",			ROREG_FUNC(l_mqtt_state)},

    //@const STATE_DISCONNECT number mqtt disconnect
    {"STATE_DISCONNECT",ROREG_INT(MQTT_STATE_DISCONNECT)},
	//@const STATE_SCONNECT number mqtt socket connecting
	{"STATE_SCONNECT",	ROREG_INT(MQTT_STATE_SCONNECT)},
	//@const STATE_MQTT number mqtt socket is connected mqtt is connecting
	{"STATE_MQTT",  	ROREG_INT(MQTT_STATE_MQTT)},
	//@const STATE_READY number mqtt mqtt is connected
	{"STATE_READY",  	ROREG_INT(MQTT_STATE_READY)},
	{ NULL,             ROREG_INT(0)}
};

static int _mqtt_struct_newindex(lua_State *L) {
	const rotable_Reg_t* reg = reg_mqtt;
    const char* key = luaL_checkstring(L, 2);
	while (1) {
		if (reg->name == NULL)
			return 0;
		if (!strcmp(reg->name, key)) {
			lua_pushcfunction(L, reg->value.value.func);
			return 1;
		}
		reg ++;
	}
    //return 0;
}
const rotable_Reg_t reg_mqtt_emtry[] =
{
	{ NULL,             ROREG_INT(0)}
};

LUAMOD_API int luaopen_mqtt( lua_State *L ) {

#ifdef LUAT_USE_NETWORK
    luat_newlib2(L, reg_mqtt);
	luat_mqtt_struct_init(L);
    return 1;
#else
	luat_newlib2(L, reg_mqtt_emtry);
    return 1;
	LLOGE("mqtt require network enable!!");
#endif
}
