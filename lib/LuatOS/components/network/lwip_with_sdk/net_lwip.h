/**
 * Try to adapt to lwip of third-party SDK*/
#ifdef __USE_SDK_LWIP__
#ifndef __NET_LWIP_H__
#define __NET_LWIP_H__
#ifndef __BSP_COMMON_H__
#include "c_common.h"
#endif
enum
{
	EV_LWIP_EVENT_START = USER_EVENT_ID_START + 0x2000000,
	EV_LWIP_SOCKET_TX,
	EV_LWIP_NETIF_INPUT,
	EV_LWIP_RUN_USER_API,
//	EV_LWIP_TCP_TIMER,
	EV_LWIP_COMMON_TIMER,
	EV_LWIP_SOCKET_RX_DONE,
	EV_LWIP_SOCKET_CREATE,
	EV_LWIP_SOCKET_CONNECT,
	EV_LWIP_SOCKET_DNS,
	EV_LWIP_SOCKET_DNS_IPV6,
	EV_LWIP_SOCKET_LISTEN,
	EV_LWIP_SOCKET_ACCPET,
	EV_LWIP_SOCKET_CLOSE,
	EV_LWIP_NETIF_LINK_STATE,
//	EV_LWIP_DHCP_TIMER,
//	EV_LWIP_FAST_TIMER,
	EV_LWIP_NETIF_SET_IP,
	EV_LWIP_NETIF_IPV6_BY_MAC,
};

void net_lwip_register_adapter(uint8_t adapter_index);
void net_lwip_init(void);
void net_lwip_set_dns_adapter(uint8_t adapter_index);
int net_lwip_check_all_ack(int socket_id);
void net_lwip_set_netif(uint8_t adapter_index, struct netif *netif);
struct netif * net_lwip_get_netif(uint8_t adapter_index);
void net_lwip_input_packets(struct netif *netif, struct pbuf *p);
void net_lwip_ping_response(struct netif *inp, struct pbuf *p, uint8_t type);
/** If you need to use a static IP, you need to set the IP first and then set the linkup
 * If you have set a static IP before and now want to use a dynamic IP, you need to delete the static IP first and then linkup
 * Once linked up, if static IP is not used, DHCP will be started
 * You cannot use DHCP to obtain the IP network card, you must set a static IP first! ! ! ! ! ! , such as GPRS*/
void net_lwip_set_link_state(uint8_t adapter_index, uint8_t updown);

/**Special for GPRS network card, user_data fills in adapter_index, do not go from network_adapter*/
int net_lwip_set_static_ip(ip_addr_t *ip, ip_addr_t *submask, ip_addr_t *gateway, ip_addr_t *ipv6, void *user_data);

void net_lwip_set_rx_fast_ack(uint8_t adapter_index, uint8_t onoff);
//Set the TCP receiving window size to affect the receiving speed. The larger the tcp_mss_num, the faster it will be, but it will also consume more RAM.
void net_lwip_set_tcp_rx_cache(uint8_t adapter_index, uint16_t tcp_mss_num);
//The message passed to the lwip task of the SDK needs to be sent by the sdk
void net_lwip_sdk_send_event(uint32_t id, uint32_t param1, uint32_t param2, uint32_t param3);
void net_lwip_do_event(OS_EVENT event);
//Whether it is within the lwip task of the SDK
uint8_t net_lwip_check_in_sdk_task(void);
#endif
#endif
