#include "string.h"
#include "iot_os.h"
#include "iot_debug.h"
#include "iot_network.h"
#include "iot_socket.h"

#define SOCKET_MSG_NETWORK_READY (0)
#define SOCKET_MSG_NETWORK_LINKED (1)
typedef struct
{
    UINT8 type;
    UINT8 data;
} DEMO_SOCKET_MESSAGE;

#define socket_dbg iot_debug_print

#define DEMO_SERVER_TCP_IP "121.40.198.143"
#define DEMO_SERVER_TCP_PORT 12415

#define DEMO_SERVER_UDP_IP "121.40.170.41"
#define DEMO_SERVER_UDP_PORT 12414

static HANDLE g_s_socket_task;

static int demo_socket_tcp_recv(int socketfd)
{
    unsigned char recv_buff[64] = {0};
    int recv_len;
    // TCP ½ÓÊÜÊý¾Ý
    recv_len = recv(socketfd, recv_buff, sizeof(recv_buff), 0);
    socket_dbg("[coreTest-socket]: tcp recv result %d data %s", recv_len, recv_buff);
    return recv_len;
}

static int demo_socket_tcp_send(int socketfd)
{
    int send_len;
    // TCP ·¢ËÍÊý¾Ý
    send_len = send(socketfd, "hello i'm client", strlen("hello i'm client"), 0);
    socket_dbg("[coreTest-socket]: tcp send [hello i'm client] result = %d", send_len);
    return send_len;
}

static int demo_socket_tcp_connect_server(void)
{
    int socketfd;
    int connErr;
    struct openat_sockaddr_in tcp_server_addr;

    // ´´½¨tcp socket
    socketfd = socket(OPENAT_AF_INET, OPENAT_SOCK_STREAM, 0);
    if (socketfd < 0)
    {
        socket_dbg("[coreTest-False-socket]: create tcp socket error");
        return -1;
    }

    socket_dbg("[coreTest-socket]: create tcp socket success");

    // ½¨Á¢TCPÁ´½Ó
    memset(&tcp_server_addr, 0, sizeof(tcp_server_addr)); // ³õÊ¼»¯·þÎñÆ÷µØÖ·
    tcp_server_addr.sin_family = OPENAT_AF_INET;
    tcp_server_addr.sin_port = htons((unsigned short)DEMO_SERVER_TCP_PORT);
    inet_aton(DEMO_SERVER_TCP_IP, &tcp_server_addr.sin_addr);

    socket_dbg("[coreTest-socket]: tcp connect to addr %s", DEMO_SERVER_TCP_IP);
    connErr = connect(socketfd, (const struct sockaddr *)&tcp_server_addr, sizeof(struct openat_sockaddr));

    if (connErr < 0)
    {
        socket_dbg("[coreTest-False-socket]: tcp connect error %d", socket_errno(socketfd));
        close(socketfd);
        return -1;
    }
    socket_dbg("[coreTest-socket]: tcp connect success");

    return socketfd;
}

static void demo_socket_tcp_client()
{
    static int socketfd = -1;
    if (socketfd < 0)
        socketfd = demo_socket_tcp_connect_server();
    if (socketfd >= 0)
    {
        int ret = demo_socket_tcp_send(socketfd);
        if (ret < 0)
            socket_dbg("[coreTest-False-socket]: send last error %d", socket_errno(socketfd));
        ret = demo_socket_tcp_recv(socketfd);
        if (ret < 0)
            socket_dbg("[coreTest-False-socket]: recv error %d", socket_errno(socketfd));
    }
    if (socketfd < 0)
        close(socketfd);
}

static int demo_socket_udp_send(int socketfd)
{
    int send_len;
    struct openat_sockaddr_in udp_server_addr;

    memset(&udp_server_addr, 0, sizeof(udp_server_addr)); // ³õÊ¼»¯·þÎñÆ÷µØÖ·
    udp_server_addr.sin_family = OPENAT_AF_INET;
    udp_server_addr.sin_port = htons((unsigned short)DEMO_SERVER_UDP_PORT);
    inet_aton(DEMO_SERVER_UDP_IP, &udp_server_addr.sin_addr);

    // UDP ·¢ËÍÊý¾Ý
    send_len = sendto(socketfd, "hello i'm client", strlen("hello i'm client"), 0, (struct sockaddr *)&udp_server_addr, sizeof(struct openat_sockaddr));
    socket_dbg("[coreTest-socket]: udp send [hello i'm client] result = %d", send_len);
    return send_len;
}

static int demo_socket_udp_recv(int socketfd)
{
    unsigned char recv_buff[64] = {0};
    int recv_len;
    openat_socklen_t udp_server_len;

    struct openat_sockaddr_in udp_server_addr;

    memset(&udp_server_addr, 0, sizeof(udp_server_addr)); // ³õÊ¼»¯·þÎñÆ÷µØÖ·
    udp_server_addr.sin_family = OPENAT_AF_INET;
    udp_server_addr.sin_port = htons((unsigned short)DEMO_SERVER_UDP_PORT);
    inet_aton(DEMO_SERVER_UDP_IP, &udp_server_addr.sin_addr);
    udp_server_len = sizeof(udp_server_addr);

    // UDP ½ÓÊÜÊý¾Ý
    recv_len = recvfrom(socketfd, recv_buff, sizeof(recv_buff), 0, (struct sockaddr *)&udp_server_addr, &udp_server_len);
    socket_dbg("[coreTest-socket]: udp recv result %d data %s", recv_len, recv_buff);

    return recv_len;
}

static void demo_socket_udp_client()
{
    static int socketfd = -1;
    if (socketfd < 0)
    {
        socketfd = socket(OPENAT_AF_INET, OPENAT_SOCK_DGRAM, 0);
        if (socketfd < 0)
        {
            socket_dbg("[coreTest-False-socket]: create udp socket error");
            return;
        }
    }
    // ´´½¨tcp socket
    socket_dbg("[coreTest-socket]: create udp socket success");

    int ret = demo_socket_udp_send(socketfd);
    if (ret < 0)
    {
        int err = socket_errno(socketfd);
        socket_dbg("[coreTest-False-socket]: send last error %d", err);
    }
    //×èÈû¶ÁÈ¡
    ret = demo_socket_udp_recv(socketfd);
    if (ret <= 0)
        socket_dbg("[coreTest-False-socket]: recv error %d", socket_errno(socketfd));
    if (socketfd < 0)
    {
        close(socketfd);
    }
}

static void demo_gethostbyname(void)
{
    //ÓòÃû½âÎö

    char *name = "www.baidu.com";
    struct openat_hostent *hostentP = NULL;
    char *ipAddr = NULL;

    // »Đè¡ ¢ ¢
    hostentP = gethostbyname(name);

    if (!hostentP)
    {
        socket_dbg("[coreTest-False-socket]: gethostbyname %s fail", name);
        return;
    }

    // ½«ip×ª»»³É×Ö·û´®
    ipAddr = ipaddr_ntoa((const openat_ip_addr_t *)hostentP->h_addr_list[0]);

    socket_dbg("[coreTest-socket]: gethostbyname %s ip %s", name, ipAddr);
}

HANDLE tcp_client_handle = NULL;
void socketTest(char *mode)
{
    extern bool networkstatus;
    if (networkstatus == FALSE)
        return FALSE;
    if (strcmp(mode, "DNS") == 0)
        demo_gethostbyname();
    else if (strcmp(mode, "UDP") == 0)
        demo_socket_udp_client();
    else if (strcmp(mode, "TCP") == 0)
        demo_socket_tcp_client();
}
