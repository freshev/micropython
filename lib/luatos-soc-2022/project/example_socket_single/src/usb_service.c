#include "luat_rtos.h"
#include "luat_debug.h"
#include "luat_uart.h"
#include "luat_mem.h"
#include "socket_service.h"

/*Function Modules name: USB and host computer communication;
The main business logic is:
1. After the device is powered on, three ports will be enumerated on the computer. The third port can be used for data communication with the serial port tool on the PC;
2. The PC serial port tool sends the port,<%d>\r\n command to modify the server port number in the socket_service function Modules;
   For example, port,1234\r\n means changing the port number of the server to be connected to 1234;*/


// static luat_rtos_task_handle usb_task_handle;


//Only for test use, the writing is not robust enough
#define USB_RECV_BUFFER_SIZE 256
static uint8_t g_s_recv_buff[USB_RECV_BUFFER_SIZE+1];
static uint32_t g_s_recved_len = 0;

static void parse_recv_data(char* data, uint32_t len)
{
    if(len > (USB_RECV_BUFFER_SIZE-g_s_recved_len))
    {
        LUAT_DEBUG_PRINT("recv len exceed max size, error");
        goto exit;
    }

    memcpy(g_s_recv_buff+g_s_recved_len, data, len);
    g_s_recved_len += len;
    *(g_s_recv_buff+len) = 0;

    if(strstr((const char *)g_s_recv_buff, "\r\n"))
    {
        int port;

        if(sscanf(g_s_recv_buff, "port,%d", &port))
        {
            socket_service_set_remote_port(port);
            luat_uart_write(LUAT_VUART_ID_0, "\r\nOK\r\n", 6);
        }
        else
        {
            luat_uart_write(LUAT_VUART_ID_0, "\r\nERROR\r\n", 9);
        }

        goto exit;
    }
    else
    {
        return;
    }

exit:
    memset(g_s_recv_buff, 0, sizeof(g_s_recv_buff));
    g_s_recved_len = 0;
    return;
}

static void usb_recv_cb(int uart_id, uint32_t data_len){
    char* data_buff = LUAT_MEM_MALLOC(data_len+1);
    memset(data_buff,0,data_len+1);
    luat_uart_read(uart_id, data_buff, data_len);
    LUAT_DEBUG_PRINT("uart_id:%d data:%s data_len:%d",uart_id,data_buff,data_len);
    parse_recv_data(data_buff, data_len);
    LUAT_MEM_FREE(data_buff);
}


void usb_service_init(void)
{
    luat_uart_t uart = {
        .id = LUAT_VUART_ID_0,
    };
    luat_uart_setup(&uart);
    luat_uart_ctrl(LUAT_VUART_ID_0, LUAT_UART_SET_RECV_CALLBACK, usb_recv_cb);
}


