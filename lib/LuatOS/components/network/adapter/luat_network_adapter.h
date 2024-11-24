#ifndef __LUAT_NW_ADAPTER_H__
#define __LUAT_NW_ADAPTER_H__
#include "luat_base.h"
// #ifdef LUAT_USE_NETWORK
#if defined(LUAT_EC7XX_CSDK) || defined(CHIP_EC618) || defined(__AIR105_BSP__)
#include "bsp_common.h"
#endif
#include "luat_rtos.h"
#ifndef __BSP_COMMON_H__
#include "c_common.h"
#endif
#ifdef LUAT_USE_TLS
#include "mbedtls/ssl.h"
#include "mbedtls/platform.h"
#include "mbedtls/debug.h"
#include "mbedtls/x509_crt.h"
#include "mbedtls/base64.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"
#include "mbedtls/sha1.h"
#endif
#ifdef LUAT_USE_LWIP
#include "lwip/opt.h"
#include "lwip/netif.h"
#include "lwip/timeouts.h"
#include "lwip/priv/tcp_priv.h"

#include "lwip/def.h"
#include "lwip/memp.h"
#include "lwip/priv/tcpip_priv.h"

#include "lwip/ip4_frag.h"
#include "lwip/etharp.h"
#include "lwip/dhcp.h"
#include "lwip/prot/dhcp.h"
#include "lwip/autoip.h"
#include "lwip/igmp.h"
#include "lwip/dns.h"
#include "lwip/nd6.h"
#include "lwip/ip6_frag.h"
#include "lwip/mld6.h"
#include "lwip/dhcp6.h"
#include "lwip/sys.h"
#include "lwip/pbuf.h"
#include "lwip/inet.h"
#endif
#define MAX_DNS_IP		(4)	//Reserve up to 4 IPs for each URL

enum
{
	EV_NW_RESET = USER_EVENT_ID_START + 0x1000000,
	EV_NW_STATE,
	EV_NW_TIMEOUT,
	EV_NW_DNS_RESULT,
	EV_NW_SOCKET_TX_OK,
	EV_NW_SOCKET_RX_NEW,
	EV_NW_SOCKET_RX_FULL,
	EV_NW_SOCKET_CLOSE_OK,
	EV_NW_SOCKET_REMOTE_CLOSE,
	EV_NW_SOCKET_CONNECT_OK,

	EV_NW_SOCKET_ERROR,
	EV_NW_SOCKET_LISTEN,
	EV_NW_SOCKET_NEW_CONNECT,	//When a new connect is received as a server, only the accept operation is allowed, otherwise CONNECT_OK is reported directly.
	EV_NW_BREAK_WAIT,
	EV_NW_END,

	NW_STATE_LINK_OFF = 0,
	NW_STATE_OFF_LINE,
	NW_STATE_WAIT_DNS,
	NW_STATE_CONNECTING,
	NW_STATE_SHAKEHAND,
	NW_STATE_ONLINE,
	NW_STATE_LISTEN,
	NW_STATE_DISCONNECTING,

	NW_WAIT_NONE = 0,
	NW_WAIT_LINK_UP,
	NW_WAIT_ON_LINE,
	NW_WAIT_TX_OK,
	NW_WAIT_OFF_LINE,
	NW_WAIT_EVENT,

	//Once the advanced API is used, the callback will be changed to the following, param1 = 0 succeeds, others fail
	EV_NW_RESULT_BASE = EV_NW_END + 1,
	EV_NW_RESULT_LINK = EV_NW_RESULT_BASE + NW_WAIT_LINK_UP,
	EV_NW_RESULT_CONNECT = EV_NW_RESULT_BASE + NW_WAIT_ON_LINE,
	EV_NW_RESULT_CLOSE = EV_NW_RESULT_BASE + NW_WAIT_OFF_LINE,
	EV_NW_RESULT_TX = EV_NW_RESULT_BASE + NW_WAIT_TX_OK,
	EV_NW_RESULT_EVENT = EV_NW_RESULT_BASE + NW_WAIT_EVENT,

	NW_ADAPTER_INDEX_LWIP_NONE = 0,
	NW_ADAPTER_INDEX_LWIP_GPRS,	//Cellular network Modules
	NW_ADAPTER_INDEX_LWIP_WIFI_STA,	//WIFI SOC
	NW_ADAPTER_INDEX_LWIP_WIFI_AP,	//WIFI SOC
	NW_ADAPTER_INDEX_LWIP_ETH,		//SOC with built-in Ethernet controller
	NW_ADAPTER_INDEX_LWIP_NETIF_QTY,
	NW_ADAPTER_INDEX_HW_PS_DEVICE = NW_ADAPTER_INDEX_LWIP_NETIF_QTY,
	NW_ADAPTER_INDEX_ETH0 = NW_ADAPTER_INDEX_HW_PS_DEVICE,	//Plug-in Ethernet + hardware protocol stack
	NW_ADAPTER_INDEX_USB,			//USB network card
	NW_ADAPTER_INDEX_POSIX,         // Connect to POSIX
	NW_ADAPTER_INDEX_LUAPROXY,      // Proxy to Lua layer
	NW_ADAPTER_INDEX_CUSTOM,        // Connect to custom adapter
	NW_ADAPTER_QTY,

	NW_CMD_AUTO_HEART_TIME = 0,



};

#ifdef LUAT_USE_LWIP
#define luat_ip_addr_t ip_addr_t
#else
typedef struct
{
	union
	{
		uint32_t ipv4;
		uint32_t ipv6_u32_addr[4];
	    uint8_t  ipv6_u8_addr[16];
	};
	uint8_t is_ipv6;
}luat_ip_addr_t;
uint8_t network_string_is_ipv4(const char *string, uint32_t len);
uint32_t network_string_to_ipv4(const char *string, uint32_t len);
int network_string_to_ipv6(const char *string, luat_ip_addr_t *ip_addr);
#endif

typedef struct
{
	uint64_t tag;
	void *param;
}luat_network_cb_param_t;

typedef struct
{
	uint32_t ttl_end;
	luat_ip_addr_t ip;
}luat_dns_ip_result;


typedef struct
{
	/* data */
	llist_head node;
	Buffer_Struct uri;
	luat_dns_ip_result result[MAX_DNS_IP];
	uint8_t ip_nums;
}luat_dns_cache_t;

typedef struct
{
	uint64_t tx_size;
	uint64_t ack_size;
	uint64_t tag;
#ifdef LUAT_USE_TLS
	//SSL related data are dynamically generated and need to be released when close
    mbedtls_ssl_context *ssl;          /**< mbed TLS control context. */
    mbedtls_ssl_config *config;          /**< mbed TLS configuration context. */
    mbedtls_x509_crt *ca_cert;
#endif

	CBFuncEx_t user_callback;
	void *user_data;			//pParam passed to user_callback
	void *socket_param;			//Generally used to store network_ctrl itself for quick search
	HANDLE	task_handle;
	HANDLE timer;
	HANDLE tls_short_timer;
	HANDLE tls_long_timer;
	HANDLE mutex;
	uint32_t tcp_keep_idle;
	int socket_id;
	char *domain_name;			//Dynamicly generated, needs to be released when close
	uint32_t domain_name_len;
	luat_ip_addr_t remote_ip;
	luat_dns_ip_result *dns_ip;	//Dynamicly generated, needs to be released when close
	luat_ip_addr_t *online_ip;	//Point to an IP, no need to release
	uint16_t remote_port;
	uint16_t local_port;
	uint8_t *cache_data;	//Dynamicly generated, needs to be released when close
	uint32_t cache_len;
	int tls_timer_state;
	uint32_t tcp_timeout_ms;
	uint8_t tls_mode;
    uint8_t tls_need_reshakehand;
    uint8_t need_close;
    uint8_t new_rx_flag;
    uint8_t dns_ip_cnt;
    uint8_t dns_ip_nums;
    uint8_t tcp_keep_alive;
	uint8_t tcp_keep_interval;
	uint8_t tcp_keep_cnt;
    uint8_t adapter_index;
    uint8_t is_tcp;
    uint8_t is_server_mode;
    uint8_t auto_mode;
    uint8_t wait_target_state;
    uint8_t state;
    uint8_t is_debug;
    uint8_t domain_ipv6;
}network_ctrl_t;

typedef struct
{
	uint64_t tag;
#ifdef LUAT_USE_LWIP
	llist_head wait_ack_head;
#endif
	llist_head tx_head;
	llist_head rx_head;
	uint32_t rx_wait_size;
	uint32_t tx_wait_size;
#ifdef LUAT_USE_LWIP
	union {
		struct ip_pcb  *ip;
		struct tcp_pcb *tcp;
		struct udp_pcb *udp;
		struct raw_pcb *raw;
	} pcb;
	struct tcp_pcb_listen *listen_tcp;
	HANDLE mutex;
	uint16_t local_port;
	uint16_t remote_port;
#endif
	void *param;
	uint8_t state;
	uint8_t is_tcp;
	uint8_t is_ipv6;
	uint8_t in_use;
	uint8_t rx_waiting;
	uint8_t remote_close;
	uint8_t fast_rx_ack;	//TCP quick response
}socket_ctrl_t;		//Recommended socket status structure for underlying protocol stack adaptation

/** The APIs in info must all be non-blocking and task-based, and the socket_id and tag must be checked for validity.
 * Currently only supports tcp and udp, raw is not supported
 * If there are no special instructions, return =0 on success and <0 on failure.*/
typedef struct
{
	//Check whether the network is ready, return non-0 ready, user_data is the user_data during registration, passed to the underlying API
	uint8_t (*check_ready)(void *user_data);
	//Create a socket and set it to non-blocking mode. User_data is passed to the corresponding adapter, and tag is used as the legal basis of the socket for check_socket_vaild comparison.
	//Return socketid on success, <0 on failure
	int (*create_soceket)(uint8_t is_tcp, uint64_t *tag, void *param, uint8_t is_ipv6, void *user_data);
	//Bind a port as a client and connect to the server corresponding to remote_ip and remote_port
	//Return 0 on success, <0 on failure
	int (*socket_connect)(int socket_id, uint64_t tag, uint16_t local_port, luat_ip_addr_t *remote_ip, uint16_t remote_port, void *user_data);
	//Bind a port as a server and start listening
	//Return 0 on success, <0 on failure
	int (*socket_listen)(int socket_id, uint64_t tag, uint16_t local_port, void *user_data);
	//Accept a client as a server
	//Return 0 on success, <0 on failure
	int (*socket_accept)(int socket_id, uint64_t tag, luat_ip_addr_t *remote_ip, uint16_t *remote_port, void *user_data);
	//To proactively disconnect a tcp connection, the entire tcp process needs to be completed. The user needs to receive the close ok callback to confirm the complete disconnection.
	//Return 0 on success, <0 on failure
	int (*socket_disconnect)(int socket_id, uint64_t tag, void *user_data);
	//Release control of the socket, except for tag exceptions, it must take effect immediately
	//Return 0 on success, <0 on failure
	int (*socket_close)(int socket_id, uint64_t tag, void *user_data);
	//Forcibly release control of the socket, which must take effect immediately
	//Return 0 on success, <0 on failure
	int (*socket_force_close)(int socket_id, void *user_data);
	//In tcp, remote_ip and remote_port are not needed. If buf is NULL, the amount of data in the current buffer is returned. When the return value is less than len, it means that the reading has been completed.
	//When udp is used, only 1 block of data is returned, and it needs to be read multiple times until there is no data.
	//Return the actual read value on success, <0 on failure
	int (*socket_receive)(int socket_id, uint64_t tag, uint8_t *buf, uint32_t len, int flags, luat_ip_addr_t *remote_ip, uint16_t *remote_port, void *user_data);
	//In tcp, remote_ip and remote_port are not required
	//Return >0 len successfully, buffer full = 0, failure < 0, if an empty packet with len=0 is sent, it will also return 0, pay attention to judgment
	int (*socket_send)(int socket_id, uint64_t tag, const uint8_t *buf, uint32_t len, int flags, luat_ip_addr_t *remote_ip, uint16_t remote_port, void *user_data);
	//Check the validity of the socket, return 0 if successful, <0 if failed
	int (*socket_check)(int socket_id, uint64_t tag, void *user_data);
	//Keep valid sockets and close invalid sockets
	void (*socket_clean)(int *vaild_socket_list, uint32_t num, void *user_data);
	int (*getsockopt)(int socket_id, uint64_t tag, int level, int optname, void *optval, uint32_t *optlen, void *user_data);
	int (*setsockopt)(int socket_id, uint64_t tag, int level, int optname, const void *optval, uint32_t optlen, void *user_data);
	//Non-posix socket, use this to set parameters according to actual hardware
	int (*user_cmd)(int socket_id, uint64_t tag, uint32_t cmd, uint32_t value, void *user_data);

	int (*dns)(const char *domain_name, uint32_t len, void *param,  void *user_data);
	int (*dns_ipv6)(const char *domain_name, uint32_t len, void *param,  void *user_data);
	int (*set_dns_server)(uint8_t server_index, luat_ip_addr_t *ip, void *user_data);
	int (*set_mac)(uint8_t *mac, void *user_data);
	int (*set_static_ip)(luat_ip_addr_t *ip, luat_ip_addr_t *submask, luat_ip_addr_t *gateway, luat_ip_addr_t *ipv6, void *user_data);
	int (*get_full_ip_info)(luat_ip_addr_t *ip, luat_ip_addr_t *submask, luat_ip_addr_t *gateway, luat_ip_addr_t *ipv6, void *user_data);
	int (*get_local_ip_info)(luat_ip_addr_t *ip, luat_ip_addr_t *submask, luat_ip_addr_t *gateway, void *user_data);
	//All network messages are passed through cb_fun callback
	//The first parameter in the cb_fun callback is OS_EVENT, which contains the necessary information about the socket. The second parameter is luat_network_cb_param_t, where the param is the param passed in here (that is, the adapter serial number)
	//OS_EVENT ID is EV_NW_XXX, param1 is the socket id, param2 is the respective parameter, param3 is the socket_param passed in by create_soceket (that is, network_ctrl *)
	//The dns result is special, the ID is EV_NW_SOCKET_DNS_RESULT, param1 is the amount of IP data obtained, 0 means failure, param2 is the ip group, dynamically allocated, param3 is the param passed in by dns (that is, network_ctrl *)
	void (*socket_set_callback)(CBFuncEx_t cb_fun, void *param, void *user_data);
	int (*check_ack)(uint8_t adapter_index, int socket_id); //Added on 2024.4.11

	char *name;
	int max_socket_num;//The maximum number of sockets is also the basic value of the maximum number of network_ctrl applications.
	uint8_t no_accept;
	uint8_t is_posix;
}network_adapter_info;
/** The API may involve task safety requirements and cannot be run in interrupts, but can only be run in tasks.*/

/**
 * Get the last registered network card adapter serial number
 * @return Registered network adapter serial number NW_ADAPTER_INDEX_XXX*/
int network_get_last_register_adapter(void);
/****************************The following is the common basic API****************** **************************************/

/**
 * Register the protocol stack interface related to the network card adapter
 * @param adapter_index Network adapter serial number NW_ADAPTER_INDEX_XXX
 * @param info protocol stack interface
 * @param user_data user data
 * @return 0 success, others failure*/
int network_register_adapter(uint8_t adapter_index, network_adapter_info *info, void *user_data);

/**
 * Configure a network card adapter as the default adapter
 * @param adapter_index Network adapter serial number NW_ADAPTER_INDEX_XXX*/
void network_register_set_default(uint8_t adapter_index);

/**
 * Set the DNS server address of a certain network card adapter
 * @param adapter_index Network adapter serial number NW_ADAPTER_INDEX_XXX
 * @param server_index DNS server serial number
 * @param ip DNS server address*/
void network_set_dns_server(uint8_t adapter_index, uint8_t server_index, luat_ip_addr_t *ip);

/**
 * Apply for a network resource on a certain network card adapter
 * @param adapter_index adapter serial number NW_ADAPTER_INDEX_XXX
 * @return network resource, network_ctrl_t structure type*/
network_ctrl_t *network_alloc_ctrl(uint8_t adapter_index);

/**
 * Return a network resource
 * @param ctrl network resources*/
void network_release_ctrl(network_ctrl_t *ctrl);

/**
 * Initialize network resources
 * When lua calls c, it must use a non-blocking interface, and task_handle does not need to
 * When calling in pure C, if it is not needed, plug the application and must have callback and param.
 * When calling in pure C, if you need to block the application, you must have a task_handle. It is recommended to have callback and param. When you can wait for the message, you can process other types of messages in the callback at the same time.
 * @param ctrl network resources
 * @param task_handle The task where the socket blocks the application, which can be empty
 * @param callback socket callback function for non-blocking applications
 * @param param User-defined parameters for callback in socket non-blocking application*/
void network_init_ctrl(network_ctrl_t *ctrl, HANDLE task_handle, CBFuncEx_t callback, void *param);

/**
 *Basic configuration of network resources
 * Set whether it is tcp or udp mode and TCP automatic keep-alive related parameters. You can also directly change the is_tcp parameter in network_ctrl_t.
 * The setting must be in the close state of the socket before connect and tls are initialized.
 * @param ctrl network resources
 * @param is_tcp 0UDP other TCP, the default is UDP
 * @param tcp_timeout_ms tcp timeout, currently not enabled
 * @param keep_alive Whether tcp automatic keep alive is enabled, 0 is not enabled, others are enabled
 * @param keep_idle tcp automatic keep alive idle time, unit seconds
 * @param keep_interval When tcp automatic keep-alive is started, the tcp probe sending time interval, in seconds
 * @param keep_cnt When tcp automatic keep-alive is started, the total number of tcp probes sent*/
void network_set_base_mode(network_ctrl_t *ctrl, uint8_t is_tcp, uint32_t tcp_timeout_ms, uint8_t keep_alive, uint32_t keep_idle, uint8_t keep_interval, uint8_t keep_cnt);
/**
 * Whether to enable debug function
 * @param ctrl network resources
 * @param on_off 0 is off, others are on, the default is off*/
void network_set_debug_onoff(network_ctrl_t *ctrl, uint8_t on_off);


/**
 * Whether to select an IPV6 address when using a domain name
 * @param ctrl network resources
 * @param onoff 0 off, others on, the default is off*/
void network_connect_ipv6_domain(network_ctrl_t *ctrl, uint8_t onoff);


/**
 * Check whether a certain network card can access the Internet
 * For specific network cards, see the socket or adapter serial number.
 * @param ctrl network resource, if it is NULL, look at the adapter serial number below
 * @param adapter_index adapter serial number NW_ADAPTER_INDEX_XXX
 * @return 0 cannot access the Internet, others can*/
uint8_t network_check_ready(network_ctrl_t *ctrl, uint8_t adapter_index);

/**
 * Set the local port, be careful not to use 60000 or above. If local_port is 0, the system will randomly select one starting from 60000.
 * The default is 0, generally no configuration is required, it is allocated by the system.
 * @param ctrl network resources
 * @param local_port local port number
 * @return local_port is not 0, and if it is repeated, it returns -1, otherwise it returns 0*/
int network_set_local_port(network_ctrl_t *ctrl, uint16_t local_port);

/**
 * Create a socket
 * @param ctrl network resources
 * @param is_ipv6 Whether it is IPV6, 0 no, 1 yes, the default is 0
 * @return Returns 0 on success, < 0 on failure*/
int network_create_socket(network_ctrl_t *ctrl, uint8_t is_ipv6);

/**
 * Connect to the server. Before connecting, you need to configure the remote_port in the network resources.
 * @param ctrl network resources
 * @param remote_ip server address
 * @return Returns 0 on success, < 0 on failure*/
int network_socket_connect(network_ctrl_t *ctrl, luat_ip_addr_t *remote_ip);

/**
 * Listen as a server
 * @param ctrl network resource network resource used for monitoring
 * @return Returns 0 on success, < 0 on failure*/
int network_socket_listen(network_ctrl_t *ctrl);

/**
 * Query whether the network card corresponding to the network resource has server function
 * @param ctrl network resource network resource used for monitoring
 * @return 1 yes 0 no*/
uint8_t network_accept_enable(network_ctrl_t *ctrl);

/**
 *Accepts a client's access request as a server
 * @param ctrl network resource network resource used for monitoring
 * @param accept_ctrl Network resource used to communicate with the client
 * @return Returns 0 on success, < 0 on failure*/
int network_socket_accept(network_ctrl_t *ctrl, network_ctrl_t *accept_ctrl);

/**
 * Actively disconnecting a tcp connection requires completing the entire tcp waving process. The user needs to receive a close ok callback to confirm the complete disconnection.
 * @param ctrl network resources
 * @return are all 0*/
int network_socket_disconnect(network_ctrl_t *ctrl);

/**
 * Release control of the socket
 * @param ctrl network resources
 * @return are all 0*/
int network_socket_close(network_ctrl_t *ctrl);

/**
 *Forcibly release control of the socket
 * @param ctrl network resources
 * @return are all 0*/
int network_socket_force_close(network_ctrl_t *ctrl);

/**
 * Read the data sent by the peer
 * When tcp is used, remote_ip and remote_port are not needed. If buf is NULL, the amount of data in the current buffer is returned. When the return value is less than len, it means that the reading has been completed.
 * When using udp, only 1 block of data is returned, and it needs to be read multiple times until there is no data.
 * @param ctrl network resources
 * @param buf The cached address after data reading
 * @param len The length expected to be read
 * @param flags Flags when reading, usually 0
 * @param remote_ip stores the peer IP, only available for UDP
 * @param remote_port stores the peer port, which is only available for UDP
 * @return Successfully returns the actual read value, failure < 0*/
int network_socket_receive(network_ctrl_t *ctrl,uint8_t *buf, uint32_t len, int flags, luat_ip_addr_t *remote_ip, uint16_t *remote_port);

/**
 * Cache the sent data and prepare to send it to the peer
 * When using tcp, remote_ip and remote_port are not required
 * @param ctrl network resources
 * @param buf The address of the cache to send data
 * @param len sending length
 * @param flags Send flag, usually 0
 * @param remote_ip Peer IP, only available for UDP
 * @param remote_port peer port, only available for UDP
 * @return Store in sending cache and return 0, failure < 0*/
int network_socket_send(network_ctrl_t *ctrl,const uint8_t *buf, uint32_t len, int flags, luat_ip_addr_t *remote_ip, uint16_t remote_port);

/**
 * Obtain a socket configuration, which is consistent with POSIX socket. Non-POSIX network cards (such as the hardware protocol stack of w5500) do not support it.
 * @param ctrl network resources
 * @param level configuration parameter level
 * @param optname parameter name
 * @param optval parameter pointer
 * @param optlen parameter length
 * @return Returns 0 if successful, otherwise failed*/
int network_getsockopt(network_ctrl_t *ctrl, int level, int optname, void *optval, uint32_t *optlen);

/**
 * Set a configuration for the socket, which is consistent with the POSIX socket. Non-POSIX network cards (such as the hardware protocol stack of w5500) do not support it.
 * @param ctrl network resources
 * @param level configuration parameter level
 * @param optname parameter name
 * @param optval parameter pointer
 * @param optlen parameter length
 * @return Returns 0 if successful, otherwise failed*/
int network_setsockopt(network_ctrl_t *ctrl, int level, int optname, const void *optval, uint32_t optlen);

/**
 * Set a socket configuration, which only supports non-POSIX network cards (such as the w5500 hardware protocol stack)
 * @param ctrl network resources
 * @param cmd configuration command
 * @param value configuration value
 * @return Returns 0 if successful, otherwise failed*/
int network_user_cmd(network_ctrl_t *ctrl,  uint32_t cmd, uint32_t value);
#ifdef LUAT_USE_LWIP
/**
 * Set the MAC of the network card
 * @param adapter_index Network adapter serial number NW_ADAPTER_INDEX_XXX
 * @param mac MAC address
 * @return Returns 0 if successful, otherwise failed*/
int network_set_mac(uint8_t adapter_index, uint8_t *mac);
/**
 * Set the static address of the network card
 * @param adapter_index Network adapter serial number NW_ADAPTER_INDEX_XXX
 * @param ip IPV4 address
 * @param submask subnet mask
 * @param gateway gateway
 * @param ipv6 IPV6 address
 * @return Returns 0 if successful, otherwise failed*/
int network_set_static_ip_info(uint8_t adapter_index, luat_ip_addr_t *ip, luat_ip_addr_t *submask, luat_ip_addr_t *gateway, luat_ip_addr_t *ipv6);
#endif
/**
 * Get the address information of the network card
 * @param ctrl network resources
 * @param ip IPV4 address
 * @param submask subnet mask
 * @param gateway gateway
 * @return Returns 0 if successful, otherwise failed*/
int network_get_local_ip_info(network_ctrl_t *ctrl, luat_ip_addr_t *ip, luat_ip_addr_t *submask, luat_ip_addr_t *gateway);
/**
 * Force the release of socket resources and also release some network resources
 * @param ctrl network resources*/
void network_force_close_socket(network_ctrl_t *ctrl);

/**
 * Start DNS resolution. Before starting, domain_name and domain_name_len need to be set.
 * domain_name is already in the form of ip, return 1, and fill in remote_ip
 * @param ctrl network resources
 * @return domain_name is already in the form of ip. Return 1, DNS starts to return 0, failure < 0*/
int network_dns(network_ctrl_t *ctrl);
/**
 * Query whether all network resources under the network card need to be released, and release them immediately if they need to be released
 * @param adapter_index Network adapter serial number NW_ADAPTER_INDEX_XXX*/
void network_clean_invaild_socket(uint8_t adapter_index);

//The following APIs are only used for internal debugging and are not used by users.
const char *network_ctrl_state_string(uint8_t state);
const char *network_ctrl_wait_state_string(uint8_t state);
const char *network_ctrl_callback_event_string(uint32_t event);
/****************************End of common basic API************************ *************************************/

/****************************tls related api************************ ****************************************/

/**
 * Set PSK for DTLS and use it for UDP encrypted transmission
 * @param ctrl network resources
 * @param psk psk data
 * @param psk_len psk data length
 * @param psk_identity psk_identity data
 * @param psk_identity_len psk_identity data length
 * @return Returns 0 if successful, otherwise failed*/
int network_set_psk_info(network_ctrl_t *ctrl,
		const unsigned char *psk, size_t psk_len,
		const unsigned char *psk_identity, size_t psk_identity_len);

/**
 * TLS settings verify the server’s certificate, which can be omitted
 * @param ctrl network resources
 * @param cert server certificate data
 * @param cert_len server certificate data length
 * @return Returns 0 if successful, otherwise failed*/
int network_set_server_cert(network_ctrl_t *ctrl, const unsigned char *cert, size_t cert_len);

/**
 * TLS settings verify the client's certificate, which is only required for two-way authentication, and is generally only required in the financial field.
 * @param ctrl network resources
 * @param cert client certificate data
 * @param certLen client certificate data length
 * @param key client certificate private key data
 * @param keylen client certificate private key data length
 * @param pwd Password data of the client certificate private key
 * @param pwdlen Password data length of client certificate private key
 * @return Returns 0 if successful, otherwise failed*/
int network_set_client_cert(network_ctrl_t *ctrl,
		const unsigned char *cert, size_t certLen,
        const unsigned char *key, size_t keylen,
        const unsigned char *pwd, size_t pwdlen);

/**
 * Get certificate verification results
 * @param ctrl network resources
 * @return Returns 0 if successful, otherwise failed*/
int network_cert_verify_result(network_ctrl_t *ctrl);

/**
 * Initialize encrypted transmission
 * @param ctrl network resources
 * @param verify_mode refer to MBEDTLS_SSL_VERIFY_XXX
 * @return Returns 0 if successful, otherwise failed*/
int network_init_tls(network_ctrl_t *ctrl, int verify_mode);

/**
 * End the encrypted transmission mode and return to normal mode. It can only be modified when the socket is closed.
 * @param ctrl network resources*/
void network_deinit_tls(network_ctrl_t *ctrl);
/** Encrypted transmission is shared by other non-blocking APIs and general APIs. Blocking APIs and RTO environment-related blocking APIs are common, and are processed internally by the API.*/
/****************************TLS related api end********************** *****************************************/


/****************************Advanced API, used to implement a complete function****************** ***********/
//Once the following api is used, the network will automatically determine the status and proceed to the next step. In addition to actively forcibly closing the socket during the intermediate processing process, other users cannot intervene until the target state is reached. Even non-blocking callbacks will only call back the final result.
//All blocking state interfaces will return an error once they receive link down, socket close, error and other messages. If it is a timeout, only the wait event will return success, and the others will return failure.
//The following api can be blocking or non-blocking. When task_handle is set in network_ctrl and timeout_ms > 0, it is a blocking interface, and timeout_ms = 0xffffffff means waiting forever.

/**
 * Wait for the network environment to be ready
 * @param ctrl network resources
 * @param timeout_ms timeout time
 * @return Success returns 0, failure < 0, non-blocking needs to wait to return 1*/
int network_wait_link_up(network_ctrl_t *ctrl, uint32_t timeout_ms);

/**
 * Connect to the server as a client
 * Write at least one domain name or IP. If a valid IP is written, the domain name will not be connected.
 * Will automatically wait for the network environment to be ready, no need for network_wait_link_up
 * If it is encryption mode, handshake will be performed automatically
 * You must make sure it is in the close state before use. It is recommended to use network_close first.
 * @param ctrl network resources
 * @param domain_name server domain name
 * @param domain_name_len server domain name length
 * @param remote_ip server IP
 * @param remote_port server port
 * @param timeout_ms timeout time
 * @return Success returns 0, failure < 0, non-blocking needs to wait to return 1*/
int network_connect(network_ctrl_t *ctrl, const char *domain_name, uint32_t domain_name_len, luat_ip_addr_t *remote_ip, uint16_t remote_port, uint32_t timeout_ms);

/**
 * Listen as a server
 * @param ctrl network resources
 * @param timeout_ms timeout time
 * @return Success returns 0, failure < 0, non-blocking needs to wait to return 1*/
int network_listen(network_ctrl_t *ctrl, uint32_t timeout_ms);

/**
 * Close the connection
 * @param ctrl network resources
 * @param timeout_ms timeout time
 * @return Success returns 0, failure < 0, non-blocking needs to wait to return 1*/
int network_close(network_ctrl_t *ctrl, uint32_t timeout_ms);

/**
 * Send data to the peer
 * In UDP, remote_ip and remote_port are only needed when remote_ip and remote_port are inconsistent with connect.
 *TCP does not look at remote_ip and remote_port
 * In blocking mode, *tx_len does not need to be checked. In non-blocking mode, it is necessary to check whether the actual length of *tx_len is consistent with len.
 * @param ctrl network resources
 * @param data data to be sent
 * @param len the length of data to be sent
 * @param flags Send flags, generally write 0
 * @param remote_ip Peer IP
 * @param remote_port peer port
 * @param tx_len The length actually stored in the buffer area
 * @param timeout_ms timeout time
 * @return Success returns 0, failure < 0, non-blocking needs to wait to return 1*/
int network_tx(network_ctrl_t *ctrl, const uint8_t *data, uint32_t len, int flags, luat_ip_addr_t *remote_ip, uint16_t remote_port, uint32_t *tx_len, uint32_t timeout_ms);

/**
 * Read the received data
 * The actual amount of data read is in read_len. If it is UDP mode and server, you need to look at remote_ip and remote_port.
 * @param ctrl network resources
 * @param data The address where the read data is stored
 * @param len The length expected to be read
 * @param flags receive flags, generally write 0
 * @param remote_ip Peer IP
 * @param remote_port peer port
 * @param rx_len actual read length
 * @return Returns 0 on success, failure or socket has been disconnected < 0*/
int network_rx(network_ctrl_t *ctrl, uint8_t *data, uint32_t len, int flags, luat_ip_addr_t *remote_ip, uint16_t *remote_port, uint32_t *rx_len);

/**
 * Wait for network status changes
 * Will return any socket exception message received
 * If it is a blocking interface, out_event saves the message (copy, non-reference), and is_timeout saves whether it times out.
 * Returning 0 indicates data reception or timeout return, returning 1 indicates switching to non-blocking waiting, and others are network exceptions.
 * @param ctrl network resources
 * @param out_event network message
 * @param timeout_ms timeout time
 * @param is_timeout whether it times out
 * @return Success returns 0, failure < 0, non-blocking needs to wait to return 1*/
int network_wait_event(network_ctrl_t *ctrl, OS_EVENT *out_event, uint32_t timeout_ms, uint8_t *is_timeout);

/*

 */
/**
 * Waiting for data to be received
 * When a socket exception is received, the user sends EV_NW_BREAK_WAIT, or there is new data and will exit. If it is other messages, use the callback function entered in network_init_ctrl. If there is no callback function, it will be discarded directly.
 * When timeout_ms=0, the interface is still blocked and waits forever.
 * Returning 0 indicates data reception, user interruption or timeout return, others are network abnormalities.
 * User interrupt, is_break = 1, timeout is_timeout = 1
 * @param ctrl network resources
 * @param timeout_ms timeout time
 * @param is_break Whether it is a user interruption
 * @param is_timeout whether it times out
 * @return Returns 0 on success, < 0 on failure*/
int network_wait_rx(network_ctrl_t *ctrl, uint32_t timeout_ms, uint8_t *is_break, uint8_t *is_timeout);
/****************************Advanced API end************************ ************************************************/

/**
 * Obtain the complete address information of the network card
 * @param ctrl network resource, if NULL is written, see adapter_index below
 * @param adapter_index Network adapter serial number NW_ADAPTER_INDEX_XXX
 * @param ip IPV4 address
 * @param submask subnet mask
 * @param gateway gateway
 * @param ipv6 IPV6 address
 * @return Returns 0 if successful, otherwise failed*/
int network_get_full_local_ip_info(network_ctrl_t *ctrl, uint8_t adapter_index, luat_ip_addr_t *ip, luat_ip_addr_t *submask, luat_ip_addr_t *gateway, luat_ip_addr_t *ipv6);

/**
 * Set the IP to an invalid state
 * @param ip*/
void network_set_ip_invaild(luat_ip_addr_t *ip);

/**
 * Check whether the IP is invalid
 * @param ip
 * @return If invalid, return 0, others are valid*/
uint8_t network_ip_is_vaild(luat_ip_addr_t *ip);

/**
 * Check whether the IP is of IPV6 type
 * @param ip
 * @return does not return 0*/
uint8_t network_ip_is_ipv6(luat_ip_addr_t *ip);

/**
 * Check whether the IP is a valid IPV4 type
 * @param ip
 * @return does not return 0*/
uint8_t network_ip_is_vaild_ipv4(luat_ip_addr_t *ip);
/**
 * Set IP to IPV4
 * @param The ip address that needs to be set
 * @param ipv4 ipv4*/
void network_set_ip_ipv4(luat_ip_addr_t *ip, uint32_t ipv4);
#endif
// #endif
