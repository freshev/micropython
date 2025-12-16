/*
 * @Author: your name
 * @Date: 2020-05-19 14:05:32
 * @LastEditTime: 2020-05-26 19:30:56
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \RDA8910_CSDK\USER\user_main.c
 */

#include "string.h"
#include "cs_types.h"
#include "osi_log.h"
#include "osi_api.h"
#include "am_openat.h"
#include "am_openat_vat.h"
#include "am_openat_common.h"
#include "iot_debug.h"
#include "iot_uart.h"
#include "iot_os.h"
#include "iot_gpio.h"
#include "iot_pmd.h"
#include "iot_adc.h"
#include "iot_vat.h"
#include "iot_network.h"
#include "iot_socket.h"
#include "iot_vat.h"

int Rtime;
/*Test URL: http://tcplab.openluat.com/
Get the new IP address and port number and replace the following TCP_SERVER_IP and TCP_SERVER_PORT and download this program
Wait for http://tcplab.openluat.com/ to display the data sent by the air Modules (RTIME)[]
Test results:
AIR724UG Development Board Mobile Card Indoor
Data loop sending cycle 60s test time 20min
When RTIME=1, the average current=0.018241
When RTIME=0, the average current = 0.039139*/
#define TCP_SERVER_IP "180.97.81.180"
#define TCP_SERVER_PORT 56797

HANDLE TestTask_HANDLE = NULL;
uint8 NetWorkCbMessage = 0;
int socketfd = -1;
int cout_res = -1;
char str_res[2048];

static void SentTask(void *param)
{
    uint8 num = 0;
    int len = 0;
    char data[512] = {0};
    while (1)
    {
        if (socketfd >= 0)
        {
            // len = sprintf(data, "RDA8910 Sent:%d and res %d and %s", num,cout_res,str_res);
            len = sprintf(data, "RDA8910 Sent:%d and AT*RTIME?= %d ", num, Rtime);
            // len = sprintf(data, "RDA8910 Sent:%d", num);
            data[len] = '\0';
            iot_debug_print("[socket]---", data);
            if (len > 0)
            {
                // TCP sends data
                len = send(socketfd, data, len + 1, 0);
                if (len < 0)
                {
                    iot_debug_print("[socket] tcp send data False");
                }
                else
                {
                    iot_debug_print("[socket] tcp send data Len = %d", len);
                    num += 1;
                }
            }
        }
        // iot_pmd_enter_deepsleep();
        iot_os_sleep(60000);
    }
}

static void RecvTask(void *param)
{
    int len = 0;
    unsigned char data[512] = {0};
    while (1)
    {
        if (socketfd >= 0)
        {
            // TCP accepts data
            len = recv(socketfd, data, sizeof(data), 0);
            if (len < 0)
            {
                iot_debug_print("[socket] tcp send data False");
            }
            else
            {
                iot_debug_print("[socket] tcp Recv data result = %s", data);
            }
        }
    }
}
static void TcpConnect()
{
    /*Create a socket
        AF_INET (IPV4 network protocol)
        Support SOCK_STREAM/SOCK_DGRAM, respectively, representing TCP and UDP connections.*/

    socketfd = socket(OPENAT_AF_INET, OPENAT_SOCK_STREAM, 0);
    while (socketfd < 0)
    {
        iot_debug_print("[socket] create tcp socket error");
        iot_os_sleep(3000);
    }
    // Establish a TCP connection
    struct openat_sockaddr_in tcp_server_addr = {0};
    //The purpose of AF_INET is to use IPv4 to communicate
    tcp_server_addr.sin_family = OPENAT_AF_INET;
    //Remote port, host byte order is converted into network byte order
    tcp_server_addr.sin_port = htons((unsigned short)TCP_SERVER_PORT);
    //Convert the string remote IP into the network sequence IP
    inet_aton(TCP_SERVER_IP, &tcp_server_addr.sin_addr);
    iot_debug_print("[socket] tcp connect to addr %s", TCP_SERVER_IP);
    int connErr = connect(socketfd, (const struct openat_sockaddr *)&tcp_server_addr, sizeof(struct openat_sockaddr));
    if (connErr < 0)
    {
        iot_debug_print("[socket] tcp connect error %d", socket_errno(socketfd));
        close(socketfd);
    }
    iot_debug_print("[socket] tcp connect success");
    iot_os_create_task(SentTask, NULL, 2048, 10, OPENAT_OS_CREATE_DEFAULT, "SentTask");
    // iot_os_create_task(RecvTask, NULL, 2048, 10, OPENAT_OS_CREATE_DEFAULT, "RecvTask");
}

static void TestTask(void *param)
{
    bool NetLink = FALSE;
    while (NetLink == FALSE)
    {
        T_OPENAT_NETWORK_CONNECT networkparam = {0};
        switch (NetWorkCbMessage)
        {
        case OPENAT_NETWORK_DISCONNECT: //The network is disconnected. It means that the GPRS network is not available and data connection cannot be made. You may be able to make a phone call.
            iot_debug_print("[socket] OPENAT_NETWORK_DISCONNECT");
            iot_os_sleep(10000);
            break;
        case OPENAT_NETWORK_READY: //The network is connected. It means that the GPRS network is available and link activation can be performed.
            iot_debug_print("[socket] OPENAT_NETWORK_READY");
            memcpy(networkparam.apn, "CMNET", strlen("CMNET"));
            // Establish a network connection, which is actually the pdp activation process
            iot_network_connect(&networkparam);
            iot_os_sleep(500);
            break;
        case OPENAT_NETWORK_LINKED: //The link has been activated. PDP has been activated. Data connection can be established through the socket interface.
            iot_debug_print("[socket] OPENAT_NETWORK_LINKED");
            NetLink = TRUE;
            break;
        }
    }
    if (NetLink == TRUE)
    {
        TcpConnect();
    }
    iot_os_delete_task(TestTask_HANDLE);
}

static void NetWorkCb(E_OPENAT_NETWORK_STATE state)
{
    NetWorkCbMessage = state;
}

static AtCmdRsp AtCmdCb_res(char *pRspStr)
{
    iot_debug_print("[vat]AtCmdCb_csq");
    // rspValue: status returned by AT
    AtCmdRsp rspValue = AT_RSP_WAIT;
    // AT instruction result lookup table
    char *rspStrTable[] = {"+CME ERROR", "*RTIME: ", "OK"};
    s16 rspType = -1;
    u8 i = 0;
    char *p = pRspStr + 2;
    strcat(str_res, pRspStr);
    for (i = 0; i < sizeof(rspStrTable) / sizeof(rspStrTable[0]); i++)
    {
        if (!strncmp(rspStrTable[i], p, strlen(rspStrTable[i])))
        {
            rspType = i;
            //Get the value returned by AT*RTIME?
            if (rspType == 1)
            {
                Rtime = STR_TO_INT(p[strlen(rspStrTable[rspType])]);
            }
            break;
        }
    }
    // Determine the status of AT return
    switch (rspType)
    {
    case 0: /* ERROR */
        rspValue = AT_RSP_ERROR;
        break;

    case 1: /* +CSQ */
        rspValue = AT_RSP_WAIT;
        break;

    case 2: /* OK */
        cout_res++;
        break;

    default:
        break;
    }
    return rspValue;
}

/**/
VOID luat_ATCmdSend(VOID)
{
    AtCmdEntity atCmdInit[] = {
        {AT_CMD_DELAY "2000", 10, NULL},
        {"AT+WAKETIM=1" AT_CMD_END, 14, NULL},
        {"AT^TRACECTRL=0,0,0" AT_CMD_END, 20, NULL},
        {"AT*RTIME=1" AT_CMD_END, 12, NULL},
        /*{"AT*RTIME=0" AT_CMD_END, 12, NULL},//Change the value of RTIME to adapt to different network environments*/
        {"AT*RTIME?" AT_CMD_END, 12, AtCmdCb_res},//RTIME query command

    };
    //Batch execution of AT commands Parameters: AT command parameters Number of AT commands
    iot_vat_push_cmd(atCmdInit, sizeof(atCmdInit) / sizeof(atCmdInit[0]));
}

//main function
int appimg_enter(void *param)
{
    //↓ Must be placed in front of iot_network_set_cb, there must be no delay.
    iot_network_set_cb(NetWorkCb);
    iot_vat_send_cmd("AT^TRACECTRL=0,1,1\r\n", sizeof("AT^TRACECTRL=0,1,1\r\n"));

    iot_debug_print("[socket] ENTER");
    //Open optimization Note: Turning off optimization cannot simply close the function, you need to set RTIME=0
    luat_ATCmdSend();

    //System sleep
    iot_os_sleep(5000);
    iot_debug_print("[socket] SLEEP_OVER");
    //Register network status callback function
    iot_debug_print("[socket] SET_CB_OVER");
    //Create a task
    TestTask_HANDLE = iot_os_create_task(TestTask, NULL, 2048, 10, OPENAT_OS_CREATE_DEFAULT, "TestTask");
    return 0;
}

//Exit prompt
void appimg_exit(void)
{
    OSI_LOGI(0, "application image exit");
}
