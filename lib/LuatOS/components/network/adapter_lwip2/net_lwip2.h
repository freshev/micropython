#ifndef __NET_LWIP_H__
#define __NET_LWIP_H__

#define MAX_SOCK_NUM 8
#include "luat_base.h"
#include "dns_def.h"
#include "luat_network_adapter.h"

typedef struct
{
	llist_head node;
	uint64_t tag;	//Considering the problem of socket reuse, tags must be used for comparison.
	luat_ip_addr_t ip;
	uint8_t *data;
	uint32_t read_pos;
	uint16_t len;
	uint16_t port;
	uint8_t is_sending;
	uint8_t is_need_ack;
}socket_data_t;

typedef struct
{
	uint64_t socket_tag;
	dns_client_t dns_client;
	socket_ctrl_t socket[MAX_SOCK_NUM];
	ip_addr_t ec618_ipv6;
	struct netif *lwip_netif[NW_ADAPTER_INDEX_LWIP_NETIF_QTY];
	CBFuncEx_t socket_cb;
	void *user_data;
	void *task_handle;
	uint32_t socket_busy;
	uint32_t socket_connect;
	uint8_t netif_network_ready[NW_ADAPTER_INDEX_LWIP_NETIF_QTY];
	// DNS related
	struct udp_pcb *dns_udp[NW_ADAPTER_INDEX_LWIP_NETIF_QTY];
	HANDLE dns_timer[NW_ADAPTER_INDEX_LWIP_NETIF_QTY];
	uint8_t next_socket_index;
}net_lwip2_ctrl_struct;


void net_lwip2_register_adapter(uint8_t adapter_index);
void net_lwip2_init(uint8_t adapter_index);
int net_lwip_check_all_ack(int socket_id);
void net_lwip2_set_netif(uint8_t adapter_index, struct netif *netif);
struct netif * net_lwip2_get_netif(uint8_t adapter_index);
/** If you need to use a static IP, you need to set the IP first and then set the linkup
 * If you have set a static IP before and now want to use a dynamic IP, you need to delete the static IP first and then linkup
 * Once linked up, if static IP is not used, DHCP will be started
 * You cannot use DHCP to obtain the IP network card, you must set a static IP first! ! ! ! ! ! , such as GPRS*/
void net_lwip2_set_link_state(uint8_t adapter_index, uint8_t updown);

#endif
