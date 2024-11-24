/*@Modules ercoap
@summary New Coap protocol parsing library
@version 1.0
@date 2023.11.14
@autherwendal
@demoercoap*/
#include "luat_base.h"
#include "luat_timer.h"
#include "luat_mem.h"
#include "luat_msgbus.h"
#include "luat_mcu.h"
#include "er-coap-13.h"

#define LUAT_LOG_TAG "ercoap"
#include "luat_log.h"


/*Parse coap packets
@api ercoap.parse(data)
@string coap packet
@return table returns table successfully, otherwise returns nil
@usage
-- This function is to parse the coap data packet
local rcoap = ercoap.parse(data)
if rcoap then
    log.info("coap", rcoap.type, rcoap.code, rcoap.payload)
    -- rcoap attributes
    -- type message type, 0 - CON requires reply, 1 - NON no reply is required, 2 - ACK received, 3 - RST error
    -- msgid message id
    --Data carried by payload
    -- code is similar to the status code of http, with 2xx for normal and 4xx for errors.
else
    log.info("ercoap", "Packet parsing failed")
end*/
static int l_ercoap_parse(lua_State *L)
{
    size_t len = 0;
    coap_packet_t coap_packet[1]  = { 0 };
    coap_status_t coap_error_code = COAP_STATUS_NO_ERROR;
    const char* data = luaL_checklstring(L, 1, &len);
    int ret = coap_parse_message(coap_packet, data, len);
    if (ret) {
        LLOGD("coap_parse_message %d", ret);
        return 0;
    }
    lua_newtable(L);
    lua_pushinteger(L, coap_packet->type);
    lua_setfield(L, -2, "type");
    lua_pushlstring(L, coap_packet->token, coap_packet->token_len);
    lua_setfield(L, -2, "token");
    lua_pushinteger(L, (coap_packet->code >> 5) * 100 + (coap_packet->code & 0x1F));
    lua_setfield(L, -2, "code");
    lua_pushinteger(L, coap_packet->mid);
    lua_setfield(L, -2, "msgid");
    lua_pushlstring(L, coap_packet->payload, coap_packet->payload_len);
    lua_setfield(L, -2, "payload");
    return 1;
}

/*Print coap packet
@api ercoap.print(data)
@string coap packet
@return boolean Returns true if parsing is successful
@usage
-- This function simply prints the coap data packet*/
static int l_ercoap_print(lua_State *L)
{
    size_t len = 0;
    coap_packet_t coap_packet[1]  = { 0 };
    coap_status_t coap_error_code = COAP_STATUS_NO_ERROR;
    const char* data = luaL_checklstring(L, 1, &len);
    int ret = coap_parse_message(coap_packet, data, len);
    if (ret) {
        LLOGD("coap_parse_message %d", ret);
        return 0;
    }
    LLOGD(
            "Parsed: type %u, tkl %u, token %02x%02x%02x%02x%02x%02x%02x%02x ,code %u.%.2u, mid %u",
            coap_packet->type,
            coap_packet->token_len,
            coap_packet->token[0],
            coap_packet->token[1],
            coap_packet->token[2],
            coap_packet->token[3],
            coap_packet->token[4],
            coap_packet->token[5],
            coap_packet->token[6],
            coap_packet->token[7],
            coap_packet->code >> 5,
            coap_packet->code & 0x1F,
            coap_packet->mid);
    if (coap_packet->payload_len > 0) {
        //LLOGD("!Payload! %.*s", coap_packet->payload_len, coap_packet->payload);
    }
    lua_pushboolean(L, 1);
    return 1;
}


//--------------------------------------------------
//--------- Data encapsulation for OneNet
//--------------------------------------------------
static uint16_t onenet_mid = 1;

static int32_t uri_add_path_tm_prefix(coap_packet_t* pkt, const char* suffix, const uint8_t* product_id, const uint8_t* dev_name)
{
    //$sys/{pid}/{device-name}/
    uint16_t header_uri_temp_length
        = 8 + strlen((const uint8_t*)suffix) + strlen((const uint8_t*)product_id) + strlen((const uint8_t*)dev_name);    // 7+1
    uint8_t* header_uri_temp = luat_heap_calloc(1, header_uri_temp_length);
    if (header_uri_temp == NULL) {
        return -1;
    }

    sprintf_(header_uri_temp, (const uint8_t*)"$sys/%s/%s/%s", product_id, dev_name, suffix);
    // LLOGD("URI %s", header_uri_temp);
    coap_set_header_uri_path(pkt, (const char*)header_uri_temp);
    luat_heap_free(header_uri_temp);
    return 0;
}

static int32_t payload_add_lifetime_and_saastoken(coap_packet_t* pkt, uint32_t lifetime, const uint8_t* saastoken)
{
    uint8_t* payload_temp        = NULL;
    uint16_t payload_temp_length = 4;    //{,}

    payload_temp_length += (7 + strlen(saastoken));    //"st":""

    if (lifetime != 0) {
        payload_temp_length += 14;    // 4+10,10 is 2 raised to the 32nd power "lt":
    }

    if (NULL == (payload_temp = luat_heap_calloc(1, payload_temp_length))) {
        return -1;
    }

    sprintf_(payload_temp, (const uint8_t*)"{\"st\":\"%s\"", saastoken);

    if (lifetime != 0) {
        sprintf_(payload_temp + strlen((const uint8_t*)payload_temp), (const uint8_t*)",\"lt\":%d", lifetime);
    }
    strcat(payload_temp, (const uint8_t*)"}");

    coap_set_header_content_type(pkt, APPLICATION_JSON);
    coap_set_payload(pkt, (const void*)payload_temp, strlen(payload_temp));
    return 0;
}

static void add_random_token(coap_packet_t* pkt, uint16_t mid)
{
    // generate a token
    uint8_t  temp_token[8];
    uint64_t tv_sec = luat_mcu_tick64();

    temp_token[0] = (uint8_t)(mid | (tv_sec >> 2));
    temp_token[1] = (uint8_t)(mid | (tv_sec >> 4));
    temp_token[2] = (uint8_t)(tv_sec);
    temp_token[3] = (uint8_t)(tv_sec >> 6);
    temp_token[4] = (uint8_t)(tv_sec >> 8);
    temp_token[5] = (uint8_t)(tv_sec >> 10);
    temp_token[6] = (uint8_t)(tv_sec >> 12);
    temp_token[7] = (uint8_t)(tv_sec >> 14);

    coap_set_header_token(pkt, temp_token, 8);
}

/*Quickly generate onenet data packets
@api ercoap.onenet(tp, product_id, device_name, token, payload)
@string request type, optional when used as a reply, required in other cases
@string project id, must be filled in
@string device name, must be filled in
@string token, must be filled in
@string object model json string, optional
@return string The synthesized data packet can be uploaded through UDP
@usage
-- Reference document: coap access https://open.iot.10086.cn/doc/v5/fuse/detail/924
-- Reference document: Object model https://open.iot.10086.cn/doc/v5/fuse/detail/902

--Type tp value token source payload
-- Login login iotauth.onenet function generated None
-- Heartbeat keep_live iotauth.onenet function generation None
-- Logout logout generated by iotauth.onenet function None
-- Obtained when property reporting thing/property/post login is required.
-- Property reply thing/property/reply Obtained when logging in. Must be present
--Event reporting thing/event/post is obtained when logging in. Must be present
-- Remote call reply. Obtained when not logged in. Required.*/
static int l_ercoap_onenet(lua_State *L)
{
    int32_t  ret = 0;
    uint16_t mid = 0;
    size_t len = 0;
    size_t payload_len = 0;
    size_t token_len = 0;
    coap_packet_t message[1] = { 0 };
    const char* type = luaL_checklstring(L, 1, &len);
    const char* product_id = luaL_checklstring(L, 2, &len);
    const char* dev_name = luaL_checklstring(L, 3, &len);
    const char* saastoken = luaL_checklstring(L, 4, &token_len);
    const char* payload = luaL_optlstring(L, 5, "", &payload_len);
    int lifetime = 3600;

    mid = onenet_mid ++;

    coap_init_message(message, COAP_TYPE_CON, COAP_POST, mid);
    // LLOGD("coap %s %s %s %s", type, product_id, dev_name, saastoken);
    if (type != NULL && strlen(type) > 0) {
        ret = uri_add_path_tm_prefix(message, type, product_id, dev_name);
        if (ret != 0) {
            LLOGD("uri_add_path_tm_prefix %d", ret);
            goto exit1;
        }
    }

    if (payload_len > 0){
        // LLOGD("Custom payload %s", payload);
        coap_set_header_accept(message, APPLICATION_JSON);
        coap_set_header_content_type(message, APPLICATION_JSON);
        coap_set_payload(message, (const void*)payload, payload_len);
        coap_set_header_token(message, saastoken, token_len);
    }
    else {
        ret = payload_add_lifetime_and_saastoken(message, lifetime, saastoken);
        add_random_token(message, mid);
        if (ret != 0) {
            LLOGD("payload_add_lifetime_and_saastoken %d", ret);
            goto exit2;
        }
    }
    //LLOGD("uplink payload %s", message->payload);
    ret = coap_serialize_get_size(message);
    if (ret < 1) {
        LLOGD("coap_serialize_get_size %d", ret);
        goto exit2;
    }
    char* ptr = luat_heap_malloc(ret);
    if (ptr == NULL) {
        LLOGD("out of memory when malloc message buff");
        goto exit2;
    }
    ret = coap_serialize_message(message, ptr);
    lua_pushlstring(L, ptr, ret);
    luat_heap_free(ptr);
    coap_free_header(message);
    return 1;

exit2:
    coap_free_header(message);
exit1:
    return 0;
}

#include "rotable2.h"
static const rotable_Reg_t reg_ercoap[] =
{
    { "onenet", ROREG_FUNC(l_ercoap_onenet)},
    { "parse",  ROREG_FUNC(l_ercoap_parse)},
    { "print",  ROREG_FUNC(l_ercoap_print)},
	{ NULL,     ROREG_INT(0) }
};

LUAMOD_API int luaopen_ercoap( lua_State *L ) {
    luat_newlib2(L, reg_ercoap);
    return 1;
}
