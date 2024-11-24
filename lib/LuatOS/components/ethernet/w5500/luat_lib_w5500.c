/*@Modules w5500
@summary w5500 Ethernet driver
@version 1.0
@date 2022.04.11
@tag LUAT_USE_W5500
@demo w5500*/

#include "luat_base.h"
#ifdef LUAT_USE_W5500
#include "luat_rtos.h"
#include "luat_zbuff.h"
#include "luat_spi.h"
#define LUAT_LOG_TAG "w5500"
#include "luat_log.h"
#include "luat_msgbus.h"

#include "w5500_def.h"
#include "luat_network_adapter.h"
/*Initialize w5500
@api w5500.init(spiid, speed, cs_pin, irq_pin, rst_pin, link_pin)
@int spi channel number, such as 0, 1, 5, select according to the actual situation of the device
@int spi speed, can be set to the highest speed corresponding to SPI
@int cs pin, chip select pin, corresponding to SCS of W5500
@int irq pin, interrupt pin, corresponding to INT of W5500
@int reset pin, reset pin, corresponding to RST of W5500
@int link status pin, can be left blank and not used, not used by default
@usage
w5500.init(spi.SPI_0, 24000000, pin.PB13, pin.PC08, pin.PC09)*/
static int l_w5500_init(lua_State *L){
	luat_spi_t spi = {0};
	spi.id = luaL_checkinteger(L, 1);
	spi.bandrate = luaL_checkinteger(L, 2);
	spi.cs = luaL_checkinteger(L, 3);
	spi.CPHA = 0;
	spi.CPOL = 0;
	spi.master = 1;
	spi.mode = 1;
	spi.dataw = 8;
	spi.bit_dict = 1;
	uint32_t irq_pin = luaL_checkinteger(L, 4);
	uint32_t reset_pin = luaL_checkinteger(L, 5);
	uint32_t link_pin = luaL_optinteger(L, 6, 0xff);
	LLOGD("cs %d irq %d reset %d link %d", spi.cs, irq_pin, reset_pin, link_pin);
	w5500_init(&spi, irq_pin, reset_pin, link_pin);
	return 0;
}

/*w5500 configure network information
@api w5500.config(ip, submask, gateway, mac, RTR, RCR, speed)
@string Static IP address, if you need to use DHCP to obtain it, please write nil
@string subnet mask, ignored if dynamic IP is used
@string gateway, ignored if dynamic ip is used
@string MAC, if you write nil, it will be automatically generated through the MCU unique code. If you want to write it, the length must be 6 bytes.
@int Retry interval, default is 2000, unit is 100us, don’t change if you don’t understand
@int Maximum number of retries, default 8, don’t change if you don’t understand
@int Speed   type, currently only 0 hardware configuration, 1 adaptive, default is 0
@usage
w5500.config("192.168.1.2", "255.255.255.0", "192.168.1.1", string.fromHex("102a3b4c5d6e"))*/
static int l_w5500_config(lua_State *L){
	if (!w5500_device_ready())
	{
		lua_pushboolean(L, 0);
		LLOGD("device no ready");
		return 1;
	}

	if (lua_isstring(L, 1))
	{
	    size_t ip_len = 0;
	    const char* ip = luaL_checklstring(L, 1, &ip_len);
	    size_t mask_len = 0;
	    const char* mask = luaL_checklstring(L, 2, &mask_len);
	    size_t gateway_len = 0;
	    const char* gateway = luaL_checklstring(L, 3, &gateway_len);
#ifdef LUAT_USE_LWIP
	    ip_addr_t lwip_ip,lwip_mask,lwip_gateway;
	    ipaddr_aton(ip, &lwip_ip);
	    ipaddr_aton(mask, &lwip_mask);
	    ipaddr_aton(gateway, &lwip_gateway);
	    w5500_set_static_ip(ip_addr_get_ip4_u32(&lwip_ip), ip_addr_get_ip4_u32(&lwip_mask), ip_addr_get_ip4_u32(&lwip_gateway));
#else
	    w5500_set_static_ip(network_string_to_ipv4(ip, ip_len), network_string_to_ipv4(mask, mask_len), network_string_to_ipv4(gateway, gateway_len));
#endif
	}
	else
	{
		w5500_set_static_ip(0, 0, 0);
	}

	if (lua_isstring(L, 4))
	{
		size_t mac_len = 0;
		const char *mac = luaL_checklstring(L, 4, &mac_len);
		w5500_set_mac((uint8_t*)mac);
	}

	w5500_set_param(luaL_optinteger(L, 5, 2000), luaL_optinteger(L, 6, 8), luaL_optinteger(L, 7, 0), 0);


	w5500_reset();
	lua_pushboolean(L, 1);
	return 1;
}
/*Register w5500 into the universal network interface
@api w5500.bind(id)
@int Universal network channel number
@usage
-- If the version used does not have the socket library, change it to network.ETH0
w5500.bind(socket.ETH0)*/
static int l_w5500_network_register(lua_State *L){

	int index = luaL_checkinteger(L, 1);
	w5500_register_adapter(index);
	return 0;
}

/*To obtain the current MAC of w5500, it must be used after init. If your own MAC is set in the config, you need to delay reading it for a while.
@api w5500.getMac()
@return string current MAC
@usage
local mac = w5500.getMac()
log.info("w5500 mac", mac:toHex())*/
static int l_w5500_get_mac(lua_State *L){
	uint8_t mac[6] = {0};
	w5500_get_mac(mac);
	lua_pushlstring(L, (const char*)mac, 6);
	return 1;
}

#include "rotable2.h"
static const rotable_Reg_t reg_w5500[] =
{
    { "init",           ROREG_FUNC(l_w5500_init)},
	{ "config",           ROREG_FUNC(l_w5500_config)},
	{ "bind",           ROREG_FUNC(l_w5500_network_register)},
	{ "getMac",           ROREG_FUNC(l_w5500_get_mac)},
	{ NULL,            ROREG_INT(0)}
};

LUAMOD_API int luaopen_w5500( lua_State *L ) {
    luat_newlib2(L, reg_w5500);
    return 1;
}

static int l_nw_state_handler(lua_State *L, void* ptr) {
	(void)ptr;
	rtos_msg_t* msg = (rtos_msg_t*)lua_topointer(L, -1);
	lua_getglobal(L, "sys_pub");

	switch (msg->arg1)
	{
/*@sys_pub w5500
Connected to the Internet
IP_READY
@usage
-- This message will be sent once after connecting to the Internet
sys.subscribe("IP_READY", function(ip, adapter)
    log.info("w5500", "IP_READY", ip, (adapter or -1) == socket.LWIP_GP)
end)*/
	case W5500_IP_READY:
		lua_pushliteral(L, "IP_READY");
		uint32_t ip = msg->arg2;
		lua_pushfstring(L, "%d.%d.%d.%d", (ip) & 0xFF, (ip >> 8) & 0xFF, (ip >> 16) & 0xFF, (ip >> 24) & 0xFF);
		lua_pushinteger(L, NW_ADAPTER_INDEX_ETH0);
		lua_call(L, 3, 0);
		break;
/*@sys_pub w5500
Disconnected
IP_LOSE
@usage
-- This message will be sent once after the network is disconnected
sys.subscribe("IP_LOSE", function(adapter)
    log.info("w5500", "IP_LOSE", (adapter or -1) == socket.ETH0)
end)*/
	case W5500_IP_LOSE:
		lua_pushliteral(L, "IP_LOSE");
		lua_pushinteger(L, NW_ADAPTER_INDEX_ETH0);
		lua_call(L, 2, 0);
		break;
/*@sys_pub w5500
w5500 status change
W5500_IND
@usage
sys.subscribe("W5500_IND", function(status)
    --The values   of status are:
    -- CABLE_INSERT network cable insertion
    -- CABLE_REMOVE Unplug the network cable
-- DHCP_TIMEOUT timeout for obtaining IP
    log.info("w5500 status", status)
end)*/
	case W5500_CABLE_INSERT:
		lua_pushliteral(L, "W5500_IND");
		lua_pushliteral(L, "CABLE_INSERT");
		lua_call(L, 2, 0);
		break;
	case W5500_CABLE_REMOVE:
		lua_pushliteral(L, "W5500_IND");
		lua_pushliteral(L, "CABLE_REMOVE");
		lua_call(L, 2, 0);
		break;
	case W5500_DHCP_RESULT:
		if (0 == msg->arg2)
		{
			lua_pushliteral(L, "W5500_IND");
			lua_pushliteral(L, "DHCP_TIMEOUT");
			lua_call(L, 2, 0);
		}
		break;
	default:
		break;
	}
	return 0;
}

//W5500 status callback function
void w5500_nw_state_cb(int state, uint32_t param) {
	rtos_msg_t msg = {0};
	msg.handler = l_nw_state_handler;
	msg.arg1 = state; // READY
	msg.arg2 = param;
	luat_msgbus_put(&msg, 0);
}

#endif
