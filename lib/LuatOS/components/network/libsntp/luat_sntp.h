#ifndef LUAT_SNTP_H
#define LUAT_SNTP_H

#include "luat_network_adapter.h"

#define SNTP_SERVER_COUNT       3
#define SNTP_SERVER_LEN_MAX     32

#define NTP_UPDATE 1
#define NTP_ERROR  2
#define NTP_TIMEOUT 3

// 2 seconds timeout, give each server a chance
#define NTP_TIMEOUT_MS    (2000)
#define NTP_RESP_SIZE     (44)
#define NTP_NETWORK_DELAY_CMAX (8)

typedef struct sntp_msg {
  uint8_t  li_vn_mode;
  uint8_t  stratum;
  uint8_t  poll;
  uint8_t  precision;
  uint32_t root_delay;
  uint32_t root_dispersion;
  uint32_t reference_identifier;
  uint32_t reference_timestamp[2];
  uint32_t originate_timestamp[2];
  uint32_t receive_timestamp[2];
  uint32_t transmit_timestamp[2];
} sntp_msg_t;


typedef struct sntp_ctx
{
    network_ctrl_t *ctrl; // Used to connect to the network layer
    size_t next_server_index; // Because there are multiple servers, we try them one by one here.
    int is_running;       // Mark whether it is running, if so, do not start a new one
    luat_rtos_timer_t timer;
    int timer_running;
    uint64_t sysboot_diff_sec;
    int32_t sysboot_diff_ms;
    uint32_t network_delay_ms;
    uint16_t sntp_debug;
    uint16_t ndelay_c;
    uint32_t ndelay_array[NTP_NETWORK_DELAY_CMAX];
    uint16_t port;
}sntp_ctx_t;

int ntp_get(int adapter_index);
void ntp_cleanup(void);
int l_sntp_get(lua_State *L);
int l_sntp_tm(lua_State *L);
int l_sntp_port(lua_State *L);

#endif