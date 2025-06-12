
#include "luat_crypto.h"
#include "luat_rtos.h"
#include "common_api.h"

#include "luat_rtos.h"
#include "platform_def.h"
#include "ctype.h"
#include "luat_network_adapter.h"

typedef struct
{
#ifdef LUAT_USE_LWIP
    network_ctrl_t lwip_ctrl_table[LWIP_NUM_SOCKETS];
//  HANDLE network_mutex;
#endif
    int last_adapter_index;
    int default_adapter_index;
    llist_head dns_cache_head;
#ifdef LUAT_USE_LWIP
    uint8_t lwip_ctrl_busy[LWIP_NUM_SOCKETS];
#endif
    uint8_t is_init;
}network_info_t;

typedef struct
{

    network_adapter_info *opt;
    void *user_data;
    uint8_t *ctrl_busy;
    network_ctrl_t *ctrl_table;
    uint16_t port;
}network_adapter_t;

static network_adapter_t prv_adapter_table[NW_ADAPTER_QTY];
static network_info_t prv_network = {
        .last_adapter_index = -1,
        .default_adapter_index = -1,
        .is_init = 0,
};
static const char *prv_network_event_id_string[] =
{
        "adapter reset",
        "LINK status change",
        "time out",
        "DNS results",
        "Sent successfully",
        "There is new data",
        "The receive buffer is full",
        "Disconnected successfully",
        "Close the peer",
        "Connection successful",
        "Connection abnormality",
        "Start listening",
        "New client is coming",
        "wake",
        "unknown",
};

static const char *prv_network_ctrl_state_string[] =
{
        "Hardware offline",
        "Offline",
        "Waiting for DNS",
        "Connecting",
        "TLS handshake in progress",
        "online",
        "Listening",
        "Offline",
        "unknown"
};

static const char *prv_network_ctrl_wait_state_string[] =
{
        "No waiting",
        "Waiting for hardware to come online",
        "Wait for connection to complete",
        "Wait for sending to complete",
        "Waiting for offline completion",
        "Wait for any network changes",
        "unknown",
};

static const char *prv_network_ctrl_callback_event_string[] =
{
        "Hardware status callback",
        "Connection status callback",
        "Offline status callback",
        "Send status callback",
        "Any network change callback",
};

const char *network_ctrl_event_id_string(uint32_t event)
{
    if (event > EV_NW_END || event < EV_NW_RESET)
    {
        return prv_network_event_id_string[EV_NW_END - EV_NW_RESET];
    }
    return prv_network_event_id_string[event - EV_NW_RESET];
}

const char *network_ctrl_state_string(uint8_t state)
{
    if (state > NW_STATE_DISCONNECTING)
    {
        return prv_network_ctrl_state_string[NW_STATE_DISCONNECTING + 1];
    }
    return prv_network_ctrl_state_string[state];
}

const char *network_ctrl_wait_state_string(uint8_t state)
{
    if (state > NW_WAIT_EVENT)
    {
        return prv_network_ctrl_wait_state_string[NW_WAIT_EVENT + 1];
    }
    return prv_network_ctrl_wait_state_string[state];
}

const char *network_ctrl_callback_event_string(uint32_t event)
{
    if (event > EV_NW_RESULT_EVENT || event < EV_NW_RESULT_LINK)
    {
        return prv_network_ctrl_callback_event_string[event - EV_NW_RESULT_EVENT + 1];
    }
    return prv_network_ctrl_callback_event_string[event - EV_NW_RESULT_EVENT];
}

network_adapter_info* network_adapter_fetch(int id, void** userdata) {
    if (id >= 0 && id < NW_ADAPTER_QTY) {
        if (prv_adapter_table[id].opt) {
            *userdata = prv_adapter_table[id].user_data;
            return prv_adapter_table[id].opt;
        }
    }
    return NULL;
}


#include "net_lwip.h"
extern void DBG_Printf(const char* format, ...);
extern void DBG_HexPrintf(void *Data, unsigned int len);
//#define DBG(x,y...)       DBG_Printf("%s %d:"x"\r\n", __FUNCTION__,__LINE__,##y)
//#define DBG_ERR(x,y...)       DBG_Printf("%s %d:"x"\r\n", __FUNCTION__,__LINE__,##y)


#define __NW_DEBUG_ENABLE__
#define LUAT_LOG_NO_NEWLINE
#ifdef __NW_DEBUG_ENABLE__
#ifdef LUAT_LOG_NO_NEWLINE
#undef DBG
#define DBG(x,y...) do {if (ctrl->is_debug) {DBG_Printf("%s %d:"x, __FUNCTION__,__LINE__,##y);}} while(0)
#define DBG_ERR(x,y...) DBG_Printf("%s %d:"x, __FUNCTION__,__LINE__,##y)
#else
#define DBG(x,y...) do {if (ctrl->is_debug) {DBG_Printf("%s %d:"x"\r\n", __FUNCTION__,__LINE__,##y);}} while(0)
#define DBG_ERR(x,y...) DBG_Printf("%s %d:"x"\r\n", __FUNCTION__,__LINE__,##y)
#endif
#else
#define DBG(x,y...)
#define DBG_ERR(x,y...)
#endif
#define NW_LOCK     platform_lock_mutex(ctrl->mutex)
#define NW_UNLOCK   platform_unlock_mutex(ctrl->mutex)

#define SOL_SOCKET  0xfff    /* options for socket level */
#define SO_REUSEADDR   0x0004 /* Allow local address reuse */
#define SO_KEEPALIVE   0x0008 /* keep connections alive */

#define IPPROTO_ICMP    1
#define IPPROTO_TCP     6
#define TCP_NODELAY    0x01    /* don't delay send to coalesce packets */
#define TCP_KEEPALIVE  0x02    /* send KEEPALIVE probes when idle for pcb->keep_idle milliseconds */
#define TCP_KEEPIDLE   0x03    /* set pcb->keep_idle  - Same as TCP_KEEPALIVE, but use seconds for get/setsockopt */
#define TCP_KEEPINTVL  0x04    /* set pcb->keep_intvl - Use seconds for get/setsockopt */
#define TCP_KEEPCNT    0x05    /* set pcb->keep_cnt   - Use number of probes sent for get/setsockopt */



static uint8_t network_check_ip_same(luat_ip_addr_t *ip1, luat_ip_addr_t *ip2)
{
#if defined ENABLE_PSIF
    return ip_addr_cmp(ip1, ip2);
#else
    return ip_addr_cmp_zoneless(ip1, ip2);
#endif
}

static int network_base_tx(network_ctrl_t *ctrl, const uint8_t *data, uint32_t len, int flags, luat_ip_addr_t *remote_ip, uint16_t remote_port)
{
    int result = -1;
    if (ctrl->is_tcp)
    {
        result = network_socket_send(ctrl, data, len, flags, NULL, 0);
    }
    else
    {
        if (remote_ip)
        {
            result = network_socket_send(ctrl, data, len, flags, remote_ip, remote_port);
        }
        else
        {
            result = network_socket_send(ctrl, data, len, flags, ctrl->online_ip, ctrl->remote_port);
        }
    }
    if (result >= 0)
    {
        ctrl->tx_size += len;
    }
    else
    {
        ctrl->need_close = 1;
    }
    return result;
}

static LUAT_RT_RET_TYPE tls_shorttimeout(LUAT_RT_CB_PARAM)
{
    network_ctrl_t *ctrl = (network_ctrl_t *)param;
    if (!ctrl->tls_mode)
    {
        platform_stop_timer(ctrl->tls_long_timer);
        return LUAT_RT_RET;
    }
    if (0 == ctrl->tls_timer_state)
    {
        ctrl->tls_timer_state = 1;
    }
    return LUAT_RT_RET;
}

static LUAT_RT_RET_TYPE tls_longtimeout(LUAT_RT_CB_PARAM)
{
    network_ctrl_t *ctrl = (network_ctrl_t *)param;
    platform_stop_timer(ctrl->tls_short_timer);
    if (!ctrl->tls_mode)
    {
        return LUAT_RT_RET;
    }
    ctrl->tls_timer_state = 2;
    return LUAT_RT_RET;
}

static void tls_settimer( void *data, uint32_t int_ms, uint32_t fin_ms )
{
    network_ctrl_t *ctrl = (network_ctrl_t *)data;
    if (!ctrl->tls_mode)
    {
        return;
    }
    if (!fin_ms)
    {
        platform_stop_timer(ctrl->tls_short_timer);
        platform_stop_timer(ctrl->tls_long_timer);
        ctrl->tls_timer_state = -1;
        return ;
    }
    platform_start_timer(ctrl->tls_short_timer, int_ms, 0);
    platform_start_timer(ctrl->tls_long_timer, fin_ms, 0);
    ctrl->tls_timer_state = 0;
}

static int tls_gettimer( void *data )
{
    network_ctrl_t *ctrl = (network_ctrl_t *)data;
    if (!ctrl->tls_mode)
    {
        return -ERROR_PARAM_INVALID;
    }
    #if MBEDTLS_VERSION_NUMBER >= 0x03000000
    if (!mbedtls_ssl_is_handshake_over(ctrl->ssl))
    #else
    if (ctrl->ssl->state != MBEDTLS_SSL_HANDSHAKE_OVER)
    #endif
    {
        return ctrl->tls_timer_state;
    }
    else
    {
        return 0;
    }
}

static void tls_dbg(void *data, int level,
        const char *file, int line,
        const char *str)
{
    (void)data;(void)level;DBG_Printf("%s %d:%s", file, line, str);
}

static int tls_send(void *ctx, const unsigned char *buf, size_t len )
{
    network_ctrl_t *ctrl = (network_ctrl_t *)ctx;
    if (!ctrl->tls_mode)
    {
        return -ERROR_PERMISSION_DENIED;
    }
    if (network_base_tx(ctrl, buf, len, 0, NULL, 0) != len)
    {
        return -0x004E;
    }
    else
    {
        return len;
    }
}

static int tls_recv(void *ctx, unsigned char *buf, size_t len )
{
#ifdef LUAT_USE_TLS
    network_ctrl_t *ctrl = (network_ctrl_t *)ctx;
    luat_ip_addr_t remote_ip;
    uint16_t remote_port;
    int result = -1;
    if (!ctrl->tls_mode)
    {
        return -1;
    }
TLS_RECV:

    result = network_socket_receive(ctrl, buf, len, 0, &remote_ip, &remote_port);
    if (result < 0)
    {
        return -0x004C;
    }
    if (result > 0)
    {
        if (!ctrl->is_tcp)
        {
            if ((remote_port == ctrl->remote_port) && network_check_ip_same(&remote_ip, ctrl->online_ip))
            {
                goto TLS_RECV;
            }
        }
        return result;
    }
    return MBEDTLS_ERR_SSL_WANT_READ;
#else
    return -1;
#endif
}

static int network_get_host_by_name(network_ctrl_t *ctrl)
{
    ctrl->remote_ip.type = 0xff;
    if (ipaddr_aton(ctrl->domain_name, &ctrl->remote_ip))
    {
        return 0;
    }
    ctrl->remote_ip.type = 0xff;
    return -1;
}

static void network_update_dns_cache(network_ctrl_t *ctrl)
{

}

static void network_get_dns_cache(network_ctrl_t *ctrl)
{

}

static int network_base_connect(network_ctrl_t *ctrl, luat_ip_addr_t *remote_ip)
{
    network_adapter_t *adapter = &prv_adapter_table[ctrl->adapter_index];
    if (ctrl->socket_id >= 0)
    {
    #ifdef LUAT_USE_TLS
        if (ctrl->tls_mode)
        {
            mbedtls_ssl_free(ctrl->ssl);
        }
    #endif
        if (network_socket_close(ctrl))
        {
            network_clean_invaild_socket(ctrl->adapter_index);
            network_socket_force_close(ctrl);
        }
        ctrl->need_close = 0;
        ctrl->socket_id = -1;
    }
    if (remote_ip)
    {
        if (network_create_soceket(ctrl, IPADDR_TYPE_V6 == remote_ip->type) < 0)
        {
            network_clean_invaild_socket(ctrl->adapter_index);
            if (network_create_soceket(ctrl, IPADDR_TYPE_V6 == remote_ip->type) < 0)
            {
                return -1;
            }
        }
        if (adapter->opt->is_posix)
        {
            volatile uint32_t val;
            val = ctrl->tcp_keep_alive;
            network_setsockopt(ctrl, SOL_SOCKET, SO_KEEPALIVE, (void *)&val, sizeof(val));
            if (ctrl->tcp_keep_alive)
            {
                val = ctrl->tcp_keep_idle;
                network_setsockopt(ctrl, IPPROTO_TCP, TCP_KEEPIDLE, (void*)&val, sizeof(val));
                val = ctrl->tcp_keep_interval;
                network_setsockopt(ctrl, IPPROTO_TCP, TCP_KEEPINTVL, (void *)&val, sizeof(val));
                val = ctrl->tcp_keep_cnt;
                network_setsockopt(ctrl, IPPROTO_TCP, TCP_KEEPCNT, (void *)&val, sizeof(val));
            }
        }
        else
        {
            network_user_cmd(ctrl, NW_CMD_AUTO_HEART_TIME, ctrl->tcp_keep_idle);
        }

        return network_socket_connect(ctrl, remote_ip);
    }
    else
    {
        if (network_create_soceket(ctrl, 0) < 0)
        {
            network_clean_invaild_socket(ctrl->adapter_index);
            if (network_create_soceket(ctrl, 0) < 0)
            {
                return -1;
            }
        }
        return network_socket_listen(ctrl);
    }
}

static int network_prepare_connect(network_ctrl_t *ctrl)
{
    if (ctrl->remote_ip.type != 0xff)
    {
        ;
    }
    else if (ctrl->domain_name)
    {
        if (network_get_host_by_name(ctrl))
        {
            if (network_dns(ctrl))
            {
                network_socket_force_close(ctrl);
                return -1;
            }
            ctrl->state = NW_STATE_WAIT_DNS;
            return 0;
        }
    }
    else
    {

        return -1;
    }

    if (network_base_connect(ctrl, &ctrl->remote_ip))
    {
        network_socket_force_close(ctrl);
        return -1;
    }
    ctrl->state = NW_STATE_CONNECTING;
    return 0;
}

static int network_state_link_off(network_ctrl_t *ctrl, OS_EVENT *event, network_adapter_t *adapter)
{
    if (EV_NW_STATE == event->ID)
    {
        if (event->Param2)
        {
            ctrl->state = NW_STATE_OFF_LINE;
            if (NW_WAIT_LINK_UP == ctrl->wait_target_state)
            {
                return 0;
            }
            else if (NW_WAIT_ON_LINE == ctrl->wait_target_state)
            {
                if (ctrl->is_server_mode)
                {
                    if (network_base_connect(ctrl, NULL))
                    {
                        return -1;
                    }
                    ctrl->state = NW_STATE_CONNECTING;
                }
                else
                {
                    if (network_prepare_connect(ctrl))
                    {
                        return -1;
                    }
                }

                return 1;
            }
        }
    }
    return 1;
}

static int network_state_off_line(network_ctrl_t *ctrl, OS_EVENT *event, network_adapter_t *adapter)
{
    return 1;
}

static int network_state_wait_dns(network_ctrl_t *ctrl, OS_EVENT *event, network_adapter_t *adapter)
{
    if ((ctrl->need_close) || ctrl->wait_target_state != NW_WAIT_ON_LINE) return -1;
    switch(event->ID)
    {
    case EV_NW_RESET:
    case EV_NW_SOCKET_ERROR:
        return -1;
    case EV_NW_STATE:
        if (!event->Param2)
        {
            return -1;
        }
        break;
    case EV_NW_DNS_RESULT:
        if (event->Param1)
        {
            //update dns cache
            ctrl->dns_ip = (luat_dns_ip_result*)event->Param2;
            ctrl->dns_ip_nums = event->Param1;
            for(int i = 0; i < ctrl->dns_ip_nums; i++)
            {
                DBG("dns ip%d, ttl %u, %s", i, ctrl->dns_ip[i].ttl_end, ipaddr_ntoa(&ctrl->dns_ip[i].ip));
            }
            network_update_dns_cache(ctrl);
        }
        else
        {
            ctrl->dns_ip_nums = 0;
            network_get_dns_cache(ctrl);
            if (!ctrl->dns_ip_nums)
            {
                return -1;
            }

        }
        if (!ctrl->remote_port)
        {
            ctrl->state = NW_STATE_OFF_LINE;
            return 0;
        }
        ctrl->dns_ip_cnt = 0;
        if (network_base_connect(ctrl, &ctrl->dns_ip[ctrl->dns_ip_cnt].ip))
        {
            network_socket_force_close(ctrl);
            return -1;
        }
        else
        {
            ctrl->state = NW_STATE_CONNECTING;
            return 1;
        }
    default:
        return 1;
    }
    return -1;
}

static int network_state_connecting(network_ctrl_t *ctrl, OS_EVENT *event, network_adapter_t *adapter)
{
    if ((ctrl->need_close) || (ctrl->wait_target_state != NW_WAIT_ON_LINE)) return -1;
    switch(event->ID)
    {
    case EV_NW_RESET:
        return -1;
    case EV_NW_SOCKET_ERROR:
    case EV_NW_SOCKET_REMOTE_CLOSE:
    case EV_NW_SOCKET_CLOSE_OK:
        if (ctrl->remote_ip.type != 0xff)
        {
            return -1;
        }
        DBG("dns ip %d no connect!,%d", ctrl->dns_ip_cnt, ctrl->dns_ip_nums);
        ctrl->dns_ip_cnt++;
        if (ctrl->dns_ip_cnt >= ctrl->dns_ip_nums)
        {
            DBG("all ip try connect failed");
            return -1;
        }
        DBG("try %d", ctrl->dns_ip_cnt);
        if (network_base_connect(ctrl, &ctrl->dns_ip[ctrl->dns_ip_cnt].ip))
        {
            network_socket_force_close(ctrl);
            return -1;
        }
        else
        {
            ctrl->state = NW_STATE_CONNECTING;
            return 1;
        }
        break;
    case EV_NW_STATE:
        if (!event->Param2)
        {
            return -1;
        }
        break;
    case EV_NW_SOCKET_LISTEN:
        if (ctrl->is_server_mode)
        {
            ctrl->state = NW_STATE_LISTEN;
            return 1;
        }
        break;
    case EV_NW_SOCKET_CONNECT_OK:
#ifdef LUAT_USE_TLS
        if (ctrl->tls_mode)
        {
            mbedtls_ssl_free(ctrl->ssl);
            memset(ctrl->ssl, 0, sizeof(mbedtls_ssl_context));
            mbedtls_ssl_setup(ctrl->ssl, ctrl->config);

            
            #if MBEDTLS_VERSION_NUMBER >= 0x03000000
            mbedtls_ssl_set_timer_cb(ctrl->ssl, ctrl, tls_settimer, tls_gettimer);
            #else
            ctrl->ssl->f_set_timer = tls_settimer;
            ctrl->ssl->f_get_timer = tls_gettimer;
            ctrl->ssl->p_timer = ctrl;
            #endif
                        
            #if MBEDTLS_VERSION_NUMBER >= 0x03000000
            mbedtls_ssl_set_bio(ctrl->ssl, ctrl, tls_send, tls_recv, NULL);
            #else
            ctrl->ssl->p_bio = ctrl;
            ctrl->ssl->f_send = tls_send;
            ctrl->ssl->f_recv = tls_recv;
            #endif

            mbedtls_ssl_set_timer( ctrl->ssl, 0 );
            // add by wendal
            //cloudflare's https needs to set hostname to access
            if (ctrl->domain_name_len > 0 && ctrl->domain_name_len < 256) {
                char host[257] = {0};
                memcpy(host, ctrl->domain_name, ctrl->domain_name_len);
                mbedtls_ssl_set_hostname(ctrl->ssl, host);
                //LLOGD("CALL mbedtls_ssl_set_hostname(%s)", host);
            }
            else {
                //LLOGD("skip mbedtls_ssl_set_hostname");
            }

            ctrl->state = NW_STATE_SHAKEHAND;
            do
            {
                int result = mbedtls_ssl_handshake_step( ctrl->ssl );
                switch(result)
                {
                case MBEDTLS_ERR_SSL_WANT_READ:
                    return 1;
                case 0:
                    break;
                default:
                    DBG_ERR("0x%x", -result);
                    return -1;
                }
            #if MBEDTLS_VERSION_NUMBER >= 0x03000000
            }while(!mbedtls_ssl_is_handshake_over(ctrl->ssl));
            #else
            }while(ctrl->ssl->state != MBEDTLS_SSL_HANDSHAKE_OVER);
            #endif
            
            return 0;
        }
        else
#endif
        {
            ctrl->state = NW_STATE_ONLINE;
            return 0;
        }

    default:
        return 1;
    }
    return -1;
}

static int network_state_shakehand(network_ctrl_t *ctrl, OS_EVENT *event, network_adapter_t *adapter)
{
    if ((ctrl->need_close) || ((ctrl->wait_target_state != NW_WAIT_ON_LINE) && (ctrl->wait_target_state != NW_WAIT_TX_OK))) return -1;
    switch(event->ID)
    {
    case EV_NW_RESET:
    case EV_NW_SOCKET_ERROR:
    case EV_NW_SOCKET_REMOTE_CLOSE:
    case EV_NW_SOCKET_CLOSE_OK:
        ctrl->need_close = 1;
        return -1;
    case EV_NW_STATE:
        if (!event->Param2)
        {
            ctrl->need_close = 1;
            return -1;
        }
        break;
    case EV_NW_SOCKET_TX_OK:
        ctrl->ack_size += event->Param2;
        break;
#ifdef LUAT_USE_TLS
    case EV_NW_SOCKET_RX_NEW:
        do
        {
            int result = mbedtls_ssl_handshake_step( ctrl->ssl );
            switch(result)
            {
            case MBEDTLS_ERR_SSL_WANT_READ:
                return 1;
            case 0:
                break;
            default:
                DBG_ERR("0x%x", -result);
                ctrl->need_close = 1;
                return -1;
            }
        #if MBEDTLS_VERSION_NUMBER >= 0x03000000
        }while(!mbedtls_ssl_is_handshake_over(ctrl->ssl));
        #else
        }while(ctrl->ssl->state != MBEDTLS_SSL_HANDSHAKE_OVER);
        #endif

        ctrl->state = NW_STATE_ONLINE;
        if (NW_WAIT_TX_OK == ctrl->wait_target_state)
        {
            if (!ctrl->cache_data)
            {
                ctrl->need_close = 1;
                return -1;
            }
            int result = mbedtls_ssl_write(ctrl->ssl, ctrl->cache_data, ctrl->cache_len);
            free(ctrl->cache_data);
            ctrl->cache_data = NULL;
            ctrl->cache_len = 0;
            if (result < 0)
            {
                DBG("%08x", -result);
                ctrl->need_close = 1;
                return -1;
            }
            return 1;
        }
        return 0;
#endif
    case EV_NW_SOCKET_CONNECT_OK:
        DBG_ERR("!");
        return 1;
    default:
        return 1;
    }
    return 1;
}

static int network_state_on_line(network_ctrl_t *ctrl, OS_EVENT *event, network_adapter_t *adapter)
{
    if ((ctrl->need_close) || NW_WAIT_OFF_LINE == ctrl->wait_target_state)
    {
        return -1;
    }

    switch(event->ID)
    {
    case EV_NW_RESET:
    case EV_NW_SOCKET_ERROR:
    case EV_NW_SOCKET_REMOTE_CLOSE:
    case EV_NW_SOCKET_CLOSE_OK:
        ctrl->need_close = 1;
        return -1;
    case EV_NW_STATE:
        if (!event->Param2)
        {
            ctrl->need_close = 1;
            return -1;
        }
        break;
    case EV_NW_SOCKET_TX_OK:
        ctrl->ack_size += event->Param2;
        if (NW_WAIT_TX_OK == ctrl->wait_target_state)
        {

            if (ctrl->ack_size == ctrl->tx_size)
            {
#ifdef LUAT_USE_LWIP
                if (ctrl->is_tcp)
                {
                    if (ctrl->adapter_index < NW_ADAPTER_INDEX_LWIP_NETIF_QTY)
                    {
                        return net_lwip_check_all_ack(ctrl->socket_id);
                    }
                    else
                    {
                        return 0;
                    }
                }
                return 0;
#else
                return 0;
#endif
            }
        }
        break;
    case EV_NW_SOCKET_RX_NEW:
#ifdef LUAT_USE_TLS
        #if MBEDTLS_VERSION_NUMBER >= 0x03000000
        if (ctrl->tls_mode && !mbedtls_ssl_is_handshake_over(ctrl->ssl))
        #else
        if (ctrl->tls_mode && (ctrl->ssl->state != MBEDTLS_SSL_HANDSHAKE_OVER))
        #endif
        {
            DBG("rehandshaking");
            do
            {
                int result = mbedtls_ssl_handshake_step( ctrl->ssl );
                switch(result)
                {
                case MBEDTLS_ERR_SSL_WANT_READ:
                    return 1;
                case 0:
                    break;
                default:
                    DBG_ERR("0x%x", -result);
                    ctrl->need_close = 1;
                    return -1;
                }
            #if MBEDTLS_VERSION_NUMBER >= 0x03000000
            }while(!mbedtls_ssl_is_handshake_over(ctrl->ssl));
            #else
            }while(ctrl->ssl->state != MBEDTLS_SSL_HANDSHAKE_OVER);
            #endif
        }
#endif
        ctrl->new_rx_flag = 1;
        if (NW_WAIT_TX_OK != ctrl->wait_target_state)
        {
            return 0;
        }
        break;
    default:
        return 1;
    }
    return 1;
}

static int network_state_listen(network_ctrl_t *ctrl, OS_EVENT *event, network_adapter_t *adapter)
{
    if ((ctrl->need_close) || NW_WAIT_OFF_LINE == ctrl->wait_target_state)
    {
        return -1;
    }
    switch(event->ID)
    {
    case EV_NW_RESET:
    case EV_NW_SOCKET_ERROR:
    case EV_NW_SOCKET_REMOTE_CLOSE:
    case EV_NW_SOCKET_CLOSE_OK:
        ctrl->need_close = 1;
        return -1;
    case EV_NW_STATE:
        if (!event->Param2)
        {
            ctrl->need_close = 1;
            return -1;
        }
        break;
    case EV_NW_SOCKET_NEW_CONNECT:
    case EV_NW_SOCKET_CONNECT_OK:
        ctrl->state = NW_STATE_ONLINE;
        return 0;
    default:
        return 1;
    }
    return 1;
}

static int network_state_disconnecting(network_ctrl_t *ctrl, OS_EVENT *event, network_adapter_t *adapter)
{
    if (ctrl->wait_target_state != NW_WAIT_OFF_LINE)
    {
        return -1;
    }
    switch(event->ID)
    {
    case EV_NW_RESET:
    case EV_NW_SOCKET_ERROR:
    case EV_NW_SOCKET_REMOTE_CLOSE:
    case EV_NW_SOCKET_CLOSE_OK:
        network_force_close_socket(ctrl);
        ctrl->state = NW_STATE_OFF_LINE;
        ctrl->socket_id = -1;
        return 0;
    case EV_NW_STATE:
        if (!event->Param2)
        {
            return -1;
        }
        else
        {
            network_force_close_socket(ctrl);
            ctrl->state = NW_STATE_OFF_LINE;
            ctrl->socket_id = -1;
        }
        break;
    default:
        return 1;
    }
    return -1;
}

typedef int (*network_state_fun)(network_ctrl_t *ctrl, OS_EVENT *event, network_adapter_t *adapter);
static network_state_fun network_state_fun_list[]=
{
        network_state_link_off,
        network_state_off_line,
        network_state_wait_dns,
        network_state_connecting,
        network_state_shakehand,
        network_state_on_line,
        network_state_listen,
        network_state_disconnecting,
};

static void network_default_statemachine(network_ctrl_t *ctrl, OS_EVENT *event, network_adapter_t *adapter)
{
    int result = -1;
    uint8_t close_flag = 0;
    NW_LOCK;
    if (ctrl->state > NW_STATE_DISCONNECTING)
    {
        ctrl->state = NW_STATE_LINK_OFF;
        event->Param1 = -1;

        network_force_close_socket(ctrl);
        event->ID = ctrl->wait_target_state + EV_NW_RESULT_BASE;

    }
    else
    {
        if ((NW_STATE_DISCONNECTING == ctrl->state) && (NW_WAIT_OFF_LINE == ctrl->wait_target_state))
        {
            close_flag = 1;
        }
        result = network_state_fun_list[ctrl->state](ctrl, event, adapter);
        if (result > 0)
        {
            NW_UNLOCK;
            if (ctrl->new_rx_flag && ctrl->user_callback)
            {
                event->ID = NW_WAIT_EVENT + EV_NW_RESULT_BASE;
                event->Param1 = 0;
                ctrl->user_callback(event, ctrl->user_data);
            }
            return ;
        }
        if (close_flag && (NW_WAIT_NONE == ctrl->wait_target_state))
        {
            event->ID = EV_NW_RESULT_CLOSE;
        }
        else
        {
            event->ID = (ctrl->wait_target_state?ctrl->wait_target_state:NW_WAIT_EVENT) + EV_NW_RESULT_BASE;
        }
        event->Param1 = result;
    }
    if ((ctrl->state != NW_STATE_LISTEN) || (result < 0))
    {
        ctrl->wait_target_state = NW_WAIT_NONE;
    }
    NW_UNLOCK;
    if (ctrl->task_handle)
    {
        platform_send_event(ctrl->task_handle, event->ID, event->Param1, event->Param2, event->Param3);
    }
    else if (ctrl->user_callback)
    {
        ctrl->user_callback(event, ctrl->user_data);
    }
}


static int32_t network_default_socket_callback(void *data, void *param)
{
    OS_EVENT *event = (OS_EVENT *)data;
    OS_EVENT temp_event;
    luat_network_cb_param_t *cb_param = (luat_network_cb_param_t *)param;
    network_adapter_t *adapter =(network_adapter_t *)(cb_param->param);
    int i;
    network_ctrl_t *ctrl = (network_ctrl_t *)event->Param3;

    if (event->ID > EV_NW_TIMEOUT)
    {
        if (ctrl && ((ctrl->tag == cb_param->tag) || (event->ID == EV_NW_DNS_RESULT)))
        {
            if ((event->ID == EV_NW_DNS_RESULT) && (ctrl->wait_target_state != NW_WAIT_ON_LINE))
            {
                DBG("socket event:%s,wait:%s", network_ctrl_event_id_string(event->ID), network_ctrl_wait_state_string(ctrl->wait_target_state));
                return 0;
            }
            if (ctrl->auto_mode)
            {
                DBG("socket %d,event:%s,state:%s,wait:%s", ctrl->socket_id, network_ctrl_event_id_string(event->ID),
                        network_ctrl_state_string(ctrl->state),
                        network_ctrl_wait_state_string(ctrl->wait_target_state));
                network_default_statemachine(ctrl, event, adapter);
                DBG("socket %d,state:%s,wait:%s", ctrl->socket_id, network_ctrl_state_string(ctrl->state),network_ctrl_wait_state_string(ctrl->wait_target_state));
            }
            else if (ctrl->task_handle)
            {
                platform_send_event(ctrl->task_handle, event->ID, event->Param1, event->Param2, event->Param3);
            }
            else if (ctrl->user_callback)
            {
                ctrl->user_callback(event, ctrl->user_data);
            }
        }
        else
        {
            DBG_ERR("cb ctrl invaild %x %08X", ctrl, event->ID);
            DBG_HexPrintf(&ctrl->tag, 8);
            DBG_HexPrintf(&cb_param->tag, 8);
        }
    }
    else
    {
        for (i = 0; i < adapter->opt->max_socket_num; i++)
        {
            temp_event = *event;
            if (adapter->ctrl_busy[i])
            {
                ctrl = &adapter->ctrl_table[i];
                if (ctrl->adapter_index == (uint8_t)(event->Param3))
                {
                    if (ctrl->auto_mode)
                    {
                        DBG("socket %d,event:%s,state:%s,wait:%s", ctrl->socket_id, network_ctrl_event_id_string(event->ID),
                                network_ctrl_state_string(ctrl->state),
                                network_ctrl_wait_state_string(ctrl->wait_target_state));
                        network_default_statemachine(ctrl, &temp_event, adapter);
                        DBG("socket %d,event:%s,wait:%s", ctrl->socket_id, network_ctrl_state_string(ctrl->state),  network_ctrl_wait_state_string(ctrl->wait_target_state));
                    }
                    else if (ctrl->task_handle)
                    {
                        platform_send_event(ctrl->task_handle, event->ID, event->Param1, event->Param2, event->Param3);
                    }
                    else if (ctrl->user_callback)
                    {
                        ctrl->user_callback(&temp_event, ctrl->user_data);
                    }
                }

            }
        }
    }

    return 0;
}

static LUAT_RT_RET_TYPE network_default_timer_callback(LUAT_RT_CB_PARAM)
{
    platform_send_event(param, EV_NW_TIMEOUT, 0, 0, 0);
    return LUAT_RT_RET;
}

int network_get_last_register_adapter(void)
{
    if (prv_network.default_adapter_index != -1) return prv_network.default_adapter_index;
    return prv_network.last_adapter_index;
}

void network_register_set_default(uint8_t adapter_index)
{
    prv_network.default_adapter_index = adapter_index;
}

int network_register_adapter(uint8_t adapter_index, network_adapter_info *info, void *user_data)
{
    prv_adapter_table[adapter_index].opt = info;
    prv_adapter_table[adapter_index].user_data = user_data;
    info->socket_set_callback(network_default_socket_callback, &prv_adapter_table[adapter_index], user_data);
    if (adapter_index < NW_ADAPTER_INDEX_HW_PS_DEVICE)
    {
        prv_adapter_table[adapter_index].ctrl_table = prv_network.lwip_ctrl_table;
        prv_adapter_table[adapter_index].ctrl_busy = prv_network.lwip_ctrl_busy;
    }
    else
    {
        prv_adapter_table[adapter_index].ctrl_table = (network_ctrl_t *)zalloc((info->max_socket_num) * sizeof(network_ctrl_t));
        prv_adapter_table[adapter_index].ctrl_busy = (uint8_t *)zalloc(info->max_socket_num);
    }

    prv_adapter_table[adapter_index].port = 60000;
    if (!prv_network.is_init)
    {
        //prv_network.network_mutex = platform_create_mutex();
        INIT_LLIST_HEAD(&prv_network.dns_cache_head);
        prv_network.is_init = 0;
    }

    prv_network.last_adapter_index = adapter_index;
    return 0;
}

void network_set_dns_server(uint8_t adapter_index, uint8_t server_index, luat_ip_addr_t *ip)
{
    network_adapter_t *adapter = &prv_adapter_table[adapter_index];
    adapter->opt->set_dns_server(server_index, ip, adapter->user_data);
}

/** Apply for a network_ctrl*/
network_ctrl_t *network_alloc_ctrl(uint8_t adapter_index)
{
    int i;
    network_ctrl_t *ctrl = NULL;
    network_adapter_t *adapter = &prv_adapter_table[adapter_index];
    OS_LOCK;
    for (i = 0; i < adapter->opt->max_socket_num; i++)
    {
        if (!adapter->ctrl_busy[i])
        {

            adapter->ctrl_busy[i] = 1;
            ctrl = &adapter->ctrl_table[i];
            ctrl->adapter_index = adapter_index;
            ctrl->domain_ipv6 = 0;
            break;
        }
    }
    OS_UNLOCK;
    if (i >= adapter->opt->max_socket_num) {DBG_ERR("adapter no more ctrl!");}
    return ctrl;
}

/** Return a network_ctrl*/
void network_release_ctrl(network_ctrl_t *ctrl)
{
    int i;
    network_adapter_t *adapter = &prv_adapter_table[ctrl->adapter_index];
    NW_UNLOCK;
    OS_LOCK;
    for (i = 0; i < adapter->opt->max_socket_num; i++)
    {
        if (&adapter->ctrl_table[i] == ctrl)
        {
            network_deinit_tls(ctrl);
            if (ctrl->timer)
            {
                platform_stop_timer(ctrl->timer);
                platform_release_timer(ctrl->timer);
                ctrl->timer = NULL;
            }
            if (ctrl->cache_data)
            {
                free(ctrl->cache_data);
                ctrl->cache_data = NULL;
            }
            if (ctrl->dns_ip)
            {
                free(ctrl->dns_ip);
                ctrl->dns_ip = NULL;
            }
            if (ctrl->domain_name)
            {
                free(ctrl->domain_name);
                ctrl->domain_name = NULL;
            }
            adapter->ctrl_busy[i] = 0;
            platform_release_mutex(ctrl->mutex);
            ctrl->mutex = NULL;
            break;
        }
    }
    OS_UNLOCK;
    if (i >= adapter->opt->max_socket_num) {DBG_ERR("adapter index maybe error!, %d, %x", ctrl->adapter_index, ctrl);}

}

void network_init_ctrl(network_ctrl_t *ctrl, HANDLE task_handle, CBFuncEx_t callback, void *param)
{
    uint8_t adapter_index = ctrl->adapter_index;
    network_adapter_t *adapter = &prv_adapter_table[ctrl->adapter_index];
    if (ctrl->dns_ip)
    {
        free(ctrl->dns_ip);
    }
    if (ctrl->cache_data)
    {
        free(ctrl->cache_data);
    }
    if (ctrl->domain_name)
    {
        free(ctrl->domain_name);
    }
    HANDLE sem = ctrl->mutex;
    memset(ctrl, 0, sizeof(network_ctrl_t));
    ctrl->adapter_index = adapter_index;
    ctrl->task_handle = task_handle;
    ctrl->user_callback = callback;
    ctrl->user_data = param;
    ctrl->socket_id = -1;
    ctrl->socket_param = ctrl;
    ctrl->remote_ip.type = 0xff;
    ctrl->mutex = sem;
    if (task_handle)
    {
        ctrl->timer = platform_create_timer(network_default_timer_callback, task_handle, NULL);
    }
    if (!ctrl->mutex)
    {
        ctrl->mutex = platform_create_mutex();
    }

}

void network_set_base_mode(network_ctrl_t *ctrl, uint8_t is_tcp, uint32_t tcp_timeout_ms, uint8_t keep_alive, uint32_t keep_idle, uint8_t keep_interval, uint8_t keep_cnt)
{
    ctrl->is_tcp = is_tcp;
    ctrl->tcp_keep_alive = keep_alive;
    ctrl->tcp_keep_idle = keep_idle;
    ctrl->tcp_keep_interval = keep_interval;
    ctrl->tcp_keep_cnt = keep_cnt;
    ctrl->tcp_timeout_ms = tcp_timeout_ms;
}

void network_connect_ipv6_domain(network_ctrl_t *ctrl, uint8_t onoff)
{
    ctrl->domain_ipv6 = onoff;
}

int network_set_local_port(network_ctrl_t *ctrl, uint16_t local_port)
{
    int i;
    network_adapter_t *adapter = &prv_adapter_table[ctrl->adapter_index];
    if (local_port)
    {
        OS_LOCK;
        for (i = 0; i < adapter->opt->max_socket_num; i++)
        {
            if (&adapter->ctrl_table[i] != ctrl)
            {
                if (adapter->ctrl_table[i].local_port == local_port)
                {
                    OS_UNLOCK;
                    return -1;
                }
            }

        }
        ctrl->local_port = local_port;
        OS_UNLOCK;
        return 0;
    }
    else
    {
        ctrl->local_port = 0;
#if 0
        if (ctrl->adapter_index < NW_ADAPTER_INDEX_LWIP_NETIF_QTY)
        {
            ctrl->local_port = 0;
            return 0;
        }
        OS_LOCK;
        adapter->port++;
        if (adapter->port < 60000)
        {
            adapter->port = 60000;
        }
        ctrl->local_port = adapter->port;
        OS_UNLOCK;
#endif
        return 0;
    }
}

int network_create_soceket(network_ctrl_t *ctrl, uint8_t is_ipv6)
{
    network_adapter_t *adapter = &prv_adapter_table[ctrl->adapter_index];
    ctrl->socket_id = adapter->opt->create_soceket(ctrl->is_tcp, &ctrl->tag, ctrl->socket_param, is_ipv6, adapter->user_data);
    if (ctrl->socket_id < 0)
    {
        ctrl->tag = 0;
        return -1;
    }
    return 0;
}

int network_socket_connect(network_ctrl_t *ctrl, luat_ip_addr_t *remote_ip)
{
    network_adapter_t *adapter = &prv_adapter_table[ctrl->adapter_index];
    ctrl->is_server_mode = 0;
    ctrl->online_ip = remote_ip;
    uint16_t local_port = ctrl->local_port;
    if (!local_port)
    {
        if (ctrl->adapter_index >= NW_ADAPTER_INDEX_HW_PS_DEVICE)
        {
            adapter->port += 100;
            local_port = adapter->port;
            if (adapter->port < 60000)
            {
                adapter->port = 60000;
            }
            if (local_port < 60000)
            {
                local_port = 60000;
            }
            local_port += ctrl->socket_id;
            DBG("%d,%d,%d", ctrl->socket_id, local_port, adapter->port);
        }
    }
    return adapter->opt->socket_connect(ctrl->socket_id, ctrl->tag, local_port, remote_ip, ctrl->remote_port, adapter->user_data);
}

int network_socket_listen(network_ctrl_t *ctrl)
{
    network_adapter_t *adapter = &prv_adapter_table[ctrl->adapter_index];
    ctrl->is_server_mode = 1;
    return adapter->opt->socket_listen(ctrl->socket_id, ctrl->tag, ctrl->local_port, adapter->user_data);
}

uint8_t network_accept_enable(network_ctrl_t *ctrl)
{
    network_adapter_t *adapter = &prv_adapter_table[ctrl->adapter_index];
    return !(adapter->opt->no_accept);
}
//Accept a client as a server
//Return 0 on success, <0 on failure
int network_socket_accept(network_ctrl_t *ctrl, network_ctrl_t *accept_ctrl)
{
    network_adapter_t *adapter = &prv_adapter_table[ctrl->adapter_index];
    if (adapter->opt->no_accept)
    {
//      DBG("%x,%d,%llu,%x,%x,%x",adapter->opt->socket_accept, ctrl->socket_id, ctrl->tag, &ctrl->remote_ip, &ctrl->remote_port, adapter->user_data);
        adapter->opt->socket_accept(ctrl->socket_id, ctrl->tag, &ctrl->remote_ip, &ctrl->remote_port, adapter->user_data);
        return 0;
    }
    accept_ctrl->socket_id = adapter->opt->socket_accept(ctrl->socket_id, ctrl->tag, &accept_ctrl->remote_ip, &accept_ctrl->remote_port, adapter->user_data);
    if (accept_ctrl->socket_id < 0)
    {
        return -1;
    }
    else
    {
        accept_ctrl->is_tcp = ctrl->is_tcp;
        accept_ctrl->tcp_keep_alive = ctrl->tcp_keep_alive;
        accept_ctrl->tcp_keep_idle = ctrl->tcp_keep_idle;
        accept_ctrl->tcp_keep_interval = ctrl->tcp_keep_interval;
        accept_ctrl->tcp_keep_cnt = ctrl->tcp_keep_cnt;
        accept_ctrl->tcp_timeout_ms = ctrl->tcp_timeout_ms;
        accept_ctrl->local_port = ctrl->local_port;
        accept_ctrl->state = NW_STATE_ONLINE;
        return 0;
    }
}
//To proactively disconnect a tcp connection, the entire tcp process needs to be completed. The user needs to receive the close ok callback to confirm the complete disconnection.
//Return 0 on success, <0 on failure
int network_socket_disconnect(network_ctrl_t *ctrl)
{
    network_adapter_t *adapter = &prv_adapter_table[ctrl->adapter_index];
    if (ctrl->socket_id >= 0)
    {
        return adapter->opt->socket_disconnect(ctrl->socket_id, ctrl->tag, adapter->user_data);
    }
    return 0;
}
//Release control of the socket
//Return 0 on success, <0 on failure
int network_socket_close(network_ctrl_t *ctrl)
{
    network_adapter_t *adapter = &prv_adapter_table[ctrl->adapter_index];
    if (ctrl->socket_id >= 0)
    {
        return adapter->opt->socket_close(ctrl->socket_id, ctrl->tag, adapter->user_data);
    }
    return 0;
}
//Forcibly release control of the socket
//Return 0 on success, <0 on failure
int network_socket_force_close(network_ctrl_t *ctrl)
{
    network_adapter_t *adapter = &prv_adapter_table[ctrl->adapter_index];
    if (ctrl->socket_id >= 0)
    {
        adapter->opt->socket_force_close(ctrl->socket_id, adapter->user_data);
    }
    ctrl->socket_id = -1;
    return 0;
}
//When tcp is used, remote_ip and remote_port are not needed. If buf is NULL, the amount of data in the current buffer is returned. When the return value is less than len, it means that the reading has been completed.
//When udp is used, only 1 block of data is returned, and it needs to be read multiple times until there is no data.
//Return the actual read value on success, <0 on failure
int network_socket_receive(network_ctrl_t *ctrl,uint8_t *buf, uint32_t len, int flags, luat_ip_addr_t *remote_ip, uint16_t *remote_port)
{
    network_adapter_t *adapter = &prv_adapter_table[ctrl->adapter_index];
    return adapter->opt->socket_receive(ctrl->socket_id, ctrl->tag, buf, len, flags, remote_ip, remote_port, adapter->user_data);
}
//In tcp, remote_ip and remote_port are not required
//Return 0 on success, <0 on failure
int network_socket_send(network_ctrl_t *ctrl,const uint8_t *buf, uint32_t len, int flags, luat_ip_addr_t *remote_ip, uint16_t remote_port)
{
    network_adapter_t *adapter = &prv_adapter_table[ctrl->adapter_index];
    return adapter->opt->socket_send(ctrl->socket_id, ctrl->tag, buf, len, flags, remote_ip, remote_port, adapter->user_data);
}

int network_getsockopt(network_ctrl_t *ctrl, int level, int optname, void *optval, uint32_t *optlen)
{
    network_adapter_t *adapter = &prv_adapter_table[ctrl->adapter_index];
    return adapter->opt->getsockopt(ctrl->socket_id, ctrl->tag, level, optname, optval, optlen, adapter->user_data);
}
int network_setsockopt(network_ctrl_t *ctrl, int level, int optname, const void *optval, uint32_t optlen)
{
    network_adapter_t *adapter = &prv_adapter_table[ctrl->adapter_index];
    return adapter->opt->setsockopt(ctrl->socket_id, ctrl->tag, level, optname, optval, optlen, adapter->user_data);
}
//Non-posix socket, use this to set parameters according to actual hardware
int network_user_cmd(network_ctrl_t *ctrl,  uint32_t cmd, uint32_t value)
{
    network_adapter_t *adapter = &prv_adapter_table[ctrl->adapter_index];
    return adapter->opt->user_cmd(ctrl->socket_id, ctrl->tag, cmd, value, adapter->user_data);
}

int network_dns(network_ctrl_t *ctrl)
{
    network_adapter_t *adapter = &prv_adapter_table[ctrl->adapter_index];
    if (ctrl->domain_ipv6)
    {
        return adapter->opt->dns_ipv6(ctrl->domain_name, ctrl->domain_name_len, ctrl, adapter->user_data);
    }
    else
    {
        return adapter->opt->dns(ctrl->domain_name, ctrl->domain_name_len, ctrl, adapter->user_data);
    }
}

int network_set_mac(uint8_t adapter_index, uint8_t *mac)
{
    network_adapter_t *adapter = &prv_adapter_table[adapter_index];
    return adapter->opt->set_mac(mac, adapter->user_data);
}

int network_set_static_ip_info(uint8_t adapter_index, luat_ip_addr_t *ip, luat_ip_addr_t *submask, luat_ip_addr_t *gateway, luat_ip_addr_t *ipv6)
{
    network_adapter_t *adapter = &prv_adapter_table[adapter_index];
    return adapter->opt->set_static_ip(ip, submask, gateway, ipv6, adapter->user_data);
}

int network_get_local_ip_info(network_ctrl_t *ctrl, luat_ip_addr_t *ip, luat_ip_addr_t *submask, luat_ip_addr_t *gateway)
{
    network_adapter_t *adapter = &prv_adapter_table[ctrl->adapter_index];
    return adapter->opt->get_local_ip_info(ip, submask, gateway, adapter->user_data);
}

void network_force_close_socket(network_ctrl_t *ctrl)
{
#ifdef LUAT_USE_TLS
    if (ctrl->tls_mode)
    {
        mbedtls_ssl_free(ctrl->ssl);
    }
#endif
    if (network_socket_close(ctrl))
    {
        network_clean_invaild_socket(ctrl->adapter_index);
        network_socket_force_close(ctrl);
    }
    ctrl->need_close = 0;
    ctrl->socket_id = -1;
    ctrl->new_rx_flag = 0;
    if (ctrl->dns_ip)
    {
        free(ctrl->dns_ip);
        ctrl->dns_ip = NULL;
    }
    if (ctrl->domain_name)
    {
        free(ctrl->domain_name);
        ctrl->domain_name = NULL;
    }
    ctrl->dns_ip_cnt = 0;
    ctrl->dns_ip_nums = 0;
    ctrl->wait_target_state = NW_WAIT_NONE;
}

void network_clean_invaild_socket(uint8_t adapter_index)
{
    int i;
    int *list;
    network_adapter_t *adapter = &prv_adapter_table[adapter_index];
    network_ctrl_t *ctrl;
    list = malloc(adapter->opt->max_socket_num * sizeof(int));
    OS_LOCK;
    for (i = 0; i < adapter->opt->max_socket_num; i++)
    {
        ctrl = &adapter->ctrl_table[i];
        if (!adapter->opt->socket_check(ctrl->socket_id, ctrl->tag, adapter->user_data))
        {
            list[i] = ctrl->socket_id;
        }
        else
        {
            ctrl->socket_id = -1;
            list[i] = -1;
        }
        DBG("%d,%d", i, list[i]);
    }
    OS_UNLOCK;
    adapter->opt->socket_clean(list, adapter->opt->max_socket_num, adapter->user_data);
    free(list);
}


#ifdef LUAT_USE_TLS
static int tls_verify(void *ctx, mbedtls_x509_crt *crt, int Index, uint32_t *result)
{
    network_ctrl_t *ctrl = (network_ctrl_t *)ctx;
    DBG("%d, %08x", Index, *result);
    return 0;
}
#endif
int network_set_psk_info(network_ctrl_t *ctrl,
        const unsigned char *psk, size_t psk_len,
        const unsigned char *psk_identity, size_t psk_identity_len)
{
#ifdef LUAT_USE_TLS
    if (!ctrl->tls_mode)
    {
        return -ERROR_PERMISSION_DENIED;
    }

//  DBG("%.*s, %.*s", psk_len, psk, psk_identity_len, psk_identity);
    int ret = mbedtls_ssl_conf_psk( ctrl->config,
            psk, psk_len, psk_identity, psk_identity_len );
    if (ret != 0)
    {
        DBG("0x%x", -ret);
        return -ERROR_OPERATION_FAILED;
    }
    return ERROR_NONE;
#else
    return -1;
#endif
}

int network_set_server_cert(network_ctrl_t *ctrl, const unsigned char *cert, size_t cert_len)
{
#ifdef LUAT_USE_TLS
    int ret;
    if (!ctrl->tls_mode)
    {
        return -ERROR_PERMISSION_DENIED;
    }
    ret = mbedtls_x509_crt_parse( ctrl->ca_cert, cert, cert_len);
    if (ret != 0)
    {
        DBG("%08x", -ret);
        return -ERROR_OPERATION_FAILED;
    }

    return ERROR_NONE;
#else
    return -1;
#endif
}

#if MBEDTLS_VERSION_NUMBER >= 0x03000000
static int tls_random( void *p_rng, unsigned char *output, size_t output_len);
#endif

int network_set_client_cert(network_ctrl_t *ctrl,
        const unsigned char *cert, size_t certLen,
        const unsigned char *key, size_t keylen,
        const unsigned char *pwd, size_t pwdlen)
{
#ifdef LUAT_USE_TLS
    int ret;
    mbedtls_x509_crt *client_cert = NULL;
    mbedtls_pk_context *pkey = NULL;
    if (!ctrl->tls_mode)
    {
        return -ERROR_PERMISSION_DENIED;
    }
    client_cert = (mbedtls_x509_crt *)zalloc(sizeof(mbedtls_x509_crt));
    pkey = (mbedtls_pk_context *)zalloc(sizeof(mbedtls_pk_context));
    if (!client_cert || !pkey)
    {
        goto ERROR_OUT;
    }
    ret = mbedtls_x509_crt_parse( client_cert, cert, certLen );
    if (ret != 0)
    {
        DBG("%08x", -ret);
        goto ERROR_OUT;
    }
#if MBEDTLS_VERSION_NUMBER >= 0x03000000
    ret = mbedtls_pk_parse_key( pkey, key, keylen, pwd, pwdlen, tls_random, NULL);
#else
    ret = mbedtls_pk_parse_key( pkey, key, keylen, pwd, pwdlen);
#endif


    if (ret != 0)
    {
        DBG("%08x", -ret);
        goto ERROR_OUT;
    }
    ret = mbedtls_ssl_conf_own_cert( ctrl->config, client_cert, pkey );
    if (ret != 0)
    {
        DBG("%08x", -ret);
        goto ERROR_OUT;
    }
    return ERROR_NONE;
ERROR_OUT:
    if (client_cert) free(client_cert);
    if (pkey) free(pkey);
    return -1;
#else
    return -1;
#endif
}

int network_cert_verify_result(network_ctrl_t *ctrl)
{
#ifdef LUAT_USE_TLS
    if (!ctrl->tls_mode)
    {
        return -1;
    }
    return mbedtls_ssl_get_verify_result(ctrl->ssl);
#else
    return -1;
#endif
}

static int tls_random( void *p_rng,
        unsigned char *output, size_t output_len )
{
    platform_random((char*)output, output_len);
    return 0;
}

int network_init_tls(network_ctrl_t *ctrl, int verify_mode)
{
#ifdef LUAT_USE_TLS
    ctrl->tls_mode = 1;
    if (!ctrl->ssl)
    {
        ctrl->ssl = (mbedtls_ssl_context *)zalloc(sizeof(mbedtls_ssl_context));
        ctrl->ca_cert = (mbedtls_x509_crt *)zalloc(sizeof(mbedtls_x509_crt));
        ctrl->config = (mbedtls_ssl_config *)zalloc(sizeof(mbedtls_ssl_config));
        mbedtls_ssl_config_defaults( ctrl->config, MBEDTLS_SSL_IS_CLIENT, ctrl->is_tcp?MBEDTLS_SSL_TRANSPORT_STREAM:MBEDTLS_SSL_TRANSPORT_DATAGRAM, MBEDTLS_SSL_PRESET_DEFAULT);
        #if MBEDTLS_VERSION_NUMBER >= 0x03000000
        mbedtls_ssl_conf_authmode(ctrl->config, verify_mode);
        #else
        ctrl->config->authmode = verify_mode;
        #endif

        #ifdef MBEDTLS_SSL_PROTO_DTLS
        ctrl->config->hs_timeout_min = 20000;
        #endif

        #if MBEDTLS_VERSION_NUMBER >= 0x03000000
        mbedtls_ssl_conf_rng(ctrl->config, tls_random, NULL);
        mbedtls_ssl_conf_dbg(ctrl->config, tls_dbg, NULL);
        mbedtls_ssl_conf_verify(ctrl->config, tls_verify, ctrl);
        mbedtls_ssl_conf_ca_chain(ctrl->config, ctrl->ca_cert, NULL);
        mbedtls_ssl_conf_read_timeout(ctrl->config, 20000);
        mbedtls_ssl_conf_legacy_renegotiation(ctrl->config, MBEDTLS_SSL_LEGACY_ALLOW_RENEGOTIATION);
        mbedtls_ssl_conf_renegotiation(ctrl->config, MBEDTLS_SSL_RENEGOTIATION_ENABLED);
        #else
        ctrl->config->f_rng = tls_random;
        ctrl->config->p_rng = NULL;
        ctrl->config->f_dbg = tls_dbg;
        ctrl->config->p_dbg = NULL;
        ctrl->config->f_vrfy = tls_verify;
        ctrl->config->p_vrfy = ctrl;
        ctrl->config->ca_chain = ctrl->ca_cert;
        ctrl->config->read_timeout = 20000;
        ctrl->config->allow_legacy_renegotiation = MBEDTLS_SSL_LEGACY_ALLOW_RENEGOTIATION;
        ctrl->config->disable_renegotiation = MBEDTLS_SSL_RENEGOTIATION_ENABLED;
        #endif
        
        ctrl->tls_long_timer = platform_create_timer(tls_longtimeout, ctrl, NULL);
        ctrl->tls_short_timer = platform_create_timer(tls_shorttimeout, ctrl, NULL);
    }
    ctrl->tls_timer_state = -1;
    return 0;
#else
    DBG("NOT SUPPORT TLS");
    return -1;
#endif
}

void network_deinit_tls(network_ctrl_t *ctrl)
{
#ifdef LUAT_USE_TLS
    if (ctrl->ssl)
    {
        mbedtls_ssl_free(ctrl->ssl);
        free(ctrl->ssl);
        ctrl->ssl = NULL;
    }

    if (ctrl->config)
    {
        mbedtls_ssl_config_free(ctrl->config);
        free(ctrl->config);
        ctrl->config = NULL;
    }

    if (ctrl->ca_cert)
    {
        mbedtls_x509_crt_free(ctrl->ca_cert);
        free(ctrl->ca_cert);
        ctrl->ca_cert = NULL;
    }

    ctrl->tls_mode = 0;
    ctrl->tls_timer_state = -1;
    if (ctrl->tls_short_timer)
    {
        platform_release_timer(ctrl->tls_short_timer);
        ctrl->tls_short_timer = NULL;
    }
    if (ctrl->tls_long_timer)
    {
        platform_release_timer(ctrl->tls_long_timer);
        ctrl->tls_long_timer = NULL;
    }
#endif
}

int network_wait_link_up(network_ctrl_t *ctrl, uint32_t timeout_ms)
{
    NW_LOCK;
    ctrl->auto_mode = 1;
//  network_adapter_t *adapter = &prv_adapter_table[ctrl->adapter_index];
    if (network_check_ready(ctrl, 0))
    {
        ctrl->state = NW_STATE_OFF_LINE;
        ctrl->wait_target_state = NW_WAIT_NONE;
        NW_UNLOCK;
        return 0;
    }
    ctrl->state = NW_STATE_LINK_OFF;
    ctrl->wait_target_state = NW_WAIT_LINK_UP;

    NW_UNLOCK;
    if (!ctrl->task_handle || !timeout_ms)
    {
        return 1;
    }
    uint8_t finish = 0;
    OS_EVENT event;
    int result;
    //DBG_INFO("%s wait for active!,%u,%x", Net->Tag, To * SYS_TICK, Net->hTask);

    platform_start_timer(ctrl->timer, timeout_ms, 0);
    while (!finish)
    {
        platform_wait_event(ctrl->task_handle, 0, (luat_event_t *)&event, NULL, 0);
        switch (event.ID)
        {
        case EV_NW_RESULT_LINK:
            result = (int)event.Param1;
            finish = 1;
            break;
        case EV_NW_TIMEOUT:
            result = -1;
            finish = 1;
            break;
        default:
            if (ctrl->user_callback)
            {
                ctrl->user_callback((void *)&event, ctrl->user_data);
            }
            break;
        }
    }
    platform_stop_timer(ctrl->timer);
    return result;
}
/** 1. Perform ready detection and wait for ready
 * 2. If there is remote_ip, start connecting to the server and wait for the connection result.
 * 3. If there is no remote_ip, start dns parsing of the url. After the parsing is completed, try to connect to all ips until one succeeds or all fail.
 * 4. If it is in encryption mode, a handshake step is still required, and the result can be returned only after the handshake step is completed.
 * If local_port is 0, the API will automatically generate one internally.*/
int network_connect(network_ctrl_t *ctrl, const char *domain_name, uint32_t domain_name_len, luat_ip_addr_t *remote_ip, uint16_t remote_port, uint32_t timeout_ms)
{
    if (ctrl->socket_id >= 0)
    {
        return -1;
    }

    NW_LOCK;
    ctrl->is_server_mode = 0;
    ctrl->tx_size = 0;
    ctrl->ack_size = 0;
    if (ctrl->dns_ip)
    {
        free(ctrl->dns_ip);
        ctrl->dns_ip = NULL;
    }
    if (ctrl->cache_data)
    {
        free(ctrl->cache_data);
        ctrl->cache_data = NULL;
    }
    ctrl->need_close = 0;
    if (ctrl->domain_name)
    {
        free(ctrl->domain_name);
    }
    ctrl->domain_name = (char *)zalloc(domain_name_len + 1);
    memcpy(ctrl->domain_name, domain_name, domain_name_len);
    ctrl->domain_name_len = domain_name_len;
    if (remote_ip)
    {
        ctrl->remote_ip = *remote_ip;
    }
    else
    {
        ctrl->remote_ip.type = 0xff;
    }
    ctrl->auto_mode = 1;
    ctrl->remote_port = remote_port;
    network_adapter_t *adapter = &prv_adapter_table[ctrl->adapter_index];
    ctrl->wait_target_state = NW_WAIT_ON_LINE;
    if (!network_check_ready(ctrl, 0))
    {

        ctrl->state = NW_STATE_LINK_OFF;
        goto NETWORK_CONNECT_WAIT;
    }
    if (network_prepare_connect(ctrl))
    {
        ctrl->state = NW_STATE_OFF_LINE;
        ctrl->wait_target_state = NW_WAIT_NONE;
        NW_UNLOCK;
        return -1;
    }
NETWORK_CONNECT_WAIT:
    NW_UNLOCK;
    if (!ctrl->task_handle || !timeout_ms)
    {

        return 1;
    }
    uint8_t finish = 0;
    OS_EVENT event = {0};
    int result;
    //DBG_INFO("%s wait for active!,%u,%x", Net->Tag, To * SYS_TICK, Net->hTask);

    platform_start_timer(ctrl->timer, timeout_ms, 0);
    while (!finish)
    {
        platform_wait_event(ctrl->task_handle, 0, (luat_event_t *)&event, NULL, 0);
        switch (event.ID)
        {
        case EV_NW_RESULT_CONNECT:
            result = (int)event.Param1;
            finish = 1;
            break;
        case EV_NW_TIMEOUT:
            result = -1;
            finish = 1;
            break;
        default:
            if (ctrl->user_callback)
            {
                ctrl->user_callback((void *)&event, ctrl->user_data);
            }
            break;
        }
    }
    platform_stop_timer(ctrl->timer);
    return result;
}

int network_listen(network_ctrl_t *ctrl, uint32_t timeout_ms)
{
    if (NW_STATE_LISTEN == ctrl->state)
    {
        DBG("socket %d is listen", ctrl->socket_id);
        return 0;
    }
    if (ctrl->socket_id >= 0)
    {
        return -1;
    }
    NW_LOCK;
    ctrl->is_server_mode = 1;
    ctrl->auto_mode = 1;
    ctrl->need_close = 0;
    network_adapter_t *adapter = &prv_adapter_table[ctrl->adapter_index];
    ctrl->wait_target_state = NW_WAIT_ON_LINE;
    if (!network_check_ready(ctrl, 0))
    {

        ctrl->state = NW_STATE_LINK_OFF;
        goto NETWORK_LISTEN_WAIT;
    }
    if (network_base_connect(ctrl, NULL))
    {
        ctrl->state = NW_STATE_OFF_LINE;
        ctrl->wait_target_state = NW_WAIT_NONE;
        NW_UNLOCK;
        return -1;
    }
    ctrl->state = NW_STATE_CONNECTING;
NETWORK_LISTEN_WAIT:
    NW_UNLOCK;
    if (!ctrl->task_handle || !timeout_ms)
    {

        return 1;
    }
    uint8_t finish = 0;
    OS_EVENT event;
    int result;
    //DBG_INFO("%s wait for active!,%u,%x", Net->Tag, To * SYS_TICK, Net->hTask);

    if (timeout_ms != 0xffffffff)
    {
        platform_start_timer(ctrl->timer, timeout_ms, 0);
    }
    while (!finish)
    {
        platform_wait_event(ctrl->task_handle, 0, (luat_event_t *)&event, NULL, 0);
        switch (event.ID)
        {
        case EV_NW_RESULT_CONNECT:
            result = (int)event.Param1;
            finish = 1;
            break;
        case EV_NW_TIMEOUT:
            result = -1;
            finish = 1;
            break;
        default:
            if (ctrl->user_callback)
            {
                ctrl->user_callback((void *)&event, ctrl->user_data);
            }
            break;
        }
    }
    if (timeout_ms != 0xffffffff)
    {
        platform_stop_timer(ctrl->timer);
    }
    return result;
}

int network_close(network_ctrl_t *ctrl, uint32_t timeout_ms)
{
    NW_LOCK;
    if (ctrl->cache_data)
    {
        free(ctrl->cache_data);
        ctrl->cache_data = NULL;
    }
    uint8_t old_state = ctrl->state;
    ctrl->auto_mode = 1;
    ctrl->need_close = 0;
    ctrl->new_rx_flag = 0;
    network_adapter_t *adapter = &prv_adapter_table[ctrl->adapter_index];
#ifdef LUAT_USE_TLS
    if (ctrl->tls_mode)
    {
        mbedtls_ssl_free(ctrl->ssl);
    }
#endif
    if (ctrl->socket_id < 0)
    {

        ctrl->state = NW_STATE_OFF_LINE;
        ctrl->wait_target_state = NW_WAIT_NONE;
        NW_UNLOCK;
        return 0;
    }

    ctrl->state = NW_STATE_DISCONNECTING;
    ctrl->wait_target_state = NW_WAIT_OFF_LINE;

    if ((NW_STATE_ONLINE == old_state) && ctrl->is_tcp)
    {
        if (network_socket_disconnect(ctrl))
        {
            network_force_close_socket(ctrl);
            ctrl->state = NW_STATE_OFF_LINE;
            ctrl->wait_target_state = NW_WAIT_NONE;
            NW_UNLOCK;
            return 0;
        }
    }
    else
    {
        network_force_close_socket(ctrl);
        ctrl->state = NW_STATE_OFF_LINE;
        ctrl->wait_target_state = NW_WAIT_NONE;
        NW_UNLOCK;
        return 0;
    }
    NW_UNLOCK;
    if (!ctrl->task_handle || !timeout_ms)
    {
        return 1;
    }
    uint8_t finish = 0;
    OS_EVENT event;
    int result;
    //DBG_INFO("%s wait for active!,%u,%x", Net->Tag, To * SYS_TICK, Net->hTask);

    platform_start_timer(ctrl->timer, timeout_ms, 0);
    while (!finish)
    {
        platform_wait_event(ctrl->task_handle, 0, (luat_event_t *)&event, NULL, 0);
        switch (event.ID)
        {
        case EV_NW_RESULT_CLOSE:
            result = 0;
            finish = 1;
            break;
        case EV_NW_TIMEOUT:
            result = 0;
            finish = 1;
            break;
        default:
            if (ctrl->user_callback)
            {
                ctrl->user_callback((void *)&event, ctrl->user_data);
            }
            break;
        }
    }
    platform_stop_timer(ctrl->timer);
    network_socket_force_close(ctrl);
    return result;
}
/** When timeout_ms=0, it is a non-blocking interface*/
int network_tx(network_ctrl_t *ctrl, const uint8_t *data, uint32_t len, int flags, luat_ip_addr_t *remote_ip, uint16_t remote_port, uint32_t *tx_len, uint32_t timeout_ms)
{
    if ((ctrl->need_close) || (ctrl->socket_id < 0) || (ctrl->state != NW_STATE_ONLINE))
    {
        return -1;
    }
    NW_LOCK;
    int result;

    ctrl->auto_mode = 1;
#ifdef LUAT_USE_TLS
    if (ctrl->tls_mode)
    {
        if (ctrl->tls_need_reshakehand)
        {
            ctrl->tls_need_reshakehand = 0;
            if (ctrl->cache_data)
            {
                free(ctrl->cache_data);
                ctrl->cache_data = NULL;
            }
            ctrl->cache_data = malloc(len);
            memcpy(ctrl->cache_data, data, len);
            ctrl->cache_len = len;
            mbedtls_ssl_session_reset(ctrl->ssl);
            do
            {
                result = mbedtls_ssl_handshake_step( ctrl->ssl );
                switch(result)
                {
                case MBEDTLS_ERR_SSL_WANT_READ:
                    ctrl->state = NW_STATE_SHAKEHAND;
                    goto NETWORK_TX_WAIT;
                case 0:
                    break;
                default:
                    #if MBEDTLS_VERSION_NUMBER >= 0x03000000
                    #else
                    DBG_ERR("0x%x", -result);
                    #endif
                    ctrl->need_close = 1;
                    NW_UNLOCK;
                    return -1;
                }
            #if MBEDTLS_VERSION_NUMBER >= 0x03000000
            }while(!mbedtls_ssl_is_handshake_over(ctrl->ssl));
            #else
            }while(ctrl->ssl->state != MBEDTLS_SSL_HANDSHAKE_OVER);
            #endif
        }

        uint32_t done = 0;
        while(done < len)
        {
            result = mbedtls_ssl_write(ctrl->ssl, data + done, len - done);
            if (result < 0)
            {
                DBG("%08x", -result);
                ctrl->need_close = 1;
                NW_UNLOCK;
                return -1;
            }
            done += result;
        }

        *tx_len = done;
    }
    else
#endif
    {
        result = network_base_tx(ctrl, data, len, flags, remote_ip, remote_port);
        if (result < 0)
        {
            ctrl->need_close = 1;
            NW_UNLOCK;
            return -1;
        }
        *tx_len = result;
        if (!result && len)
        {
            NW_UNLOCK;
            return 0;
        }
    }

NETWORK_TX_WAIT:
    ctrl->wait_target_state = NW_WAIT_TX_OK;
    NW_UNLOCK;

    if (!ctrl->task_handle || !timeout_ms)
    {
        return 1;
    }
    uint8_t finish = 0;
    OS_EVENT event;
    //DBG_INFO("%s wait for active!,%u,%x", Net->Tag, To * SYS_TICK, Net->hTask);

    platform_start_timer(ctrl->timer, timeout_ms, 0);
    while (!finish)
    {
        platform_wait_event(ctrl->task_handle, 0, (luat_event_t *)&event, NULL, 0);
        switch (event.ID)
        {
        case EV_NW_RESULT_TX:
            result = (int)event.Param1;
            finish = 1;
            break;
        case EV_NW_TIMEOUT:
            result = -1;
            finish = 1;
            break;
        default:
            if (ctrl->user_callback)
            {
                ctrl->user_callback((void *)&event, ctrl->user_data);
            }
            break;
        }
    }
    platform_stop_timer(ctrl->timer);
    return result;
}
/** The actual amount of data read is in rx_len. If it is UDP mode and server, you need to look at remote_ip and remote_port.*/
int network_rx(network_ctrl_t *ctrl, uint8_t *data, uint32_t len, int flags, luat_ip_addr_t *remote_ip, uint16_t *remote_port, uint32_t *rx_len)
{
    if (((ctrl->need_close && !ctrl->new_rx_flag) || (ctrl->socket_id < 0) || (ctrl->state != NW_STATE_ONLINE)))
    {
        return -1;
    }
    NW_LOCK;
    int result = -1;
    ctrl->auto_mode = 1;
    uint32_t read_len = 0;
    uint8_t is_error = 0;


    if (data)
    {
        ctrl->new_rx_flag = 0;
#ifdef LUAT_USE_TLS
        if (ctrl->tls_mode)
        {

            do
            {
                result = mbedtls_ssl_read(ctrl->ssl, data + read_len, len - read_len);
                if (result < 0 && (result != MBEDTLS_ERR_SSL_WANT_READ))
                {
                    is_error = 1;
                    break;
                }
                else if (result > 0)
                {
                    read_len += result;
                    if (read_len >= len)
                    {
                        break;
                    }
                }
                else
                {
                    break;
                }
            }while(network_socket_receive(ctrl, NULL, len, flags, remote_ip, remote_port) > 0);

            if ( !is_error )
            {
                result = read_len;
            }
            else
            {
                result = -1;
            }
        }
        else
#endif
        {

            result = network_socket_receive(ctrl, data, len, flags, remote_ip, remote_port);
        }
    }
    else
    {
#ifdef LUAT_USE_TLS
        if (ctrl->tls_mode)
        {
            read_len = 0;
            do
            {
                result = mbedtls_ssl_read(ctrl->ssl, NULL, 0);
                if ((result < 0) && (result != (MBEDTLS_ERR_SSL_WANT_READ)))
                {
                    is_error = 1;
                    read_len = 0;
                    break;
                }
                else if (!result)
                {
                    #if MBEDTLS_VERSION_NUMBER >= 0x03000000
                    read_len = ctrl->ssl->MBEDTLS_PRIVATE(in_msglen);
                    #else
                    read_len = ctrl->ssl->in_msglen;
                    #endif
                    break;
                }
                else if ((MBEDTLS_ERR_SSL_WANT_READ) == result)
                {
                    read_len = 0;
                    ctrl->new_rx_flag = 0;
                    DBG("socket %d ssl data need more", ctrl->socket_id);
                    break;
                }
            }while(network_socket_receive(ctrl, NULL, len, flags, remote_ip, remote_port) > 0);

            if ( !is_error )
            {
                result = read_len;
            }
            else
            {
                result = -1;
            }
        }
        else
#endif
        {
            result = network_socket_receive(ctrl, data, len, flags, remote_ip, remote_port);
        }
    }


    NW_UNLOCK;
    if (result >= 0)
    {
        *rx_len = result;
        return 0;
    }
    else
    {
        return -1;
    }
}

int network_wait_event(network_ctrl_t *ctrl, OS_EVENT *out_event, uint32_t timeout_ms, uint8_t *is_timeout)
{
    if (ctrl->new_rx_flag)
    {
        ctrl->wait_target_state = NW_WAIT_EVENT;
        if (out_event)
        {
            out_event->ID = 0;
        }
        return 0;
    }
    if ((ctrl->need_close) || (ctrl->socket_id < 0) || (ctrl->state != NW_STATE_ONLINE))
    {
        return -1;
    }
    NW_LOCK;
    ctrl->auto_mode = 1;
    ctrl->wait_target_state = NW_WAIT_EVENT;
    NW_UNLOCK;
    if (!ctrl->task_handle || !timeout_ms)
    {
        return 1;
    }
    *is_timeout = 0;
    uint8_t finish = 0;
    OS_EVENT event;
    int result;
    //DBG_INFO("%s wait for active!,%u,%x", Net->Tag, To * SYS_TICK, Net->hTask);

    platform_start_timer(ctrl->timer, timeout_ms, 0);
    while (!finish)
    {
        platform_wait_event(ctrl->task_handle, 0, (luat_event_t *)&event, NULL, 0);
        switch (event.ID)
        {
        case EV_NW_RESULT_EVENT:
            result = (int)event.Param1;
            if (result)
            {
                result = -1;
            }
            if (out_event)
            {
                out_event->ID = 0;
            }
            finish = 1;
            break;
        case EV_NW_TIMEOUT:
            *is_timeout = 1;
            result = 0;
            finish = 1;
            break;
        case EV_NW_BREAK_WAIT:
            if (out_event)
            {
                *out_event = event;
            }
            result = 0;
            finish = 1;
            break;
        default:
            if (out_event)
            {
                *out_event = event;
                result = 0;
                finish = 1;
                break;
            }
            else if (ctrl->user_callback)
            {
                ctrl->user_callback((void *)&event, ctrl->user_data);
            }
            break;
        }
    }
    platform_stop_timer(ctrl->timer);
    return result;
}

int network_wait_rx(network_ctrl_t *ctrl, uint32_t timeout_ms, uint8_t *is_break, uint8_t *is_timeout)
{
    *is_timeout = 0;
    *is_break = 0;
    if (ctrl->new_rx_flag)
    {
        ctrl->wait_target_state = NW_WAIT_EVENT;
        return 0;
    }
    if ((ctrl->need_close) || (ctrl->socket_id < 0) || (ctrl->state != NW_STATE_ONLINE))
    {
        return -1;
    }
    NW_LOCK;
    ctrl->auto_mode = 1;
    ctrl->wait_target_state = NW_WAIT_EVENT;
    NW_UNLOCK;

    uint8_t finish = 0;
    OS_EVENT event;
    int result;
    //DBG_INFO("%s wait for active!,%u,%x", Net->Tag, To * SYS_TICK, Net->hTask);
    if (timeout_ms)
    {
        platform_start_timer(ctrl->timer, timeout_ms, 0);
    }
    while (!finish)
    {
        platform_wait_event(ctrl->task_handle, 0, (luat_event_t *)&event, NULL, 0);
        switch (event.ID)
        {
        case EV_NW_RESULT_EVENT:
            result = (int)event.Param1;
            if (result)
            {
                result = -1;
                finish = 1;
            }
            else if (ctrl->new_rx_flag)
            {
                result = 0;
                finish = 1;
            }
            break;
        case EV_NW_TIMEOUT:
            *is_timeout = 1;
            result = 0;
            finish = 1;
            break;
        case EV_NW_BREAK_WAIT:
            *is_break = 1;
            result = 0;
            finish = 1;
            break;
        default:
            if (ctrl->user_callback)
            {
                ctrl->user_callback((void *)&event, ctrl->user_data);
            }
            break;
        }
        ctrl->wait_target_state = NW_WAIT_EVENT;
    }
    platform_stop_timer(ctrl->timer);
    return result;
}

uint8_t network_check_ready(network_ctrl_t *ctrl, uint8_t adapter_index)
{
    network_adapter_t *adapter;
    if (ctrl)
    {
        adapter = &prv_adapter_table[ctrl->adapter_index];
    }
    else if (adapter_index < NW_ADAPTER_QTY)
    {
        adapter = &prv_adapter_table[adapter_index];
    }
    else
    {
        return 0;
    }
    if (adapter->opt)
    {
        return adapter->opt->check_ready(adapter->user_data);
    }
    else
    {
        return 0;
    }
}
