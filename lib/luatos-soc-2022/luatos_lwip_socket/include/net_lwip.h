#ifndef __NET_LWIP_H__
#define __NET_LWIP_H__
void net_lwip_register_adapter(uint8_t adapter_index);
void net_lwip_init(void);
int net_lwip_check_all_ack(int socket_id);
void net_lwip_set_netif(uint8_t adapter_index, struct netif *netif, void *init, uint8_t is_default);
struct netif * net_lwip_get_netif(uint8_t adapter_index);
void net_lwip_input_packets(struct netif *netif, struct pbuf *p);
/** If you need to use a static IP, you need to set the IP first and then set the linkup
 * If you have set a static IP before and now want to use a dynamic IP, you need to delete the static IP first and then linkup
 * Once linked up, if static IP is not used, DHCP will be started
 * You cannot use DHCP to obtain the IP network card, you must set a static IP first! ! ! ! ! ! , such as GPRS*/
void net_lwip_set_link_state(uint8_t adapter_index, uint8_t updown);

/**Special for GPRS network card, user_data fills in adapter_index, do not go from network_adapter*/
int net_lwip_set_static_ip(ip_addr_t *ip, ip_addr_t *submask, ip_addr_t *gateway, ip_addr_t *ipv6, void *user_data);

void net_lwip_set_rx_fast_ack(uint8_t adapter_index, uint8_t onoff);
ip_addr_t *net_lwip_get_ip6(void);
//Set the TCP receiving window size, which affects the receiving speed. The larger the tcp_mss_num is, the faster it is. It cannot be greater than 32 and cannot be less than 6.
void net_lwip_set_tcp_rx_cache(uint8_t adapter_index, uint16_t tcp_mss_num);

void net_lwip_check_switch(uint8_t onoff);
#endif
