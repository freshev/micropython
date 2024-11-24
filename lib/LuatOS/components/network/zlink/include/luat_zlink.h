#ifndef LUAT_ZLINK_H
#define LUAT_ZLINK_H

typedef void 	(*luat_zlink_output)(void *arg, const void *data, size_t len);

int luat_pcap_init(luat_zlink_output output, void *arg);
int luat_pcap_write(const void *data, size_t len);
int luat_pcap_push(const void *data, size_t len);

typedef struct luat_zlink_pkg
{
    uint8_t magic[4]; // ZINK
    uint8_t pkgid[4];   // package id, auto-increment
    uint8_t flags[2];   // Flag bit, reserved, currently 0
    uint8_t len[2];    //Only calculate the data after len
    uint8_t cmd0;    //Command classification
    uint8_t cmd1;    //Specific command
    uint8_t data[0];
}luat_zlink_pkg_t;

enum {
    //Basic instructions
    LUAT_ZLINK_CMD_NONE = 0, // empty instruction
    LUAT_ZLINK_CMD_VERSION, // Query the protocol version number
    LUAT_ZLINK_CMD_PING,    //Heartbeat packet
    LUAT_ZLINK_CMD_PONG,    //Heartbeat packet response
    LUAT_ZLINK_CMD_REBOOT,  // Restart Modules

    LUAT_ZLINK_CMD_MSG = 64,     // Log output, used for debugging

    //WLAN command
    // WLAN basic commands
    LUAT_ZLINK_CMD_WLAN_INIT = (1 << 8) + 1,
    LUAT_ZLINK_CMD_WLAN_STATUS,
    //WLAN setting command
    LUAT_ZLINK_CMD_WLAN_SSID = (1 << 8) + 16,
    LUAT_ZLINK_CMD_WLAN_PASSWORD,
    LUAT_ZLINK_CMD_WLAN_MAC,

    //WLAN control command
    LUAT_ZLINK_CMD_WLAN_CONNECT = (1 << 8) + 32,
    LUAT_ZLINK_CMD_WLAN_DISCONNECT,
    LUAT_ZLINK_CMD_WLAN_SCAN,


    // MAC packet sending and receiving instructions, only sending and receiving (ACK), there should be no need for anything else.
    LUAT_ZLINK_CMD_MACPKG_SEND = (2 << 8) + 1,
    LUAT_ZLINK_CMD_MACPKG_ACK,
};

#endif
