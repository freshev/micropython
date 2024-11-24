/*@Modulesulwip
@summary lwip integration in user space (under development)
@version 1.0
@date 2024.1.22
@autherwendal
@tag LUAT_USE_ULWIP
@usage
--[[
Note: This library is under development and the interface may change at any time.
LWIP integration in user space, used to support netif network integration of lwip, to achieve direct control of the sending and receiving of MAC packets/IP packets in Lua code

The overall data path is as follows

lua code -> ulwip.input -> lwip(netif->input) -> lwip processing logic -> luatos socket framework

lua code <- ulwip callback function <- lwip(netif->low_level_output) <- lwip processing logic <- luatos socket framework

Application examples:
1. The wifi Modules of Air601 serves as the controlled terminal and sends and receives MAC packets through UART/SPI to realize the function of the integrated wifi Modules of Air780E/Air780EP.
2. Use Ethernet Moduless such as W5500/CH395/ENC28J60 to control the sending and receiving of mac packets in the user's lua code, and integrate it into the luatos socket framework
3. Integrate lowpan6 through Bluetooth Modules

-- Under development, please pay attention to https://github.com/wendal/xt804-spinet
]]*/

#include "luat_base.h"
#include "luat_ulwip.h"
#include "luat_crypto.h"
#include "luat_gpio.h"
#include "luat_spi.h"

#define LUAT_LOG_TAG "ulwip"
#include "luat_log.h"

static ulwip_ctx_t nets[USERLWIP_NET_COUNT];
extern struct netif *netif_default;

//Search for the netif corresponding to adpater_index
struct netif* ulwip_find_netif(uint8_t adapter_index) {
    struct netif *netif = NULL;
    for (size_t i = 0; i < USERLWIP_NET_COUNT; i++)
    {
        if (nets[i].adapter_index == adapter_index)
        {
            netif = nets[i].netif;
            break;
        }
    }
    return netif;
}

int ulwip_find_index(uint8_t adapter_index) {
    for (size_t i = 0; i < USERLWIP_NET_COUNT; i++)
    {
        if (nets[i].adapter_index == adapter_index)
        {
            return i;
        }
    }
    return -1;
}

static int netif_ip_event_cb(lua_State *L, void* ptr) {
    rtos_msg_t* msg = (rtos_msg_t*)lua_topointer(L, -1);
    lua_getglobal(L, "sys_pub");
    char buff[32] = {0};
    ulwip_ctx_t* ctx = (ulwip_ctx_t*)ptr;
    if (lua_isfunction(L, -1)) {
        if (msg->arg2) {
            lua_pushstring(L, "IP_READY");
            ipaddr_ntoa_r(&ctx->netif->ip_addr, buff,  32);
            LLOGD("IP_READY %d %s", ctx->adapter_index, buff);
            lua_pushstring(L, buff);
            lua_pushinteger(L, ctx->adapter_index);
            lua_call(L, 3, 0);
        }
        else {
            lua_pushstring(L, "IP_LOSE");
            LLOGD("IP_LOSE %d", ctx->adapter_index);
            lua_pushinteger(L, ctx->adapter_index);
            lua_call(L, 2, 0);
        }
    }
    return 0;
}

int ulwip_netif_ip_event(ulwip_ctx_t* ctx) {
    struct netif* netif = ctx->netif;
    int ready_now = !ip_addr_isany(&netif->ip_addr);
    ready_now &= netif_is_link_up(netif);
    ready_now &= netif_is_up(netif);

    net_lwip2_set_link_state(ctx->adapter_index, ready_now);
    if (ctx->ip_ready == ready_now) {
        return 0;
    }
    ctx->ip_ready = ready_now;
    rtos_msg_t msg = {0};
    msg.arg1 = ctx->adapter_index;
    msg.arg2 = ready_now;
    msg.ptr = ctx;
    msg.handler = netif_ip_event_cb;
    luat_msgbus_put(&msg, 0);
    return 0;
}

//Callback function, used for lwip's netif output data
int l_ulwip_netif_output_cb(lua_State *L, void* ptr) {
    // LLOGD("l_ulwip_netif_output_cb");
    rtos_msg_t* msg = (rtos_msg_t*)lua_topointer(L, -1);
    int idx = ulwip_find_index(msg->arg2);
    if (idx < 0 || nets[idx].netif == NULL) {
        LLOGE("Illegal adapter index number %d", msg->arg2);
        return 0;
    }
    lua_geti(L, LUA_REGISTRYINDEX, nets[idx].output_lua_ref);
    if (lua_isfunction(L, -1)) {
        lua_pushinteger(L, msg->arg2);
        if (nets[idx].use_zbuff_out) {
            luat_zbuff_t* buff = lua_newuserdata(L, sizeof(luat_zbuff_t));
            if (buff == NULL)
            {
                LLOGE("malloc failed for l_ulwip_netif_output_cb");
                return 0;
            }
            memset(buff, 0, sizeof(luat_zbuff_t));
            buff->addr = ptr;
            buff->len = msg->arg1;
            buff->used = msg->arg1;
            luaL_setmetatable(L, LUAT_ZBUFF_TYPE);
        }
        else {
            lua_pushlstring(L, (const char*)ptr, msg->arg1);
            luat_heap_free(ptr);
        }
        lua_call(L, 2, 0);
    }
    else {
        // LLOGD("Not a callback function %d", nets[msg->arg2].output_lua_ref);
        luat_heap_free(ptr);
    }
    return 0;
}

static err_t netif_output(struct netif *netif, struct pbuf *p) {
    // LLOGD("lwip data to be sent %p %d", p, p->tot_len);
    rtos_msg_t msg = {0};
    msg.handler = l_ulwip_netif_output_cb;
    msg.arg1 = p->tot_len;
    msg.arg2 = -1;
    for (size_t i = 0; i < USERLWIP_NET_COUNT; i++)
    {
        if (nets[i].netif == netif)
        {
            msg.arg2 = nets[i].adapter_index;
            break;
        }
    }
    if (msg.arg2 < 0) {
        LLOGE("netif_output %p not found", netif);
        return ERR_IF;
    }
    msg.ptr = luat_heap_malloc(p->tot_len);
    if (msg.ptr == NULL)
    {
        LLOGE("malloc %d failed for netif_output", p->tot_len);
        return ERR_MEM;
    }
    
    size_t offset = 0;
    do {
        memcpy((char*)msg.ptr + offset, p->payload, p->len);
        offset += p->len;
        p = p->next;
    } while (p);
    luat_msgbus_put(&msg, 0);
    return 0;
}

static err_t luat_netif_init(struct netif *netif) {
    for (size_t i = 0; i < USERLWIP_NET_COUNT; i++)
    {
    if (nets[i].netif == netif)
        {
            LLOGD("netif init %d %p", nets[i].adapter_index, netif);

            netif->linkoutput = netif_output;
            netif->output     = ulwip_etharp_output;
            #if LWIP_IPV6
            netif->output_ip6 = ethip6_output;
            #endif
            netif->mtu        = nets[i].mtu;
            netif->flags      = nets[i].flags;
            memcpy(netif->hwaddr, nets[i].hwaddr, ETH_HWADDR_LEN);
            netif->hwaddr_len = ETH_HWADDR_LEN;
            return 0;
        }
    }
    return ERR_IF;
}

/*Initialize lwip netif
@api ulwip.setup(adapter_index, mac, output_lua_ref, opts)
@int adapter_index adapter number
@string mac network card mac address
@function output_lua_ref callback function, parameters are (adapter_index, data)
@table additional parameters, such as {mtu=1500, flags=(ulwip.FLAG_BROADCAST | ulwip.FLAG_ETHARP)}
@return boolean success or failure
@usage
--Initialize an adapter and set the callback function
ulwip.setup(socket.LWIP_STA, string.fromHex("18fe34a27b69"), function(adapter_index, data)
    log.info("ulwip", "output_lua_ref", adapter_index, data:toHex())
end)
-- Note that after setup, the status of netif is down. Only after calling ulwip.updown(adapter_index, true) can the data be sent and received normally.

-- Additional parameter configuration table optional values
-- mtu, default 1460
-- flags, default ulwip.FLAG_BROADCAST | ulwip.FLAG_ETHARP | ulwip.FLAG_ETHERNET | ulwip.FLAG_IGMP | ulwip.FLAG_MLD6
-- zbuff_out callback function accepts zbuff as parameter, defaults to false
-- reverse local lwip device, reverse call logic, default false, this parameter is designed to intercept the hardware networking data of the current device*/
static int l_ulwip_setup(lua_State *L) {
    // Must have adapter number
    uint8_t adapter_index = (uint8_t)luaL_checkinteger(L, 1);
    //Set MAC address, required
    const char* mac = luaL_checkstring(L, 2);
    if (adapter_index >= NW_ADAPTER_INDEX_LWIP_NETIF_QTY)
    {
        LLOGE("Illegal adapter_index %d", adapter_index);
        return 0;
    }
    if (!lua_isfunction(L, 3)) {
        LLOGE("output_lua_ref must be a function");
        return 0;
    }
    uint16_t mtu = 1460;
    uint8_t flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET | NETIF_FLAG_IGMP | NETIF_FLAG_MLD6;
    uint16_t zbuff_out = 0;
    uint8_t reverse = 0;
    if (lua_istable(L, 4)) {
        lua_getfield(L, 4, "mtu");
        if (lua_isinteger(L, -1)) {
            mtu = (uint16_t)luaL_checkinteger(L, -1);
        }
        lua_pop(L, 1);
        
        lua_getfield(L, 4, "flags");
        if (lua_isinteger(L, -1)) {
            flags = (uint8_t)luaL_checkinteger(L, -1);
        }
        lua_pop(L, 1);

        lua_getfield(L, 4, "zbuff_out");
        if (lua_isboolean(L, -1)) {
            zbuff_out = lua_toboolean(L, -1);
            if (zbuff_out) {
                LLOGD("Use zbuff as the callback function of netif out");
            }
        }
        lua_pop(L, 1);
        
        lua_getfield(L, 4, "reverse");
        if (lua_isinteger(L, -1)) {
            reverse = (uint8_t)luaL_checkinteger(L, -1);
        }
        if (lua_isboolean(L, -1)) {
            reverse = (uint8_t)lua_toboolean(L, -1);
        }
        lua_pop(L, 1);
    }

    struct netif *netif = NULL;
    struct netif *tmp = NULL;
    for (size_t i = 0; i < USERLWIP_NET_COUNT; i++)
    {
        if (nets[i].netif == NULL)
        {
            
            if (reverse) {
                #if defined(CHIP_EC718) || defined(CHIP_EC618) || defined(CHIP_EC716)
                extern struct netif * net_lwip_get_netif(uint8_t adapter_index);
                netif = net_lwip_get_netif(adapter_index);
                #else
                netif = netif_default;
                #endif
                if (netif == NULL) {
                    LLOGE("The netif of the current device does not exist and the reverse parameter cannot be enabled!!");
                    return 0;
                }
                nets[i].adapter_index = adapter_index;
                nets[i].netif = netif;
                nets[i].mtu = netif->mtu; // Actually meaningless
                nets[i].flags = netif->flags;
                nets[i].use_zbuff_out = zbuff_out;
                nets[i].reverse = reverse;
                lua_pushvalue(L, 3);
                memcpy(nets[i].hwaddr, netif->hwaddr, ETH_HWADDR_LEN);
                nets[i].output_lua_ref = luaL_ref(L, LUA_REGISTRYINDEX);
                lua_pushboolean(L, 1);
                LLOGD("Mount netif (reverse mode) %p %d %d", netif, i, adapter_index);
                return 1;
            }

            netif = luat_heap_malloc(sizeof(struct netif));
            if (netif) {
                memset(netif, 0, sizeof(struct netif));
                nets[i].adapter_index = adapter_index;
                nets[i].netif = netif;
                nets[i].mtu = mtu;
                nets[i].flags = flags;
                nets[i].use_zbuff_out = zbuff_out;
                nets[i].reverse = reverse;
                lua_pushvalue(L, 3);
                memcpy(nets[i].hwaddr, mac, ETH_HWADDR_LEN);
                nets[i].output_lua_ref = luaL_ref(L, LUA_REGISTRYINDEX);
                LLOGD("Mount netif (normal mode) %p %d %d", netif, i, adapter_index);
                break;
            }
        }
    }
    if (netif == NULL)
    {
        LLOGE("There is no free netif");
        return 0;
    }

    // Netif has been assigned, continue initialization
    tmp = netif_add(netif, IP4_ADDR_ANY4, IP4_ADDR_ANY4, IP4_ADDR_ANY4, NULL, luat_netif_init, netif_input);
    netif->name[0] = 'u';
    netif->name[1] = 's';
    if (NULL == tmp) {
        LLOGE("netif_add returns exception!!!");
    }
    #if LWIP_IPV6
    netif_create_ip6_linklocal_address(netif, 1);
    netif->ip6_autoconfig_enabled = 1;
    #endif

    net_lwip2_set_netif(adapter_index, netif);
    
    lua_pushboolean(L, 1);
    return 1;
}

/*Set the status of netif
@api ulwip.updown(adapter_index, up)
@int adapter_index adapter number
@boolean up true means up, false means down
@return boolean success or failure
@usage
-- Refer to ulwip.setup*/
static int l_ulwip_updown(lua_State *L) {
    // Must have adapter number
    uint8_t adapter_index = (uint8_t)luaL_checkinteger(L, 1);
    int idx = ulwip_find_index(adapter_index);
    if (idx < 0) {
        LLOGE("netif not found");
        return 0;
    }
    if (lua_isboolean(L, 2)) {
        if (lua_toboolean(L, 2)) {
            netif_set_up(nets[idx].netif);
        }
        else {
            netif_set_down(nets[idx].netif);
        }
        ulwip_netif_ip_event(&nets[idx]);
    }
    lua_pushboolean(L, netif_is_up(nets[idx].netif));
    return 1;
}

/*Set the physical link status of netif
@api ulwip.link(adapter_index, up)
@int adapter_index adapter number
@boolean up true means up, false means down
@return boolean current status
@usage
-- Refer to ulwip.setup*/
static int l_ulwip_link(lua_State *L) {
    // Must have adapter number
    uint8_t adapter_index = (uint8_t)luaL_checkinteger(L, 1);
    int idx = ulwip_find_index(adapter_index);
    if (idx < 0) {
        LLOGE("netif not found");
        return 0;
    }
    if (lua_isboolean(L, 2))
    {
        if (lua_toboolean(L, 2))
        {
            netif_set_link_up(nets[idx].netif);
        }
        else {
            netif_set_link_down(nets[idx].netif);
        }
        ulwip_netif_ip_event(&nets[idx]);
    }
    lua_pushboolean(L, netif_is_link_up(nets[idx].netif));
    return 1;
}

static void netif_input_cb(void *ptr) {
    netif_cb_ctx_t* cb_ctx = (netif_cb_ctx_t*)ptr;
    if (ERR_OK != cb_ctx->ctx->netif->input(cb_ctx->p, cb_ctx->ctx->netif)) {
        LLOGW("ctx->netif->input failed %d", cb_ctx->p->tot_len);
        pbuf_free(cb_ctx->p);
    }
    luat_heap_free(cb_ctx);
}

/*Enter data into netif
@api ulwip.input(adapter_index, data, len, offset)
@int adapter_index adapter number
@string/userdata data input data
@int If data is zbuff, len defaults to used of zbuff, which is invalid for string
@int If data is zbuff, offset is the starting position of the data. The default is 0, which is invalid for string.
@return boolean success or failure
@usage
-- Refer to ulwip.setup*/
static int l_ulwip_input(lua_State *L) {
    // Must have adapter number
    uint8_t adapter_index = (uint8_t)luaL_checkinteger(L, 1);
    int ret = 0;
    struct pbuf *q = NULL;
    const char* data = NULL;
    size_t len = 0;
    size_t offset = 0;
    int idx = ulwip_find_index(adapter_index);
    if (idx < 0) {
        LLOGE("netif %d not found", adapter_index);
        return 0;
    }
    if (nets[idx].netif == NULL || nets[idx].netif->input == NULL) {
        LLOGE("This netif does not support input operation %d", adapter_index);
        return 0;
    }
    if (lua_type(L, 2) == LUA_TSTRING)
    {
        data = luaL_checklstring(L, 2, &len);
    }
    else if (lua_type(L, 2) == LUA_TUSERDATA) {
        luat_zbuff_t* zb = (luat_zbuff_t*)luaL_checkudata(L, 2, LUAT_ZBUFF_TYPE);
        data = (const char*)zb->addr;
        if (lua_isinteger(L, 3)) {
            len = luaL_checkinteger(L, 3);
            offset = luaL_checkinteger(L, 4);
            data += offset;
        }
        else {
            len = zb->used;
        }
    }
    else {
        LLOGE("Unknown data format, currently only supports zbuff and string");
        return 0;
    }
    // LLOGD("Input mac frame %d %02X:%02X:%02X:%02X:%02X:%02X", len, data[0], data[1], data[2], data[3], data[4], data[5]);
    struct pbuf *p = pbuf_alloc(PBUF_RAW, (uint16_t)len, PBUF_RAM);
    if (p == NULL) {
        LLOGE("pbuf_alloc failed");
        return 0;
    }
    for (q = p; q != NULL; q = q->next) {
        memcpy(q->payload, data, q->len);
        data += q->len;
    }
    #if NO_SYS
    ret = nets[idx].netif->input(p, nets[idx].netif);
    #else
    netif_cb_ctx_t* cb_ctx = (netif_cb_ctx_t*)luat_heap_malloc(sizeof(netif_cb_ctx_t));
    if (cb_ctx == NULL) {
        LLOGE("netif->input ret %d", ret);
        LWIP_DEBUGF(NETIF_DEBUG, ("l_ulwip_input: IP input error\n"));
        pbuf_free(p);
        return 0;
    }
    memset(cb_ctx, 0, sizeof(netif_cb_ctx_t));
    cb_ctx->ctx = &nets[idx];
    cb_ctx->p = p;
    ret = tcpip_callback(netif_input_cb, cb_ctx);
    if(ret != ERR_OK) {
        luat_heap_free(cb_ctx);
        cb_ctx = NULL;
    }
    #endif
    if(ret != ERR_OK) {
        LLOGE("netif->input ret %d", ret);
        LWIP_DEBUGF(NETIF_DEBUG, ("l_ulwip_input: IP input error\n"));
        pbuf_free(p);
        return 0;
    }
    lua_pushboolean(L, 1);
    return 1;
}


/*Enable or disable dhcp
@api ulwip.dhcp(adapter_index, up)
@int adapter_index adapter number
@boolean up true means startup, false means shutdown
@return boolean current status
@usage
-- Refer to ulwip.setup*/
static int l_ulwip_dhcp(lua_State *L) {
    // Must have adapter number
    uint8_t adapter_index = (uint8_t)luaL_checkinteger(L, 1);
    struct netif* netif = ulwip_find_netif(adapter_index);
    if (netif == NULL) {
        LLOGE("netif not found");
        return 0;
    }
    int dhcp_enable = lua_toboolean(L, 2);
    int i = ulwip_find_index(adapter_index);
    if (i < 0)
    {
        LLOGE("Adapter_index %d not found", adapter_index);
        return 0;
    }
    nets[i].dhcp_enable = dhcp_enable;
    if (dhcp_enable) {
        nets[i].ip_static = 0;
        ulwip_dhcp_client_start(&nets[i]);
    }
    else {
        ulwip_dhcp_client_stop(&nets[i]);
    }
    lua_pushboolean(L, 1);
    return 1;
}

/*Set or get ip information
@api ulwip.ip(adapter_index, ip, netmask, gw)
@int adapter_index adapter number
@string ip IP address, can be left blank only when obtaining
@string netmask subnet mask, can be left blank only when obtaining
@string gw gateway address, can be left blank only when obtaining
@return string ip address, subnet mask, gateway address
@usage
-- Get existing value
local ip, netmask, gw = ulwip.ip(socket.LWIP_STA)
-- Set new value
ulwip.ip(socket.LWIP_STA, "192.168.0.1", "255.255.255.0", "192.168.0.1")*/
static int l_ulwip_ip(lua_State *L) {
    const char* tmp = NULL;
    // Must have adapter number
    uint8_t adapter_index = (uint8_t)luaL_checkinteger(L, 1);
    struct netif* netif = ulwip_find_netif(adapter_index);
    if (netif == NULL) {
        LLOGE("netif %d not found", adapter_index);
        return 0;
    }
    if (lua_type(L, 2) == LUA_TSTRING)
    {
        tmp = luaL_checkstring(L, 2);
        ipaddr_aton(tmp, &netif->ip_addr);
        tmp = luaL_checkstring(L, 3);
        ipaddr_aton(tmp, &netif->netmask);
        tmp = luaL_checkstring(L, 4);
        ipaddr_aton(tmp, &netif->gw);
        
        int idx = ulwip_find_index(adapter_index);
        nets[idx].ip_static = !ip_addr_isany(&netif->ip_addr);
    }
    //Feedback IP information
    tmp = ip_ntoa(&netif->ip_addr);
    lua_pushstring(L, tmp);
    tmp = ip_ntoa(&netif->netmask);
    lua_pushstring(L, tmp);
    tmp = ip_ntoa(&netif->gw);
    lua_pushstring(L, tmp);
    return 3;
}

/*Register netif into luatos socket
@api ulwip.reg(adapter_index)
@int adapter_index adapter number
@return boolean success or failure
@usage
-- Refer to ulwip.setup*/
static int l_ulwip_reg(lua_State *L) {
    // Must have adapter number
    uint8_t adapter_index = (uint8_t)luaL_checkinteger(L, 1);
    struct netif* netif = ulwip_find_netif(adapter_index);
    if (netif == NULL) {
        LLOGE("netif %d not found", adapter_index);
        return 0;
    }
    net_lwip2_register_adapter(adapter_index);
    lua_pushboolean(L, 1);
    return 1;
}

/*Operate XT804 for SPI fast sending and receiving
@api ulwip.xt804_xfer(spi_id, cs_pin, addr, zbuff, len, offset, auto_seek, auto_len)
@int spi_id SPI ID number
@int cs_pin GPIO number of CS pin
@int addr register address
@zbuff zbuff object
@int len   length
@int offset offset, default buff:used()
@boolean auto_seek whether to automatically move the offset, default false
@int auto_len Automatic fragmentation length, automatically selected by register by default
@return nil no return value
@usage
-- This function is an auxiliary function*/
static int l_ulwip_xt804_xfer(lua_State *L) {
    int spi_id = luaL_checkinteger(L, 1);  // SPI ID number
    int cs_pin = luaL_checkinteger(L, 2);  //GPIO number of CS pin
    int addr = luaL_checkinteger(L, 3);    //Register address
    luat_zbuff_t* zbuff = ((luat_zbuff_t *)luaL_checkudata(L, 4, LUAT_ZBUFF_TYPE));
    size_t len = luaL_checkinteger(L, 5);
    size_t offset = luaL_optinteger(L, 6, zbuff->used);
    int auto_seek = lua_toboolean(L, 7);
    size_t auto_len = luaL_optinteger(L, 8, 0);
    if (auto_len == 0) {
        int tmpaddr = addr & 0x7F;
        if (tmpaddr == 0x00 || tmpaddr == 0x10 || tmpaddr == 0x01 || tmpaddr == 0x11)
            auto_len = 4;
        else
            auto_len = 2;
    }
    if (len % auto_len != 0) {
        len = (len + auto_len - 1) / auto_len * auto_len;
    }

    char tmp[5] = {0};
    tmp[0] = addr & 0xFF;
    int is_write = addr & 0x80;
    
    if (is_write) {
        for (size_t i = 0; i < len / auto_len; i++) {
            memcpy(tmp+1, zbuff->addr + offset, auto_len);
            luat_gpio_set(cs_pin, 0);
            luat_spi_send(spi_id, (const char*)tmp, auto_len + 1);
            luat_gpio_set(cs_pin, 1);
            offset += auto_len;
        }
    }
    else {
        for (size_t i = 0; i < len / auto_len; i++) {
            luat_gpio_set(cs_pin, 0);
            memcpy(tmp+1, zbuff->addr + offset, auto_len);
            luat_spi_send(spi_id, (const char*)tmp, 1);
            luat_spi_recv(spi_id, (char*)(zbuff->addr + offset), auto_len);
            luat_gpio_set(cs_pin, 1);
            offset += auto_len;
        }
    }
    if (auto_seek) {
        zbuff->used += len;
    }
    return 0;
 }

#include "rotable2.h"
static const rotable_Reg_t reg_ulwip[] =
{
    { "input" ,             ROREG_FUNC(l_ulwip_input)},
    { "setup" ,             ROREG_FUNC(l_ulwip_setup)},
    { "updown" ,            ROREG_FUNC(l_ulwip_updown)},
    { "link" ,              ROREG_FUNC(l_ulwip_link)},
    { "dhcp" ,              ROREG_FUNC(l_ulwip_dhcp)},
    { "ip" ,                ROREG_FUNC(l_ulwip_ip)},
    { "reg" ,               ROREG_FUNC(l_ulwip_reg)},

    { "xt804_xfer" ,        ROREG_FUNC(l_ulwip_xt804_xfer)},

    // Network card FLAGS, default
    // NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET | NETIF_FLAG_IGMP | NETIF_FLAG_MLD6

    // @const FLAG_BROADCAST number supports broadcasting
    { "FLAG_BROADCAST",     ROREG_INT(NETIF_FLAG_BROADCAST)}, 
    // @const FLAG_ETHARP number supports ARP
    { "FLAG_ETHARP",        ROREG_INT(NETIF_FLAG_ETHARP)}, 
    // @const FLAG_ETHERNET number Ethernet mode
    { "FLAG_ETHERNET",      ROREG_INT(NETIF_FLAG_ETHERNET)}, 
    // @const FLAG_IGMP number 支持IGMP
    { "FLAG_IGMP",          ROREG_INT(NETIF_FLAG_IGMP)}, 
    // @const FLAG_MLD6 number 支持_MLD6
    { "FLAG_MLD6",          ROREG_INT(NETIF_FLAG_MLD6)}, 
	{ NULL,                 ROREG_INT(0)}
};

LUAMOD_API int luaopen_ulwip( lua_State *L ) {
    luat_newlib2(L, reg_ulwip);
    return 1;
}
