#include "luat_base.h"
#include "luat_pcap.h"
#include "luat_mcu.h"

typedef struct luat_pcap_ctx {
    luat_pcap_output output;
    void *arg;
}luat_pcap_ctx_t;

/*pcap file header structure*/
typedef struct pcap_file_header {
    uint32_t magic;         /*Identifies the beginning of the file, used to identify the byte order*/
    uint16_t version_major; /*Major version number*/
    uint16_t version_minor; /*minor version number*/
    uint32_t r0;
    uint32_t r1;
    uint32_t snaplen;       /*maximum capture length*/
    uint32_t linktype;      /*Link type*/
}pcap_file_header_t;

/*pcap packet header structure*/
typedef struct pcap_pkthdr {
    uint32_t ts_sec;        /*Timestamp (seconds)*/
    uint32_t ts_usec;       /*Timestamp (microseconds)*/
    uint32_t caplen;        /*Capture length*/
    uint32_t len;           /*actual length*/
}pcap_pkthdr_t;


static luat_pcap_ctx_t ctx;

int luat_pcap_init(luat_pcap_output output, void *arg) {
    ctx.output = output;
    ctx.arg = arg;
    return 0;
}
int luat_pcap_write_head(void) {
    pcap_file_header_t head = {0};
    head.magic = 0xa1b2c3d4;
    head.version_major = 2;
    head.version_minor = 4;
    head.snaplen = 65535; // 64k
    head.linktype = 1; // 1: ethernet
    ctx.output(ctx.arg, &head, sizeof(pcap_file_header_t));
    return 0;
}

int luat_pcap_write_macpkg(const void *data, size_t len) {
    uint64_t ts = luat_mcu_tick64_ms();
    pcap_pkthdr_t head = {0};
    head.ts_sec = ts / 1000;
    head.ts_usec = (ts % 1000) * 1000;
    head.caplen = len;
    head.len = len;
    ctx.output(ctx.arg, &head, sizeof(pcap_pkthdr_t));
    ctx.output(ctx.arg, data, len);
    return 0;
}
