
#include "luat_base.h"
#include "luat_ulwip.h"
#include "luat_crypto.h"

#define LUAT_LOG_TAG "ulwip"
#include "luat_log.h"

#include "lwip/ip.h"
#include "lwip/udp.h"
#include "lwip/tcp.h"
#include "luat_napt.h"

// IP packet input callback for EC618/EC7xx platform
static uint8_t napt_tmpbuff[1600];

uint8_t napt_target_adapter;

err_t ulwip_ip_input_cb(struct pbuf *p, struct netif *inp) {
    //LLOGD("IP packet received (len=%d)", p->tot_len);
    u8_t ipVersion;
    if (napt_target_adapter == 0) {
        // NAPT target adapter (intranet) is not registered, no forwarding is required
        return 1;
    }
    ipVersion = IP_HDR_GET_VERSION(p->payload);
    if (ipVersion != 4) {
        // LLOGD("Only forward IPv4 packets, ignore %p %d", p, p->tot_len);
        return 1; // Currently only IPv4 forwarding is considered
    }
    // If the package is too large, it will not be processed.
    if (p->tot_len > sizeof(napt_tmpbuff) - 14) {
        LLOGD("IP packet is too large, ignore %p %d", p, p->tot_len);
        return 1;
    }
    int offset = 14; // MAC header length, the incoming luat_napt_input needs to be a MAC packet
    struct pbuf *q;
    for (q = p; q != NULL; q = q->next) {
        memcpy(napt_tmpbuff + offset, q->payload, q->len);
        offset += q->len;
    }
    struct ip_hdr *ip = (struct ip_hdr *)(napt_tmpbuff + 14);
    if (ip->_proto == IP_PROTO_TCP) {
        //TCP protocol
        struct tcp_hdr *tcp = (struct tcp_hdr *)((char*)ip + sizeof(struct ip_hdr));
        u16_t tcpPort = tcp->dest;
        if(!luat_napt_port_is_used(tcpPort)) {
            // The target port is not occupied and does not need to be forwarded.
            // LLOGD("The destination port of the IP packet is not used, ignore %p %d", p, p->tot_len);
            return 1;
        }
    }
    else if (ip->_proto == IP_PROTO_UDP) {
        // UDP protocol
        struct udp_hdr *udp = (struct udp_hdr *)((char*)ip + sizeof(struct ip_hdr));
        u16_t udpPort = udp->dest;
        if(!luat_napt_port_is_used(udpPort)) {
            // The target port is not occupied and does not need to be forwarded.
            // LLOGD("The destination port of the IP packet is not used, ignore %p %d", p, p->tot_len);
            return 1;
        }
    }
    // If netif has been registered, it will be forwarded.
    struct netif* tmp = ulwip_find_netif(NW_ADAPTER_INDEX_LWIP_GPRS);
    struct netif* gw = ulwip_find_netif(napt_target_adapter);
    if (gw && tmp == inp) {
        int rc = luat_napt_input(0, napt_tmpbuff, q->tot_len + 14, &gw->ip_addr);
        // LLOGD("luat_napt_input %d", rc);
        if (rc == 0) {
            char* ptr = luat_heap_malloc(p->tot_len + 14);
            if (ptr == NULL) {
                LLOGE("The IP packet transformation is completed, but the system has insufficient memory and cannot be forwarded to the Lua layer.");
                return 4;
            }
            memcpy(napt_tmpbuff + 6, gw->hwaddr, 6);
            napt_tmpbuff[12] = 0x08;
            napt_tmpbuff[13] = 0x00;
            memcpy(ptr, napt_tmpbuff, p->tot_len + 14);
            rtos_msg_t msg = {0};
            msg.handler = l_ulwip_netif_output_cb;
            msg.arg1 = p->tot_len + 14;
            msg.arg2 = NW_ADAPTER_INDEX_LWIP_WIFI_AP;
            msg.ptr = ptr;
            luat_msgbus_put(&msg, 0);
            return ERR_OK;
        }
    }
    else {
        // LLOGD("The forwarding target does not exist, skip the NAPT process %p %d", p, p->tot_len);
    }
    return 2;
}
