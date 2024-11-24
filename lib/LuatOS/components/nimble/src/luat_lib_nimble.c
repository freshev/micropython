/*@Modules nimble
@summary Bluetooth BLE library (nimble version)
@version 1.0
@date 2022.10.21
@demo nimble
@tag LUAT_USE_NIMBLE
@usage
-- This library currently supports Air101/Air103/ESP32/ESP32C3/ESP32S3
-- Please refer to the demo for usage, the API function will fall into the specified mode

-- Name explanation:
-- peripheral peripheral mode, or slave mode, is the connected device
-- central central mode, or host mode, scans and connects to other devices
-- ibeacon periodic beacon broadcast

-- UUID The service and characteristics of the device will be identified by UUID. It supports 2 bytes/4 bytes/16 bytes, and usually uses a shortened version of 2 bytes.
-- The chr device service consists of multiple characteristics, referred to as chr.
-- The characteristic feature consists of UUID and flags, where UUID is used as an identifier and flags represents the functions that the feature can support.*/

#include "luat_base.h"
#include "luat_msgbus.h"
#include "luat_mem.h"
#include "luat_spi.h"


#include "host/ble_gatt.h"
#include "host/ble_hs_id.h"
#include "host/util/util.h"
#include "host/ble_hs_adv.h"
#include "host/ble_gap.h"

#include "luat_nimble.h"

#define LUAT_LOG_TAG "nimble"
#include "luat_log.h"

#define CFG_ADDR_ORDER 1

static uint32_t nimble_mode = 0;
uint16_t g_ble_state;
uint16_t g_ble_conn_handle;

// peripheral, scanned, UUID configuration of the connected device
ble_uuid_any_t ble_peripheral_srv_uuid;
uint16_t s_chr_flags[LUAT_BLE_MAX_CHR];
uint16_t s_chr_val_handles[LUAT_BLE_MAX_CHR];
ble_uuid_any_t s_chr_uuids[LUAT_BLE_MAX_CHR];
uint8_t s_chr_notify_states[LUAT_BLE_MAX_CHR];
uint8_t s_chr_indicate_states[LUAT_BLE_MAX_CHR];

#define WM_GATT_SVC_UUID      0x180D
// #define WM_GATT_SVC_UUID      0xFFF0
#define WM_GATT_INDICATE_UUID 0xFFF1
#define WM_GATT_WRITE_UUID    0xFFF2
#define WM_GATT_NOTIFY_UUID    0xFFF3


uint8_t luat_ble_dev_name[32];
size_t  luat_ble_dev_name_len;

uint8_t adv_buff[128];
int adv_buff_len = 0;
struct ble_hs_adv_fields adv_fields;
struct ble_gap_adv_params adv_params = {0};

static uint8_t ble_uuid_addr_conv = 0; // BLE addresses need to be reversed, which is a pain in the ass

struct ble_gatt_svc *peer_servs[MAX_PER_SERV];
struct ble_gatt_chr *peer_chrs[MAX_PER_SERV*MAX_PER_SERV];

static int buff2uuid(ble_uuid_any_t* uuid, const char* data, size_t data_len) {
    if (data_len > 16)
        return -1;
    char tmp[16];
    for (size_t i = 0; i < data_len; i++)
    {
        if (ble_uuid_addr_conv == 0)
            tmp[i] = data[i];
        else
            tmp[i] = data[data_len - i - 1];
    }
    return ble_uuid_init_from_buf(uuid, tmp, data_len);
}


// Universal API, suitable for all modes
//--------------------------------------------------

/*Initialize the BLE context and start external broadcast/scanning
@api nimble.init(name)
@string Bluetooth device name, optional, recommended to fill in
@return bool success or failure
@usage
-- Refer to demo/nimble
-- This function is applicable to all modes*/
static int l_nimble_init(lua_State* L) {
    int rc = 0;
    size_t len = 0;
    const char* name = NULL;

    lua_gc(L, LUA_GCCOLLECT, 0);
    lua_gc(L, LUA_GCCOLLECT, 0);

    if(lua_isstring(L, 1)) {
        name = luaL_checklstring(L, 1, &len);
        if (len > 0) {
            memcpy(luat_ble_dev_name, name, len);
            luat_ble_dev_name_len = len;
        }
    }
    LLOGD("init name %s mode %d", name == NULL ? "-" : name, nimble_mode);
    rc = luat_nimble_init(0xFF, name, nimble_mode);
    if (rc) {
        lua_pushboolean(L, 0);
        lua_pushinteger(L, rc);
        return 2;
    }
    else {
        lua_pushboolean(L, 1);
        return 1;
    }
}

/*Close BLE context
@api nimble.deinit()
@return bool success or failure
@usage
--Only supported by some devices, and may not be supported currently.
-- This function is applicable to all modes*/
static int l_nimble_deinit(lua_State* L) {
    int rc = 0;
    rc = luat_nimble_deinit();
    if (rc) {
        lua_pushboolean(L, 0);
        lua_pushinteger(L, rc);
        return 2;
    }
    else {
        lua_pushboolean(L, 1);
        return 1;
    }
}

static int l_nimble_debug(lua_State* L) {
    LLOGI("nimble.debug is removed");
    lua_pushinteger(L, 0);
    return 1;
}

static int l_nimble_server_init(lua_State* L) {
    LLOGI("nimble.server_init is removed");
    return 0;
}

static int l_nimble_server_deinit(lua_State* L) {
    LLOGI("nimble.server_deinit is removed");
    return 0;
}

/*Setup mode
@api nimble.mode(tp)
@int mode, default server/peripheral, optional client/central mode nimble.MODE_BLE_CLIENT
@return bool success or failure
@usage
-- Refer to demo/nimble
-- Must be called before nimble.init()
-- nimble.mode(nimble.MODE_BLE_CLIENT) -- referred to as slave mode, not perfect*/
static int l_nimble_mode(lua_State *L) {
    if (lua_isinteger(L, 1)) {
        nimble_mode = lua_tointeger(L, 1);
    }
    lua_pushinteger(L, nimble_mode);
    return 1;
}

/*Has the connection been established?
@api nimble.connok()
@return bool Returns true if connected, otherwise returns false
@usage
log.info("ble", "connected?", nimble.connok())
-- Slave peripheral mode, whether the device has been connected
--Host central mode, whether it is connected to the device
--ibeacon mode, meaningless*/
static int l_nimble_connok(lua_State *L) {
    lua_pushboolean(L, g_ble_state == BT_STATE_CONNECTED ? 1 : 0);
    return 1;
}


//--------------------------------------------------
// Slave peripheral mode series API

/*Send message
@api nimble.send_msg(conn, handle, data)
@int connection id, currently fixed to 1
@int handles id, currently filled with 0
@string data string, may contain invisible characters
@return bool success or failure
@usage
-- Refer to demo/nimble
-- This function is applicable to peripheral/slave mode*/
static int l_nimble_send_msg(lua_State *L) {
    int conn_id = luaL_checkinteger(L, 1);
    int handle_id = luaL_checkinteger(L, 2);
    size_t len = 0;
    const char* data = luaL_checklstring(L, 3, &len);
    int ret = 0;
    if (len == 0) {
        LLOGI("send emtry msg? ignored");
    }
    else {
        ret = luat_nimble_server_send(0, data, len);
    }

    lua_pushboolean(L, ret == 0 ? 1 : 0);
    // lua_pushinteger(L, ret);
    return 1;
}


/*Set the UUID of server/peripheral
@api nimble.setUUID(tp, addr)
@string configuration string, as explained in the following examples
@string address string
@return bool success or failure
@usage
-- Refer to demo/nimble, firmware compiled after 2023-02-25 supports this API
-- Must be called before nimble.init()
-- This function is applicable to peripheral/slave mode

--Set UUID in SERVER/Peripheral mode, support setting 3
--Address supports 2/4/16 bytes, requires binary data
-- 2-byte address example: AABB, write string.fromHex("AABB") , or string.char(0xAA, 0xBB)
-- 4-byte address example: AABBCCDD, write string.fromHex("AABBCCDD"), or string.char(0xAA, 0xBB, 0xCC, 0xDD)
nimble.setUUID("srv", string.fromHex("380D")) -- Service master UUID, default value 180D
nimble.setUUID("write", string.fromHex("FF31")) -- UUID for writing data to this device, default value FFF1
nimble.setUUID("indicate", string.fromHex("FF32")) -- UUID for subscribing to the data of this device, default value FFF2*/
static int l_nimble_set_uuid(lua_State *L) {
    size_t len = 0;
    ble_uuid_any_t tmp = {0};
    const char* key = luaL_checkstring(L, 1);
    const char* uuid = luaL_checklstring(L, 2, &len);
    int ret = buff2uuid(&tmp, (const void*)uuid, len);
    if (ret != 0) {
        LLOGW("invaild UUID, len must be 2/4/16");
        return 0;
    }
    if (!strcmp("srv", key)) {
        memcpy(&ble_peripheral_srv_uuid, &tmp, sizeof(ble_uuid_any_t));
    }
    else if (!strcmp("write", key)) {
        memcpy(&s_chr_uuids[0], &tmp, sizeof(ble_uuid_any_t));
    }
    else if (!strcmp("indicate", key)) {
        memcpy(&s_chr_uuids[1], &tmp, sizeof(ble_uuid_any_t));
    }
    else if (!strcmp("notify", key)) {
        memcpy(&s_chr_uuids[2], &tmp, sizeof(ble_uuid_any_t));
    }
    else {
        LLOGW("only support srv/write/indicate/notify");
        return 0;
    }
    lua_pushboolean(L, 1);
    return 1;
}

/*Get Bluetooth MAC
@api nimble.mac(mac)
@string MAC address to be set, 6 bytes, if not passed, just get it
@return string Bluetooth MAC address, 6 bytes
@usage
-- Refer to demo/nimble, firmware compiled after 2023-02-25 supports this API
-- This function is applicable to all modes
local mac = nimble.mac()
log.info("ble", "mac", mac and mac:toHex() or "Unknwn")

-- Modify MAC address, new in 2024.06.05, currently only supported by Air601, it will take effect after restarting
nimble.mac(string.fromHex("1234567890AB"))*/
static int l_nimble_mac(lua_State *L) {
    int rc = 0;
    uint8_t own_addr_type = 0;
    uint8_t addr_val[6] = {0};
    if (lua_type(L, 1) == LUA_TSTRING) {
        size_t len = 0;
        const char* tmac = luaL_checklstring(L, 1, &len);
        if (len != 6) {
            LLOGW("mac len must be 6");
            return 0;
        }
        luat_nimble_mac_set(tmac);
    }
    #ifdef TLS_CONFIG_CPU_XT804
    if (1) {
        extern int luat_nimble_mac_get(uint8_t* mac);
        luat_nimble_mac_get(addr_val);
        lua_pushlstring(L, (const char*)addr_val, 6);
        return 1;
    }
    #endif
    rc = ble_hs_util_ensure_addr(0);
    if (rc != 0) {
        LLOGW("fail to fetch BLE MAC, rc %d", rc);
        return 0;
    }

    /* Figure out address to use while advertising (no privacy for now) */
    rc = ble_hs_id_infer_auto(0, &own_addr_type);
    if (rc != 0) {
        LLOGE("error determining address type; rc=%d", rc);
        return 0;
    }

    /* Printing ADDR */
    
    rc = ble_hs_id_copy_addr(own_addr_type, addr_val, NULL);
    if (rc == 0) {
        lua_pushlstring(L, (const char*)addr_val, 6);
        return 1;
    }
    LLOGW("fail to fetch BLE MAC, rc %d", rc);
    return 0;
}


/*Send notify
@api nimble.sendNotify(srv_uuid, chr_uuid, data)
@string UUID of the service, reserved, just fill in nil currently
@string UUID of the feature, must be filled in
@string data, required, related to MTU size, generally should not exceed 256 bytes
@return bool returns true if successful, otherwise returns false
@usage
--This API was added on 2023.07.31
-- This function is applicable to peripheral mode
nimble.sendNotify(nil, string.fromHex("FF01"), string.char(0x31, 0x32, 0x33, 0x34, 0x35))*/
static int l_nimble_send_notify(lua_State *L) {
    size_t tmp_size = 0;
    size_t data_len = 0;
    ble_uuid_any_t chr_uuid;
    const char* tmp = luaL_checklstring(L, 2, &tmp_size);
    int ret = buff2uuid(&chr_uuid, tmp, tmp_size);
    if (ret) {
        LLOGE("ble_uuid_init_from_buf rc %d", ret);
        return 0;
    }
    const char* data = luaL_checklstring(L, 3, &data_len);
    ret = luat_nimble_server_send_notify(NULL, &chr_uuid, data, data_len);
    lua_pushboolean(L, ret == 0 ? 1 : 0);
    return 1;
}

/*Send indicate
@api nimble.sendIndicate(srv_uuid, chr_uuid, data)
@string UUID of the service, reserved, just fill in nil currently
@string UUID of the feature, must be filled in
@string data, required, related to MTU size, generally should not exceed 256 bytes
@return bool returns true if successful, otherwise returns false
@usage
--This API was added on 2023.07.31
-- This function is applicable to peripheral mode
nimble.sendIndicate(nil, string.fromHex("FF01"), string.char(0x31, 0x32, 0x33, 0x34, 0x35))*/
static int l_nimble_send_indicate(lua_State *L) {
    size_t tmp_size = 0;
    size_t data_len = 0;
    ble_uuid_any_t chr_uuid;
    const char* tmp = luaL_checklstring(L, 2, &tmp_size);
    int ret = buff2uuid(&chr_uuid, tmp, tmp_size);
    if (ret) {
        LLOGE("ble_uuid_init_from_buf rc %d", ret);
        return 0;
    }
    const char* data = luaL_checklstring(L, 3, &data_len);
    ret = luat_nimble_server_send_indicate(NULL, &chr_uuid, data, data_len);
    lua_pushboolean(L, ret == 0 ? 1 : 0);
    return 1;
}


/*Set broadcast parameters
@api nimble.advParams(conn_mode, disc_mode, itvl_min, itvl_max, channel_map, filter_policy, high_duty_cycle)
@int broadcast mode, 0 - not connectable, 1 - directed connection, 2 - undirected connection, default 0
@int Discovery mode, 0 - not discoverable, 1 - limited discovery, 3 - universal discovery, default 0
@int Minimum broadcast interval, 0 - use default value, range 1 - 65535, unit 0.625ms, default 0
@int Maximum broadcast interval, 0 - use default value, range 1 - 65535, unit 0.625ms, default 0
@int Broadcast channel, default 0, generally no need to set
@int filtering rule, default 0, generally no need to set
@int When the broadcast mode is "directed connection", whether to use high duty cycle mode, default 0, optional 1
@return nil no return value
@usage
-- Currently only ibeacon mode/peripheral/slave can be used
-- For example, set unconnectable + limit discovery
-- Need to be set before nimble.init
nimble.advParams(0, 1)
-- Note that conn_mode and disc_mode are automatically configured in peripheral mode*/
static int l_nimble_set_adv_params(lua_State *L) {
    /** Advertising mode. Can be one of following constants:
     *  - BLE_GAP_CONN_MODE_NON (non-connectable; 3.C.9.3.2).
     *  - BLE_GAP_CONN_MODE_DIR (directed-connectable; 3.C.9.3.3).
     *  - BLE_GAP_CONN_MODE_UND (undirected-connectable; 3.C.9.3.4).
     */
    adv_params.conn_mode = luaL_optinteger(L, 1, 0);
    /** Discoverable mode. Can be one of following constants:
     *  - BLE_GAP_DISC_MODE_NON  (non-discoverable; 3.C.9.2.2).
     *  - BLE_GAP_DISC_MODE_LTD (limited-discoverable; 3.C.9.2.3).
     *  - BLE_GAP_DISC_MODE_GEN (general-discoverable; 3.C.9.2.4).
     */
    adv_params.disc_mode = luaL_optinteger(L, 2, 0);

    /** Minimum advertising interval, if 0 stack use sane defaults */
    adv_params.itvl_min = luaL_optinteger(L, 3, 0);
    /** Maximum advertising interval, if 0 stack use sane defaults */
    adv_params.itvl_max = luaL_optinteger(L, 4, 0);
    /** Advertising channel map , if 0 stack use sane defaults */
    adv_params.channel_map = luaL_optinteger(L, 5, 0);

    /** Advertising  Filter policy */
    adv_params.filter_policy = luaL_optinteger(L, 6, 0);

    /** If do High Duty cycle for Directed Advertising */
    adv_params.high_duty_cycle = luaL_optinteger(L, 7, 0);

    return 0;
}

/*Set the characteristics of chr
@api nimble.setChr(index, uuid, flags)
Index of @int chr, default 0-3
@int chr UUID, can be 2/4/16 bytes
FLAGS of @int chr, please check the constant table
@return nil no return value
@usage
-- Only peripheral/slave can be used
nimble.setChr(0, string.fromHex("FF01"), nimble.CHR_F_WRITE_NO_RSP | nimble.CHR_F_NOTIFY)
nimble.setChr(1, string.fromHex("FF02"), nimble.CHR_F_READ | nimble.CHR_F_NOTIFY)
nimble.setChr(2, string.fromHex("FF03"), nimble.CHR_F_WRITE_NO_RSP)
-- See demo/nimble/kt6368a*/
static int l_nimble_set_chr(lua_State *L) {
    size_t tmp_size = 0;
    // ble_uuid_any_t srv_uuid = {0};
    ble_uuid_any_t chr_uuid = {0};
    const char* tmp;
    int ret = 0;
    int index = luaL_checkinteger(L, 1);
    tmp = luaL_checklstring(L, 2, &tmp_size);
    // LLOGD("chr? %02X%02X %d", tmp[0], tmp[1], tmp_size);
    ret = buff2uuid(&chr_uuid, tmp, tmp_size);
    if (ret) {
        LLOGE("ble_uuid_init_from_buf rc %d", ret);
        return 0;
    }
    int flags = luaL_checkinteger(L, 3);

    luat_nimble_peripheral_set_chr(index, &chr_uuid, flags);
    return 0;
}

/*Set the characteristics of chr
@api nimble.config(id, value)
@int configured id, please refer to the constant table
@any has different optional values   depending on the configuration.
@return nil no return value
@usage
-- This function is available in any mode
--This API was added on 2023.07.31
-- For example, set the endianness of address translation. The default is 0, which is compatible with old code.
-- Set to 1, the service UUID and chr UUID are more intuitive
nimble.config(nimble.CFG_ADDR_ORDER, 1)*/
static int l_nimble_config(lua_State *L) {
    int conf = luaL_checkinteger(L, 1);
    if (conf == CFG_ADDR_ORDER) {
        if (lua_isboolean(L, 2))
            ble_uuid_addr_conv = lua_toboolean(L, 2);
        else if (lua_isinteger(L, 2))
            ble_uuid_addr_conv = lua_tointeger(L, 2);
    }
    return 0;
}

//-------------------------------------
// ibeacon series API


/*Configure iBeacon parameters, only iBeacon mode is available
@api nimble.ibeacon(data, major, minor, measured_power)
@string data, must be 16 bytes
@int Major version number, default 2, optional, range 0 ~ 65536
@int Minor version number, default 10, optional, range 0 ~ 65536
@int Nominal power, default 0, range -126 to 20
@return bool returns true if successful, otherwise returns false
@usage
-- Refer to demo/nimble, firmware compiled after 2023-02-25 supports this API
-- This function is suitable for ibeacon mode
nimble.ibeacon(data, 2, 10, 0)
nimble.init()*/
static int l_nimble_ibeacon(lua_State *L) {
    size_t len = 0;
    const char* data = luaL_checklstring(L, 1, &len);
    if (len != 16) {
        LLOGE("ibeacon data MUST 16 bytes, but %d", len);
        return 0;
    }
    uint16_t major = luaL_optinteger(L, 2, 2);
    uint16_t minor = luaL_optinteger(L, 3, 10);
    int8_t measured_power = luaL_optinteger(L, 4, 0);

    int rc = luat_nimble_ibeacon_setup(data, major, minor, measured_power);
    lua_pushboolean(L, rc == 0 ? 1 : 0);
    return 1;
}

/*Configure broadcast data, only iBeacon mode is available
@api nimble.advData(data, flags)
@string broadcast data, currently up to 128 bytes
@int broadcast identifier, optional, the default value is 0x06, that is, traditional Bluetooth (0x04) + normal discovery mode (0x02) is not supported
@return bool returns true if successful, otherwise returns false
@usage
-- Refer to demo/nimble/adv_free, firmware compiled after 2023-03-18 supports this API
-- This function is suitable for ibeacon mode
-- Data sources can be diverse
local data = string.fromHex("123487651234876512348765123487651234876512348765")
-- local data = crypto.trng(25)
-- local data = string.char(0x11, 0x13, 0xA3, 0x5A, 0x11, 0x13, 0xA3, 0x5A, 0x11, 0x13, 0xA3, 0x5A, 0x11, 0x13, 0xA3, 0x5A)
nimble.advData(data)
nimble.init()

-- nimble supports calling again at any time after init to update data
-- For example, change once every minute
while 1 do
    sys.wait(60000)
    local data = crypto.trng(25)
    nimble.advData(data)
end*/
static int l_nimble_set_adv_data(lua_State *L) {
    size_t len = 0;
    const char* data = luaL_checklstring(L, 1, &len);
    int flags = luaL_optinteger(L, 2, BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP);
    int rc = luat_nimble_set_adv_data(data, len, flags);
    lua_pushboolean(L, rc == 0 ? 1 : 0);
    return 1;
}

//-----------------------------------------------------
//Host central mode API

/*Scan slave
@api nimble.scan(timeout)
@int timeout time, unit seconds, default 28 seconds
@return bool Whether the scan was started successfully or not
@usage
-- Refer to demo/nimble/scan
-- This function is applicable to central/host mode
-- This function will return directly, and then return the result through an asynchronous callback

-- Before calling this function, you need to ensure that nimble.init() has been executed
nimble.scan()
-- The timeout parameter was added on 2023.7.11*/
static int l_nimble_scan(lua_State *L) {
    int timeout = luaL_optinteger(L, 1, 28);
    if (timeout < 1)
        timeout = 1;
    int ret = luat_nimble_blecent_scan(timeout);
    lua_pushboolean(L, ret == 0 ? 1 : 0);
    // lua_pushinteger(L, ret);
    return 1;
}

/*Connect to slave
@api nimble.connect(mac)
@string MAC address of the device
@return bool Whether the connection is started successfully or not
@usage
-- This function is applicable to central/host mode
-- This function will return directly, and then return the result through an asynchronous callback*/
static int l_nimble_connect(lua_State *L) {
    size_t len = 0;
    const char* addr = luaL_checklstring(L, 1, &len);
    if (addr == NULL)
        return 0;
    luat_nimble_blecent_connect(addr);
    return 0;
}

/*Disconnect from slave
@api nimble.disconnect()
@return nil no return value
@usage
-- This function is applicable to central/host mode
-- This function will return directly*/
static int l_nimble_disconnect(lua_State *L) {
    int id = luaL_optinteger(L, 1, 0);
    luat_nimble_blecent_disconnect(id);
    return 0;
}

/*Scan the service list of the slave machine
@api nimble.discSvr()
@return nil no return value
@usage
-- This function is applicable to central/host mode
-- This function will return directly and then return the result asynchronously
-- This API usually does not need to be called. It will be called once after the connection to the slave is completed.*/
static int l_nimble_disc_svr(lua_State *L) {
    int id = luaL_optinteger(L, 1, 0);
    luat_nimble_central_disc_srv(id);
    return 0;
}

/*Get the service list of the slave machine
@api nimble.listSvr()
@return table array of service UUIDs
@usage
-- This function is applicable to central/host mode*/
static int l_nimble_list_svr(lua_State *L) {
    lua_newtable(L);
    char buff[64];
    size_t i;
    for (i = 0; i < MAX_PER_SERV; i++)
    {
        if (peer_servs[i] == NULL)
            break;
        lua_pushstring(L, ble_uuid_to_str(&peer_servs[i]->uuid, buff));
        lua_seti(L, -2, i+1);
    }
    return 1;
}

/*Scan the characteristic value of the specified service of the slave machine
@api nimble.discChr(svr_uuid)
@string specifies the UUID value of the service
@return boolean whether the scan was successfully started or not
@usage
-- This function is applicable to central/host mode*/
static int l_nimble_disc_chr(lua_State *L) {
    size_t tmp_size = 0;
    size_t data_len = 0;
    ble_uuid_any_t svr_uuid;
    const char* tmp = luaL_checklstring(L, 1, &tmp_size);
    int ret = buff2uuid(&svr_uuid, tmp, tmp_size);
    if (ret) {
        return 0;
    }
    size_t i;
    char buff[64];
    for (i = 0; i < MAX_PER_SERV; i++)
    {
        if (peer_servs[i] == NULL)
            break;
        if (0 == ble_uuid_cmp(&peer_servs[i]->uuid, &svr_uuid)) {
            // LLOGD("Find the matching UUID and query its characteristic value");
            lua_pushboolean(L, 1);
            luat_nimble_central_disc_chr(0, peer_servs[i]);
            return 1;
        }
        // LLOGD("Desired service id %s", ble_uuid_to_str(&svr_uuid, buff));
        // LLOGD("Actual service id %s", ble_uuid_to_str(&peer_servs[i]->uuid, buff));
    }
    return 0;
}

/*Get the characteristic value list of the specified service of the slave machine
@api nimble.listChr(svr_uuid)
@string specifies the UUID value of the service
@return table feature value list, including UUID and flags
@usage
-- This function is applicable to central/host mode*/
static int l_nimble_list_chr(lua_State *L) {
    size_t tmp_size = 0;
    size_t data_len = 0;
    ble_uuid_any_t svr_uuid;
    const char* tmp = luaL_checklstring(L, 1, &tmp_size);
    int ret = buff2uuid(&svr_uuid, tmp, tmp_size);
    if (ret) {
        return 0;
    }
    size_t i;
    char buff[64];
    lua_newtable(L);
    for (i = 0; i < MAX_PER_SERV; i++)
    {
        if (peer_servs[i] == NULL)
            continue;
        if (0 == ble_uuid_cmp(&peer_servs[i]->uuid, &svr_uuid)) {
            for (size_t j = 0; j < MAX_PER_SERV; j++)
            {
                if (peer_chrs[i*MAX_PER_SERV + j] == NULL)
                    break;
                lua_newtable(L);
                lua_pushstring(L, ble_uuid_to_str(&(peer_chrs[i*MAX_PER_SERV+j]->uuid), buff));
                lua_setfield(L, -2, "uuid");
                lua_pushinteger(L, peer_chrs[i*MAX_PER_SERV+j]->properties);
                lua_setfield(L, -2, "flags");

                lua_seti(L, -2, j + 1);
            }
            return 1;
        }
    }
    return 1;
}

static int find_chr(lua_State *L, struct ble_gatt_svc **svc, struct ble_gatt_chr **chr) {
    size_t tmp_size = 0;
    int32_t ret = 0;
    const char* tmp;
    ble_uuid_any_t svr_uuid;
    ble_uuid_any_t chr_uuid;
    // UUID of service
    tmp = luaL_checklstring(L, 1, &tmp_size);
    ret = buff2uuid(&svr_uuid, tmp, tmp_size);
    if (ret) {
        return -1;
    }
    // UUID of the characteristic
    tmp = luaL_checklstring(L, 2, &tmp_size);
    ret = buff2uuid(&chr_uuid, tmp, tmp_size);
    if (ret) {
        return -1;
    }
    for (size_t i = 0; i < MAX_PER_SERV; i++)
    {
        if (peer_servs[i] == NULL)
            continue;
        if (0 == ble_uuid_cmp(&peer_servs[i]->uuid, &svr_uuid)) {
            *svc = peer_servs[i];
            for (size_t j = 0; j < MAX_PER_SERV; j++)
            {
                if (peer_chrs[i*MAX_PER_SERV + j] == NULL)
                    break;
                if (0 == ble_uuid_cmp(&peer_chrs[i*MAX_PER_SERV + j]->uuid, &chr_uuid)) {
                    *chr = peer_chrs[i*MAX_PER_SERV + j];
                    return 0;
                }
            }
        }
    }
    return -1;
}

/*Scan other attributes of the slave's characteristic value for the specified service
@api nimble.discDsc(svr_uuid, chr_uuid)
@string specifies the UUID value of the service
@string UUID value of characteristic value
@return boolean whether the scan was successfully started or not
@usage
-- This function is applicable to central/host mode*/
static int l_nimble_disc_dsc(lua_State *L) {
    int ret;
    struct ble_gatt_svc *svc;
    struct ble_gatt_chr *chr;
    ret = find_chr(L, &svc, &chr);
    if (ret) {
        LLOGW("bad svr/chr UUID");
        return 0;
    }
    ret = luat_nimble_central_disc_dsc(0, svc, chr);
    if (ret == 0) {
        lua_pushboolean(L, 1);
        return 1;
    }
    return 0;
}


/*Write data to the specified characteristic value of the specified service
@api nimble.writeChr(svr_uuid, chr_uuid, data)
@string specifies the UUID value of the service
@string specifies the UUID value of the characteristic value
@string data to be written
@return boolean whether writing was successfully started or not
@usage
-- This function is applicable to central/host mode*/
static int l_nimble_write_chr(lua_State *L) {
    size_t tmp_size = 0;
    int32_t ret = 0;
    const char* tmp;
    struct ble_gatt_svc *svc;
    struct ble_gatt_chr *chr;
    ret = find_chr(L, &svc, &chr);
    if (ret) {
        LLOGW("bad svr/chr UUID");
        return 0;
    }
    // data
    tmp = luaL_checklstring(L, 3, &tmp_size);
    ret = luat_nimble_central_write(0, chr, tmp, tmp_size);
    if (ret == 0) {
        lua_pushboolean(L, 1);
        return 1;
    }
    return 0;
}

/*Read data from the specified characteristic value of the specified service (asynchronously)
@api nimble.readChr(svr_uuid, chr_uuid)
@string specifies the UUID value of the service
@string specifies the UUID value of the characteristic value
@return boolean whether writing was successfully started or not
@usage
-- This function is applicable to central/host mode
-- Please refer to the parameter demo/nimble/central for detailed usage.*/
static int l_nimble_read_chr(lua_State *L) {
    int32_t ret = 0;
    struct ble_gatt_svc *svc;
    struct ble_gatt_chr *chr;
    ret = find_chr(L, &svc, &chr);
    if (ret) {
        LLOGW("bad svr/chr UUID");
        return 0;
    }
    ret = luat_nimble_central_read(0, chr);
    if (ret == 0) {
        lua_pushboolean(L, 1);
        return 1;
    }
    return 0;
}

/*Subscribe to the specified characteristic value of the specified service
@api nimble.subChr(svr_uuid, chr_uuid)
@string specifies the UUID value of the service
@string specifies the UUID value of the characteristic value
@return boolean whether the subscription was successfully started or not
@usage
-- This function is applicable to central/host mode
-- Please refer to the parameter demo/nimble/central for detailed usage.*/
static int l_nimble_subscribe_chr(lua_State *L) {
    int32_t ret = 0;
    struct ble_gatt_svc *svc;
    struct ble_gatt_chr *chr;
    ret = find_chr(L, &svc, &chr);
    if (ret) {
        LLOGW("bad svr/chr UUID");
        return 0;
    }
    ret = luat_nimble_central_subscribe(0, chr, 1);
    if (ret == 0) {
        LLOGD("Subscription successful");
        lua_pushboolean(L, 1);
        return 1;
    }
    LLOGD("Subscription failed %d", ret);
    return 0;
}

/*Unsubscribe from the specified characteristic value of the specified service
@api nimble.unsubChr(svr_uuid, chr_uuid)
@string specifies the UUID value of the service
@string specifies the UUID value of the characteristic value
@return boolean whether the unsubscription was successfully initiated or not
@usage
-- This function is applicable to central/host mode
-- Please refer to the parameter demo/nimble/central for detailed usage.*/
static int l_nimble_unsubscribe_chr(lua_State *L) {
    int32_t ret = 0;
    struct ble_gatt_svc *svc;
    struct ble_gatt_chr *chr;
    ret = find_chr(L, &svc, &chr);
    if (ret) {
        LLOGW("bad svr/chr UUID");
        return 0;
    }
    ret = luat_nimble_central_subscribe(0, chr, 0);
    if (ret == 0) {
        lua_pushboolean(L, 1);
        return 1;
    }
    return 0;
}

#include "rotable2.h"
static const rotable_Reg_t reg_nimble[] =
{
	{ "init",           ROREG_FUNC(l_nimble_init)},
    { "deinit",         ROREG_FUNC(l_nimble_deinit)},
    { "debug",          ROREG_FUNC(l_nimble_debug)},
    { "mode",           ROREG_FUNC(l_nimble_mode)},
    { "connok",         ROREG_FUNC(l_nimble_connok)},
    { "config",         ROREG_FUNC(l_nimble_config)},

    // Peripheral mode, broadcast and wait for connection
    { "send_msg",       ROREG_FUNC(l_nimble_send_msg)},
    { "sendNotify",     ROREG_FUNC(l_nimble_send_notify)},
    { "sendIndicate",   ROREG_FUNC(l_nimble_send_indicate)},
    { "setUUID",        ROREG_FUNC(l_nimble_set_uuid)},
    { "setChr",         ROREG_FUNC(l_nimble_set_chr)},
    { "mac",            ROREG_FUNC(l_nimble_mac)},
    { "server_init",    ROREG_FUNC(l_nimble_server_init)},
    { "server_deinit",  ROREG_FUNC(l_nimble_server_deinit)},

    // Central mode, scan and connect peripherals
    { "scan",           ROREG_FUNC(l_nimble_scan)},
    { "connect",        ROREG_FUNC(l_nimble_connect)},
    { "disconnect",     ROREG_FUNC(l_nimble_disconnect)},
    { "discSvr",        ROREG_FUNC(l_nimble_disc_svr)},
    { "discChr",        ROREG_FUNC(l_nimble_disc_chr)},
    { "discDsc",        ROREG_FUNC(l_nimble_disc_dsc)},
    { "listSvr",        ROREG_FUNC(l_nimble_list_svr)},
    { "listChr",        ROREG_FUNC(l_nimble_list_chr)},
    { "readChr",        ROREG_FUNC(l_nimble_read_chr)},
    { "writeChr",       ROREG_FUNC(l_nimble_write_chr)},
    { "subChr",         ROREG_FUNC(l_nimble_subscribe_chr)},
    { "unsubChr",       ROREG_FUNC(l_nimble_unsubscribe_chr)},

    //ibeacon broadcast mode
    { "ibeacon",        ROREG_FUNC(l_nimble_ibeacon)},

    // Broadcast data
    { "advData",        ROREG_FUNC(l_nimble_set_adv_data)},
    { "advParams",        ROREG_FUNC(l_nimble_set_adv_params)},

    // put some constants
    { "STATE_OFF",           ROREG_INT(BT_STATE_OFF)},
    { "STATE_ON",            ROREG_INT(BT_STATE_ON)},
    { "STATE_CONNECTED",     ROREG_INT(BT_STATE_CONNECTED)},
    { "STATE_DISCONNECT",    ROREG_INT(BT_STATE_DISCONNECT)},

    // model
    { "MODE_BLE_SERVER",           ROREG_INT(BT_MODE_BLE_SERVER)},
    { "MODE_BLE_CLIENT",           ROREG_INT(BT_MODE_BLE_CLIENT)},
    { "MODE_BLE_BEACON",           ROREG_INT(BT_MODE_BLE_BEACON)},
    { "MODE_BLE_MESH",             ROREG_INT(BT_MODE_BLE_MESH)},
    { "SERVER",                    ROREG_INT(BT_MODE_BLE_SERVER)},
    { "CLIENT",                    ROREG_INT(BT_MODE_BLE_CLIENT)},
    { "BEACON",                    ROREG_INT(BT_MODE_BLE_BEACON)},
    { "MESH",                      ROREG_INT(BT_MODE_BLE_MESH)},

    // FLAGS
    //@const CHR_F_WRITE number FLAGS value of chr, writable, and requires response
    {"CHR_F_WRITE",                ROREG_INT(BLE_GATT_CHR_F_WRITE)},
    //@const CHR_F_READ number chr's FLAGS value, readable
    {"CHR_F_READ",                 ROREG_INT(BLE_GATT_CHR_F_READ)},
    //@const CHR_F_WRITE_NO_RSP number FLAGS value of chr, writable, no response required
    {"CHR_F_WRITE_NO_RSP",         ROREG_INT(BLE_GATT_CHR_F_WRITE_NO_RSP)},
    //@const CHR_F_NOTIFY number chr's FLAGS value, can be subscribed, no reply is required
    {"CHR_F_NOTIFY",               ROREG_INT(BLE_GATT_CHR_F_NOTIFY)},
    //@const CHR_F_INDICATE number chr's FLAGS value, can be subscribed, needs reply
    {"CHR_F_INDICATE",             ROREG_INT(BLE_GATT_CHR_F_INDICATE)},

    // CONFIG
    //@const CFG_ADDR_ORDER number The big and small ends of UUID conversion, used in conjunction with the config function, default 0, optional 0/1
    {"CFG_ADDR_ORDER",                ROREG_INT(CFG_ADDR_ORDER)},

	{ NULL,             ROREG_INT(0)}
};

LUAMOD_API int luaopen_nimble( lua_State *L ) {
    memcpy(&ble_peripheral_srv_uuid, BLE_UUID16_DECLARE(WM_GATT_SVC_UUID), sizeof(ble_uuid16_t));
    memcpy(&s_chr_uuids[0], BLE_UUID16_DECLARE(WM_GATT_WRITE_UUID), sizeof(ble_uuid16_t));
    memcpy(&s_chr_uuids[1], BLE_UUID16_DECLARE(WM_GATT_INDICATE_UUID), sizeof(ble_uuid16_t));
    memcpy(&s_chr_uuids[2], BLE_UUID16_DECLARE(WM_GATT_NOTIFY_UUID), sizeof(ble_uuid16_t));

    s_chr_flags[0] = BLE_GATT_CHR_F_WRITE;
    s_chr_flags[1] = BLE_GATT_CHR_F_INDICATE;
    s_chr_flags[2] = BLE_GATT_CHR_F_NOTIFY;

    rotable2_newlib(L, reg_nimble);
    return 1;
}

