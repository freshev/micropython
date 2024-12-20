#include "sockets.h"
#include "dns.h"
#include "lwip/ip4_addr.h"
#include "netdb.h"
#include "luat_debug.h"
#include "luat_rtos.h"
#include "luat_mem.h"
#include "network_service.h"
#include "socket_service.h"

/*Function Modules name: socket long connection, data sending and receiving framework;
The main business logic is:
1. After powering on, start a task and wait for the network_service function Modules to detect that the IP network is ready;
   Then connect to the server with address REMOTE_SERVER_ADDRESS and port g_s_remote_server_port through tcp;
2. After the server connection is successful, create a data sending task, tcp_send_task_proc;
   In this task, each piece of data is read from the queue that stores the data to be sent, and sent to the server in turn;
3. After the server connection is successful, create a data receiving task, tcp_recv_task_proc;
   In this task, use the select mechanism to monitor readable and abnormal events;
4. In the tasks of data sending and data receiving, if any exception occurs, the socket connection will be actively disconnected and reconnected after 5 seconds.

If users do not have their own test server, they can visit https://netlab.luatos.com/ and open a TCP service. The port number of the TCP service is randomly assigned;
After the port number is determined, directly modify the g_s_remote_server_port variable value in this file;
You can also directly burn this example without modification. After the device is running, the port number can be dynamically modified by communicating with the PC serial port tool through the usb_service function Modules.*/

typedef struct socket_service_send_data
{
    char *data;
    uint32_t len;
    socket_service_send_data_callback_t callback_func;
    uint32_t callback_param;
}socket_service_send_data_t;



#define REMOTE_SERVER_ADDRESS "112.125.89.8"
// #define REMOTE_SERVER_PORT 34410
static int g_s_remote_server_port = 37954;

static luat_rtos_task_handle g_s_tcp_connect_task_handle, g_s_tcp_send_task_handle, g_s_tcp_recv_task_handle;
static luat_rtos_semaphore_t g_s_connect_ok_semaphore;

//socket id, connection status, connection task existence status
static int g_s_socket_id = -1;
static uint8_t g_s_is_connected = 0;
static uint8_t g_s_is_tcp_connect_task_exist = 0;



//socket data sending task mailbox length
#define TCP_SEND_TASK_MAILBOX_EVENT_MAX_SIZE 50


void socket_service_set_remote_port(int port)
{
	g_s_remote_server_port = port;
}


static void close_socket(void)
{
	LUAT_DEBUG_PRINT("enter");
	close(g_s_socket_id);
	g_s_socket_id = -1;
	g_s_is_connected = 0;
}


#define RECV_BUF_LEN 1024

static void tcp_recv_task_proc(void *arg)
{	
	fd_set read_set, error_set;
	struct timeval timeout;
	int ret, read_len;
	char * recv_buf = NULL;

	LUAT_DEBUG_PRINT("enter");

	while(1)
	{
		if(g_s_is_connected)
		{
			luat_rtos_semaphore_take(g_s_connect_ok_semaphore, LUAT_NO_WAIT);

			if(NULL == recv_buf)
			{
				recv_buf = (char *)LUAT_MEM_MALLOC(RECV_BUF_LEN);
				LUAT_DEBUG_ASSERT(recv_buf != NULL,"malloc recv_buf fail");
			}

			timeout.tv_sec = 60;
			timeout.tv_usec = 0;

			while(1)
			{
				FD_ZERO(&read_set);
				FD_ZERO(&error_set);
				FD_SET(g_s_socket_id,&read_set);
				FD_SET(g_s_socket_id,&error_set);

				LUAT_DEBUG_PRINT("before select");

				ret = select(g_s_socket_id+1, &read_set, NULL, &error_set, &timeout);

				if(ret < 0)
				{
					//fail
					LUAT_DEBUG_PRINT("select fail, ret %d",ret);
					break;
				}
				else if(ret == 0)
				{
					//time out
					LUAT_DEBUG_PRINT("select timeout");
				}
				else
				{
					if(FD_ISSET(g_s_socket_id, &error_set))
					{
						//Error
						LUAT_DEBUG_PRINT("select error event");
						break;
					}
					else if(FD_ISSET(g_s_socket_id, &read_set))
					{
						LUAT_DEBUG_PRINT("select read event");
						read_len = recv(g_s_socket_id, recv_buf, RECV_BUF_LEN, 0);
						//After the read event, the data cannot be read for the first time, indicating an error.
						//In actual measurement, when selecting, the server actively disconnects and will go here
						if(read_len <= 0)
						{
							LUAT_DEBUG_PRINT("select read event error");
							break;
						}
						else
						{
							do
							{
								LUAT_DEBUG_PRINT("recv %d bytes data from server",read_len);
								if(read_len > 0)
								{
									//The data is read, in recv_buf, the length is read_len
								}

								read_len = recv(g_s_socket_id, recv_buf, RECV_BUF_LEN, 0);
							}while(read_len > 0);
						}					

					}
					else
					{
						LUAT_DEBUG_PRINT("select other socket event");
					}
				}
			}

			close_socket();			

			luat_rtos_task_sleep(5000);
			socket_service_init();					
		}
		else
		{
			//wait for connect ok
			LUAT_DEBUG_PRINT("wait connect ok semaphore");
			luat_rtos_semaphore_take(g_s_connect_ok_semaphore, 1000);
		}
	}
	
}

//The result of synchronous insertion into the queue is judged by the return value: 0 is successful, others fail.
//After the insertion is successful, the asynchronous sending result is judged by the result in callback_func(int result, void *callback_param): 0 is successful, others fail.
int socket_service_send_data(const char *data, uint32_t len, socket_service_send_data_callback_t callback_func, uint32_t callback_param)
{
	if(data==NULL || len==0)
	{
		return -1;
	}
	socket_service_send_data_t *data_item = (socket_service_send_data_t *)LUAT_MEM_MALLOC(sizeof(socket_service_send_data_t));
	LUAT_DEBUG_ASSERT(data_item != NULL,"malloc data_item fail");;

	data_item->data = LUAT_MEM_MALLOC(len);
	LUAT_DEBUG_ASSERT(data_item->data != NULL, "malloc data_item.data fail");
	memcpy(data_item->data, data, len);
	data_item->len = len;
	data_item->callback_func = callback_func;
	data_item->callback_param = callback_param;

	int ret = luat_rtos_message_send(g_s_tcp_send_task_handle, 0, data_item);

	if(ret != 0)
	{
		LUAT_MEM_FREE(data_item->data);
		data_item->data = NULL;
		LUAT_MEM_FREE(data_item);
		data_item = NULL;
	}

	return ret;
}

static void tcp_send_task_proc(void *arg)
{
	uint32_t message_id;
	socket_service_send_data_t *data_item;
	int ret = -1;
	uint32_t sent_len = 0;

	while(1)
	{
		if(luat_rtos_message_recv(g_s_tcp_send_task_handle, &message_id, (void **)&data_item, LUAT_WAIT_FOREVER) == 0)
		{
			if(g_s_is_connected)
			{
				sent_len = 0;
				// LUAT_DEBUG_PRINT("total len %d, sent len %d", data_item->len, sent_len);
				while (sent_len < data_item->len)
				{
					ret = send(g_s_socket_id, data_item->data+sent_len, data_item->len-sent_len, 0);
					if (ret >= 0)
					{
						LUAT_DEBUG_PRINT("send %d bytes", ret);
						sent_len += ret;
						if (sent_len >= data_item->len)
						{
							if(data_item->callback_func)
							{
								data_item->callback_func(0, data_item->callback_param);
							}
						}						
					}
					else
					{
						if (errno == EWOULDBLOCK)
						{
							LUAT_DEBUG_PRINT("block, wait send buffer ok");
							luat_rtos_task_sleep(1000);
						}
						else
						{
							close_socket();
							if(data_item->callback_func)
							{
								data_item->callback_func(ret, data_item->callback_param);
							}
							luat_rtos_task_sleep(5000);
							socket_service_init();

							break;
						}
					}					
				}				
			}
			else
			{
				if(data_item->callback_func)
				{
					data_item->callback_func(1, data_item->callback_param);
				}
			}

			LUAT_MEM_FREE(data_item->data);
			data_item->data = NULL;
			LUAT_MEM_FREE(data_item);
			data_item = NULL;
		}
	}
}


static void tcp_connect_task_proc(void *arg)
{
	ip_addr_t remote_ip;
    struct sockaddr_in name;
    socklen_t sockaddr_t_size = sizeof(name);
    int ret, h_errnop;
    struct hostent dns_result;
    struct hostent *p_result;

	LUAT_DEBUG_PRINT("enter");

	while(1)
    {
		//Check whether the network is ready. If it is not ready, wait for 1 second and then retry in a loop.
		while(!network_service_is_ready())
		{
			LUAT_DEBUG_PRINT("wait network_service_is_ready");
			luat_rtos_task_sleep(1000);
		}

		//Execute DNS. If it fails, wait for 1 second, return to check the network logic, and try again.
		char buf[128] = {0};
		ret = lwip_gethostbyname_r(REMOTE_SERVER_ADDRESS, &dns_result, buf, sizeof(buf), &p_result, &h_errnop);
		if(ret == 0)
		{
			remote_ip = *((ip_addr_t *)dns_result.h_addr_list[0]);
		}
		else
		{
			luat_rtos_task_sleep(1000);
			LUAT_DEBUG_PRINT("dns fail");
			continue;
		}

		//Create the socket. If the creation fails, wait 3 seconds and try again in a loop.
		g_s_socket_id = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if(g_s_socket_id < 0)
		{
			LUAT_DEBUG_PRINT("create socket fail");
			luat_rtos_task_sleep(3000);
			continue;
		}
		
		//Connect to the server, if it fails, close the socket, wait 5 seconds, return to check the network logic, and try again
		name.sin_family = AF_INET;
		name.sin_addr.s_addr = remote_ip.u_addr.ip4.addr;
		name.sin_port = htons(g_s_remote_server_port);
        ret = connect(g_s_socket_id, (const struct sockaddr *)&name, sockaddr_t_size);
		if(ret < 0)
		{
			LUAT_DEBUG_PRINT("connect fail, ret %d",ret);
			close_socket();
			luat_rtos_task_sleep(5000);
			continue;
		}
		LUAT_DEBUG_PRINT("connect ok");
		g_s_is_connected = 1;
		
		fcntl(g_s_socket_id, F_SETFL, O_NONBLOCK);		

		if (NULL == g_s_connect_ok_semaphore)
		{
			luat_rtos_semaphore_create(&g_s_connect_ok_semaphore, 1);
		}
		luat_rtos_semaphore_release(g_s_connect_ok_semaphore);
		   

		if(g_s_tcp_send_task_handle == NULL)
		{
			luat_rtos_task_create(&g_s_tcp_send_task_handle, 2048, 20, "tcp_send", tcp_send_task_proc, NULL, TCP_SEND_TASK_MAILBOX_EVENT_MAX_SIZE);
		}
		if(g_s_tcp_recv_task_handle == NULL)
		{
			luat_rtos_task_create(&g_s_tcp_recv_task_handle, 2048, 20, "tcp_recv", tcp_recv_task_proc, NULL, 0);
		}			

		break;				
    }

	//Print out the minimum remaining stack space size since the task started
	//Then we can calculate the maximum used size, which can generally be multiplied by about 1.5 as the final allocated value, which must be a multiple of 4
	// LUAT_DEBUG_PRINT("before luat_rtos_task_delete, %d", luat_rtos_task_get_high_water_mark());

	LUAT_DEBUG_PRINT("exit");
	g_s_is_tcp_connect_task_exist = 0;
	luat_rtos_task_delete(g_s_tcp_connect_task_handle);	
}


void socket_service_init(void)
{
	if(!g_s_is_tcp_connect_task_exist)
	{
		g_s_is_tcp_connect_task_exist = (luat_rtos_task_create(&g_s_tcp_connect_task_handle, 2560, 30, "tcp_connect", tcp_connect_task_proc, NULL, 0)==0);
	}	
}


