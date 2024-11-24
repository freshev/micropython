
#include "luat_base.h"
#include "luat_ulwip.h"
#include "luat_crypto.h"

#define LUAT_LOG_TAG "ulwip"
#include "luat_log.h"

#include "lwip/opt.h"
#include "lwip/udp.h"
#include "lwip/ip.h"
#include "lwip/ip_addr.h"

// -------------------------------------
// DHCP client logic
// -------------------------------------

static int ulwip_dhcp_client_run(ulwip_ctx_t* ctx, char* rxbuff, size_t len) {
    PV_Union uIP;
    // Check the status of dhcp
    dhcp_client_info_t* dhcp = ctx->dhcp_client;
    struct netif* netif = ctx->netif;

    Buffer_Struct rx_msg_buf = {0,0,0};
    Buffer_Struct tx_msg_buf = {0,0,0};
	uint32_t remote_ip = 0;
    int result = 0;

    if (rxbuff) {
        rx_msg_buf.Data = (uint8_t*)rxbuff;
        rx_msg_buf.Pos = len;
        rx_msg_buf.MaxLen = len;
    }

    // Check to see if the acquisition is successful
    if (DHCP_STATE_CHECK == dhcp->state) {
        uIP.u32 = dhcp->ip;
		LLOGD("Dynamic IP:%d.%d.%d.%d", uIP.u8[0], uIP.u8[1], uIP.u8[2], uIP.u8[3]);
		uIP.u32 = dhcp->submask;
		LLOGD("Subnet mask:%d.%d.%d.%d", uIP.u8[0], uIP.u8[1], uIP.u8[2], uIP.u8[3]);
		uIP.u32 = dhcp->gateway;
		LLOGD("Gateway:%d.%d.%d.%d", uIP.u8[0], uIP.u8[1], uIP.u8[2], uIP.u8[3]);
		LLOGD("Lease time: %u seconds", dhcp->lease_time);

        //Set to netif
        ip_addr_set_ip4_u32(&netif->ip_addr, dhcp->ip);
        ip_addr_set_ip4_u32(&netif->netmask, dhcp->submask);
        ip_addr_set_ip4_u32(&netif->gw,      dhcp->gateway);
        dhcp->state = DHCP_STATE_WAIT_LEASE_P1;
        if (rxbuff) {
            luat_heap_free(rxbuff);
        }
        ulwip_netif_ip_event(ctx);
        return 0;
    }
    result = ip4_dhcp_run(dhcp, rxbuff == NULL ? NULL : &rx_msg_buf, &tx_msg_buf, &remote_ip);
    if (rxbuff) {
        luat_heap_free(rxbuff);
    }
    if (result) {
        LLOGE("ip4_dhcp_run error %d", result);
        return 0;
    }
    if (!tx_msg_buf.Pos) {
        return 0; // No data needs to be sent
    }
    // Sent via UDP
    struct pbuf *p;
    struct pbuf *q;
    // LLOGD("The length of the DHCP packet to be sent is %d. The first 4 bytes are %02X%02X%02X%02X.", tx_msg_buf.Pos, 
    //     tx_msg_buf.Data[0], tx_msg_buf.Data[1], tx_msg_buf.Data[2], tx_msg_buf.Data[3]);
    p = pbuf_alloc(PBUF_TRANSPORT, tx_msg_buf.Pos, PBUF_RAM);
    char* data = (char*)tx_msg_buf.Data;
    for (q = p; q != NULL; q = q->next) {
        memcpy(q->payload, data, q->len);
        data += q->len;
    }
    data = p->payload;
    // LLOGI("dhcp payload len %d %02X%02X%02X%02X", p->tot_len, data[0], data[1], data[2], data[3]);
    udp_sendto_if(ctx->dhcp_pcb, p, IP_ADDR_BROADCAST, 67, netif);
    pbuf_free(p);
    return 0;
}

static int ulwip_dhcp_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port) {
    LLOGD("DHCP packet received (len=%d)", p->tot_len);
    ulwip_ctx_t *ctx = (ulwip_ctx_t *)arg;
    char* ptr = luat_heap_malloc(p->tot_len);
    if (!ptr) {
        return ERR_OK;
    }
    size_t offset = 0;
    do {
        memcpy(ptr + offset, p->payload, p->len);
        offset += p->len;
        p = p->next;
    } while (p);
    ulwip_dhcp_client_run(ctx, ptr, p->tot_len);
    // LLOGD("Pass DHCP packet");
    return ERR_OK;
}

static void ulwip_dhcp_client_run_proxy(void* ctx) {
    ulwip_dhcp_client_run((ulwip_ctx_t *)ctx, NULL, 0);
}

// timer callback, or called directly, arg is the index number of nets
static void dhcp_client_timer_cb(void *arg) {
    ulwip_ctx_t *ctx = (ulwip_ctx_t *)arg;
    // Simple defense
    if (ctx->dhcp_client == NULL || ctx->dhcp_enable == 0) {
        return;
    }
    #if NO_SYS
    ulwip_dhcp_client_run_proxy(ctx);
    #else
    tcpip_callback(ulwip_dhcp_client_run_proxy, ctx);
    #endif
}

void ulwip_dhcp_client_start(ulwip_ctx_t *ctx) {
    if (!ctx->dhcp_client) {
        ctx->dhcp_client = luat_heap_malloc(sizeof(dhcp_client_info_t));
        memset(ctx->dhcp_client, 0, sizeof(dhcp_client_info_t));
        memcpy(ctx->dhcp_client->mac, ctx->netif->hwaddr, 6);
        luat_crypto_trng((char*)&ctx->dhcp_client->xid, sizeof(ctx->dhcp_client->xid));
        sprintf_(ctx->dhcp_client->name, "airm2m-%02x%02x%02x%02x%02x%02x",
			ctx->dhcp_client->mac[0],ctx->dhcp_client->mac[1], ctx->dhcp_client->mac[2],
			ctx->dhcp_client->mac[3],ctx->dhcp_client->mac[4], ctx->dhcp_client->mac[5]);
        luat_rtos_timer_create(&ctx->dhcp_timer);
        ctx->dhcp_pcb = udp_new();
        ip_set_option(ctx->dhcp_pcb, SOF_BROADCAST);
        udp_bind(ctx->dhcp_pcb, IP4_ADDR_ANY, 68);
        udp_connect(ctx->dhcp_pcb, IP4_ADDR_ANY, 67);
        udp_recv(ctx->dhcp_pcb, ulwip_dhcp_recv, ctx);
    }
    ip_addr_set_any(0, &ctx->netif->ip_addr);
    ctx->dhcp_client->state = DHCP_STATE_DISCOVER;
    ctx->dhcp_client->discover_cnt = 0;
    if (!luat_rtos_timer_is_active(ctx->dhcp_timer))
    {
        luat_rtos_timer_start(ctx->dhcp_timer, 1000, 1, dhcp_client_timer_cb, ctx);
    }
    dhcp_client_timer_cb(ctx);
}

void ulwip_dhcp_client_stop(ulwip_ctx_t *ctx) {
    if (luat_rtos_timer_is_active(ctx->dhcp_timer)) {
        luat_rtos_timer_stop(ctx->dhcp_timer);
    }
}
