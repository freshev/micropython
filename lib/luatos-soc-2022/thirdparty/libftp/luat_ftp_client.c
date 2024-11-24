/*@Modules  ftp
@summary ftp 客户端
@version 1.0
@date    2022.09.05
@demo    ftp
@tag LUAT_USE_FTP*/

#include "luat_network_adapter.h"
#include "luat_rtos.h"
#include "luat_fs.h"
#include "luat_mem.h"

#include "luat_ftp.h"
#include "common_api.h"
#include "luat_debug.h"

#define FTP_DEBUG 0
#if FTP_DEBUG == 0
#undef DBG
#define DBG(...)
#endif


luat_ftp_ctrl_t g_s_ftp;

uint32_t luat_ftp_release(void) {
	if (!g_s_ftp.network) return 0;
	if (g_s_ftp.network->cmd_netc){
		network_force_close_socket(g_s_ftp.network->cmd_netc);
		network_release_ctrl(g_s_ftp.network->cmd_netc);
		g_s_ftp.network->cmd_netc = NULL;
	}
	if (g_s_ftp.network->data_netc){
		network_force_close_socket(g_s_ftp.network->data_netc);
		network_release_ctrl(g_s_ftp.network->data_netc);
		g_s_ftp.network->data_netc = NULL;
	}
	luat_heap_free(g_s_ftp.network);
	g_s_ftp.network = NULL;
	return 0;
}

static uint32_t luat_ftp_data_send(luat_ftp_ctrl_t *ftp_ctrl, uint8_t* send_data, uint32_t send_len) {
	if (send_len == 0)
		return 0;
	uint32_t tx_len = 0;
	DBG("luat_ftp_data_send data:%d",send_len);
	network_tx(g_s_ftp.network->data_netc, send_data, send_len, 0, NULL, 0, &tx_len, 0);
	return tx_len;
}

static uint32_t luat_ftp_cmd_send(luat_ftp_ctrl_t *ftp_ctrl, uint8_t* send_data, uint32_t send_len,uint32_t timeout_ms) {
	if (send_len == 0)
		return 0;
	uint32_t tx_len = 0;
	DBG("luat_ftp_cmd_send data:%.*s",send_len,send_data);
	network_tx(g_s_ftp.network->cmd_netc, send_data, send_len, 0, NULL, 0, &tx_len, timeout_ms);
	return tx_len;
}

static int luat_ftp_cmd_recv(luat_ftp_ctrl_t *ftp_ctrl,uint8_t *recv_data,uint32_t *recv_len,uint32_t timeout_ms){
	int result = 0,total_len = 0;
	uint8_t is_break = 0,is_timeout = 0;
	while (1){
		result = network_wait_rx(g_s_ftp.network->cmd_netc, timeout_ms, &is_break, &is_timeout);
		DBG("network_wait_rx result:%d is_break:%d is_timeout:%d",result,is_break,is_timeout);
		if (result) return -1;
		if (is_timeout) return 1;
		else if (is_break) return 2;
		do{
			result = network_rx(g_s_ftp.network->cmd_netc, &recv_data[total_len], FTP_CMD_RECV_MAX - total_len, 0, NULL, NULL, recv_len);
			if(*recv_len > 0)
			{
				total_len += *recv_len;
			}
			// DBG("recv len %d %d", *recv_len, total_len);
			// DBG("recv data %s", recv_data);
		} while (!result && *recv_len > 0);
		if(0 == memcmp(recv_data + total_len - 2, "\r\n", 2)){
			recv_data[total_len] = 0;
			DBG("all recv len %d %d", *recv_len, total_len);
			DBG("all recv data %s", recv_data);
			break;
		}
	}
	
	
	#if 0
	int result = network_rx(g_s_ftp.network->cmd_netc, NULL, 0, 0, NULL, NULL, &total_len);
	if (0 == result){
		if (total_len>0){
next:
			result = network_rx(g_s_ftp.network->cmd_netc, recv_data, total_len, 0, NULL, NULL, recv_len);
			DBG("result:%d recv_len:%d",result,*recv_len);
			DBG("recv_data %.*s",total_len, recv_data);
			if (result)
				goto next;
			if (*recv_len == 0||result!=0) {
				return -1;
			}
			return 0;
		}
	}else{
		DBG("ftp network_rx fail");
		return -1;
	}
	#endif
	return 0;
}

static int32_t luat_ftp_data_callback(void *data, void *param){
	OS_EVENT *event = (OS_EVENT *)data;
	uint8_t *rx_buffer;
	int ret = 0;
	uint32_t rx_len = 0;
	if (!g_s_ftp.network)
	{
		return 0;
	}
	DBG("event->ID %d LINK %d ON_LINE %d EVENT %d TX_OK %d CLOSED %d",event->ID,EV_NW_RESULT_LINK & 0x0fffffff,EV_NW_RESULT_CONNECT & 0x0fffffff,EV_NW_RESULT_EVENT & 0x0fffffff,EV_NW_RESULT_TX & 0x0fffffff,EV_NW_RESULT_CLOSE & 0x0fffffff);
	DBG("luat_ftp_data_callback %d %d",event->ID - EV_NW_RESULT_BASE, event->Param1);
	if (event->Param1){
		if (EV_NW_RESULT_CONNECT == event->ID)
		{
			luat_rtos_event_send(g_s_ftp.task_handle, FTP_EVENT_DATA_CONNECT, 0xffffffff, 0, 0, LUAT_WAIT_FOREVER);
		}
		else
		{
			luat_rtos_event_send(g_s_ftp.task_handle, FTP_EVENT_DATA_CLOSED, 0, 0, 0, LUAT_WAIT_FOREVER);
		}
		return -1;
	}
	switch (event->ID)
	{
	case EV_NW_RESULT_TX:
		luat_rtos_event_send(g_s_ftp.task_handle, FTP_EVENT_DATA_TX_DONE, 0, 0, 0, LUAT_WAIT_FOREVER);
		break;
	case EV_NW_RESULT_EVENT:
		rx_buffer = NULL;
		uint8_t tmpbuff[4];
		do
		{
			// Read the length first
			ret = network_rx(g_s_ftp.network->data_netc, NULL, 0, 0, NULL, NULL, &rx_len);
			if (rx_len <= 0) {
				// No data? Then read it once, and then exit
				network_rx(g_s_ftp.network->data_netc, tmpbuff, 4, 0, NULL, NULL, &rx_len);
				break;
			}
			if (rx_len > 2048)
				rx_len = 2048;
			rx_buffer = luat_heap_malloc(rx_len);
			// If rx_buffer == NULL, the memory is full
			if (rx_buffer == NULL) {
				DBG("out of memory when malloc ftp buff");
				network_close(g_s_ftp.network->data_netc, 0);
				return -1;
			}
			ret = network_rx(g_s_ftp.network->data_netc, rx_buffer, rx_len, 0, NULL, NULL, &rx_len);
			DBG("luat_ftp_data_callback network_rx ret:%d rx_len:%d",ret,rx_len);
			if (!ret && rx_len > 0)
			{
				if (g_s_ftp.fd || g_s_ftp.is_fota == 1)
				{
					luat_rtos_event_send(g_s_ftp.task_handle, FTP_EVENT_DATA_WRITE_FILE, rx_buffer, rx_len, 0, LUAT_WAIT_FOREVER);
					rx_buffer = NULL;
					continue;
				}
				else
				{
					OS_BufferWrite(&g_s_ftp.result_buffer, rx_buffer, rx_len);
				}
			}
			luat_heap_free(rx_buffer);
			rx_buffer = NULL;
		} while (!ret && rx_len);
		if (rx_buffer)
			luat_heap_free(rx_buffer);
		rx_buffer = NULL;
		break;
	case EV_NW_RESULT_CLOSE:
		luat_rtos_event_send(g_s_ftp.task_handle, FTP_EVENT_DATA_CLOSED, 0, 0, 0, LUAT_WAIT_FOREVER);
		return 0;
		break;
	case EV_NW_RESULT_CONNECT:
		luat_rtos_event_send(g_s_ftp.task_handle, FTP_EVENT_DATA_CONNECT, 0, 0, 0, LUAT_WAIT_FOREVER);
		break;
	case EV_NW_RESULT_LINK:
		return 0;
	}

	ret = network_wait_event(g_s_ftp.network->data_netc, NULL, 0, NULL);
	if (ret < 0){
		network_close(g_s_ftp.network->data_netc, 0);
		return -1;
	}
    return 0;
}

static int32_t ftp_task_cb(void *pdata, void *param)
{
	OS_EVENT *event = pdata;
	if (event->ID >= FTP_EVENT_LOGIN && event->ID <= FTP_EVENT_PUSH)
	{
		DBG("last cmd not finish, ignore %d,%u,%u,%x", event->ID - USER_EVENT_ID_START, event->Param1, event->Param2, param);
		return -1;
	}
	switch(event->ID)
	{
	case FTP_EVENT_DATA_WRITE_FILE:
		if (g_s_ftp.is_fota)
		{
			luat_fota_write(g_s_ftp.context, (void*)event->Param1, event->Param2);
			luat_heap_free((void*)event->Param1);
		}else if (g_s_ftp.fd){
			luat_fs_fwrite((void*)event->Param1, event->Param2, 1, g_s_ftp.fd);
			luat_heap_free((void*)event->Param1);
		}
		break;
	case FTP_EVENT_DATA_TX_DONE:
		g_s_ftp.network->upload_done_size = (size_t)g_s_ftp.network->data_netc->ack_size;
		if (g_s_ftp.network->upload_done_size >= g_s_ftp.network->local_file_size)
		{
			DBG("ftp data upload done!");
			network_close(g_s_ftp.network->data_netc, 0);
		}
		break;
	case FTP_EVENT_DATA_CONNECT:
		if (g_s_ftp.network->data_netc_connecting)
		{
			g_s_ftp.network->data_netc_connecting = 0;
			g_s_ftp.network->data_netc_online = !event->Param1;
		}
		break;
	case FTP_EVENT_DATA_CLOSED:
		DBG("ftp data channel close");
		g_s_ftp.network->data_netc_online = 0;
		if (g_s_ftp.network->data_netc)
		{
			network_force_close_socket(g_s_ftp.network->data_netc);
			network_release_ctrl(g_s_ftp.network->data_netc);
			g_s_ftp.network->data_netc = NULL;
		}
		break;
	case FTP_EVENT_CLOSE:
		g_s_ftp.is_run = 0;
		break;
	default:
//		DBG("ignore %x,%x,%x", event->ID, param, EV_NW_RESULT_EVENT);
		break;
	}
	return 0;
}

static int luat_ftp_pasv_connect(luat_ftp_ctrl_t *ftp_ctrl,uint32_t timeout_ms){
	char h1[4]={0},h2[4]={0},h3[4]={0},h4[4]={0},p1[4]={0},p2[4]={0},data_addr[64]={0};
	uint8_t port1,port2;
	uint16_t data_port;	
	luat_ftp_cmd_send(&g_s_ftp, (uint8_t*)"PASV\r\n", strlen("PASV\r\n"),FTP_SOCKET_TIMEOUT);
	int ret = luat_ftp_cmd_recv(&g_s_ftp,g_s_ftp.network->cmd_recv_data,&g_s_ftp.network->cmd_recv_len,FTP_SOCKET_TIMEOUT);
	if (ret){
		return -1;
	}else{
		DBG("luat_ftp_pasv_connect cmd_recv_data",g_s_ftp.network->cmd_recv_data);
		if (memcmp(g_s_ftp.network->cmd_recv_data, FTP_ENTER_PASSIVE, 3)){
			DBG("ftp pasv_connect wrong");
			return -1;
		}
	}
    char *temp = memchr(g_s_ftp.network->cmd_recv_data, '(', strlen((const char *)(g_s_ftp.network->cmd_recv_data)));
    char *temp1 = memchr(temp+1, ',', strlen(temp)-1);
    memcpy(h1, temp+1, temp1-temp-1);
    char *temp2 = memchr(temp1+1, ',', strlen(temp1)-1);
    memcpy(h2, temp1+1, temp2-temp1-1);
    char *temp3 = memchr(temp2+1, ',', strlen(temp2)-1);
    memcpy(h3, temp2+1, temp3-temp2-1);
    char *temp4 = memchr(temp3+1, ',', strlen(temp3)-1);
    memcpy(h4, temp3+1, temp4-temp3-1);
    char *temp5 = memchr(temp4+1, ',', strlen(temp4)-1);
    memcpy(p1, temp4+1, temp5-temp4-1);
    char *temp6 = memchr(temp5+1, ')', strlen(temp5)-1);
    memcpy(p2, temp5+1, temp6-temp5-1);
	snprintf_(data_addr, 64, "%s.%s.%s.%s",h1,h2,h3,h4);
	port1 = (uint8_t)atoi(p1);
	port2 = (uint8_t)atoi(p2);
	data_port = port1 * 256 + port2;
	DBG("data_addr:%s data_port:%d",data_addr,data_port);
	if (memcmp(data_addr,"172.",4)==0||memcmp(data_addr,"192.",4)==0||memcmp(data_addr,"10.",3)==0||memcmp(data_addr,"127.0.0.1",9)==0||memcmp(data_addr,"169.254.0.0",11)==0||memcmp(data_addr,"169.254.0.16",12)==0){
		memset(data_addr,0,64);
		DBG("g_s_ftp.network->addr:%s",g_s_ftp.network->addr);
		memcpy(data_addr, g_s_ftp.network->addr, strlen(g_s_ftp.network->addr)+1);
	}
	DBG("data_addr:%s data_port:%d",data_addr,data_port);
	if (g_s_ftp.network->data_netc)
	{
		DBG("data_netc already create");
		return -1;
	}
	g_s_ftp.network->data_netc = network_alloc_ctrl(g_s_ftp.network->adapter_index);
	if (!g_s_ftp.network->data_netc){
		DBG("data_netc create fail");
		return -1;
	}
	network_init_ctrl(g_s_ftp.network->data_netc,NULL, luat_ftp_data_callback, g_s_ftp.network);
	network_set_base_mode(g_s_ftp.network->data_netc, 1, 10000, 0, 0, 0, 0);
	network_set_local_port(g_s_ftp.network->data_netc, 0);
	network_deinit_tls(g_s_ftp.network->data_netc);
	if(network_connect(g_s_ftp.network->data_netc, data_addr, strlen(data_addr), NULL, data_port, 0)<0){
		DBG("ftp data network connect fail");
		network_force_close_socket(g_s_ftp.network->data_netc);
		network_release_ctrl(g_s_ftp.network->data_netc);
		g_s_ftp.network->data_netc = NULL;
		return -1;
	}
	uint8_t is_timeout;
	OS_EVENT event;
	g_s_ftp.network->data_netc_connecting = 1;
	g_s_ftp.network->data_netc_online = 0;
	while(g_s_ftp.network->data_netc_connecting)
	{
		if (network_wait_event(g_s_ftp.network->cmd_netc, &event, timeout_ms, &is_timeout))
		{
			return -1;
		}
		if (is_timeout)
		{
			return -1;
		}
		if (event.ID)
		{
			ftp_task_cb(&event, NULL);
		}
		else
		{
        	if (g_s_ftp.network->cmd_netc->new_rx_flag)
        	{
        		network_rx(g_s_ftp.network->cmd_netc, g_s_ftp.network->cmd_recv_data, 1024, 0, NULL, NULL, &g_s_ftp.network->cmd_recv_len);
        		DBG("ftp cmd rx %.*s", g_s_ftp.network->cmd_recv_len, g_s_ftp.network->cmd_recv_data);
        	}
		}
	}
	if (g_s_ftp.network->data_netc_online)
	{
		DBG("ftp pasv_connect ok");
		return 0;
	}
	return -1;
}



static int ftp_login(void)
{
	int ret;
	if(network_connect(g_s_ftp.network->cmd_netc, g_s_ftp.network->addr, strlen(g_s_ftp.network->addr), NULL, g_s_ftp.network->port, FTP_SOCKET_TIMEOUT)){
		DBG("ftp network_connect fail");
		return -1;
	}
	ret = luat_ftp_cmd_recv(&g_s_ftp,g_s_ftp.network->cmd_recv_data,&g_s_ftp.network->cmd_recv_len,FTP_SOCKET_TIMEOUT);
	if (ret){
		return -1;
	}else{
		if (memcmp(g_s_ftp.network->cmd_recv_data, FTP_SERVICE_NEW_OK, 3)){
			DBG("ftp connect error");
			return -1;
		}
	}
	DBG("ftp connect ok");
	memset(g_s_ftp.network->cmd_send_data,0,FTP_CMD_SEND_MAX);
	memset(g_s_ftp.network->cmd_recv_data,0,FTP_CMD_RECV_MAX);
	snprintf_((char *)(g_s_ftp.network->cmd_send_data), FTP_CMD_SEND_MAX, "USER %s\r\n",g_s_ftp.network->username);
	luat_ftp_cmd_send(&g_s_ftp, g_s_ftp.network->cmd_send_data, strlen((const char *)(g_s_ftp.network->cmd_send_data)),FTP_SOCKET_TIMEOUT);
	ret = luat_ftp_cmd_recv(&g_s_ftp,g_s_ftp.network->cmd_recv_data,&g_s_ftp.network->cmd_recv_len,FTP_SOCKET_TIMEOUT);
	DBG("ftp recv data  %s", g_s_ftp.network->cmd_recv_data);
	if (ret){
		DBG("ftp username wrong");
		return -1;
	}else{
		if (memcmp(g_s_ftp.network->cmd_recv_data, FTP_USERNAME_OK, 3)){
			DBG("ftp username wrong");
			return -1;
		}
	}
	DBG("ftp username ok");
	memset(g_s_ftp.network->cmd_send_data,0,FTP_CMD_SEND_MAX);
	memset(g_s_ftp.network->cmd_recv_data,0,FTP_CMD_RECV_MAX);
	snprintf_((char *)(g_s_ftp.network->cmd_send_data), FTP_CMD_SEND_MAX, "PASS %s\r\n",g_s_ftp.network->password);
	luat_ftp_cmd_send(&g_s_ftp, g_s_ftp.network->cmd_send_data, strlen((const char *)(g_s_ftp.network->cmd_send_data)),FTP_SOCKET_TIMEOUT);
	ret = luat_ftp_cmd_recv(&g_s_ftp,g_s_ftp.network->cmd_recv_data,&g_s_ftp.network->cmd_recv_len,FTP_SOCKET_TIMEOUT);

	if (ret){
		DBG("ftp login wrong");
		return -1;
	}else{
		if (memcmp(g_s_ftp.network->cmd_recv_data, FTP_LOGIN_OK, 3)){
			DBG("ftp login wrong");
			return -1;
		}
	}
	DBG("ftp login ok");
	return 0;
}

static void l_ftp_cb(FTP_SUCCESS_STATE_e state){
	if (g_s_ftp.network->ftp_cb){
		luat_ftp_cb_t ftp_cb = g_s_ftp.network->ftp_cb;
		ftp_cb(&g_s_ftp,state);
	}
	OS_DeInitBuffer(&g_s_ftp.result_buffer);
}

static void ftp_task(void *param){
	FTP_SUCCESS_STATE_e ftp_state = FTP_SUCCESS_NO_DATA;
	int ret;
	int count = 0;
	luat_rtos_task_handle task_handle = g_s_ftp.task_handle;
	OS_EVENT task_event;
	uint8_t is_timeout = 0;
	g_s_ftp.is_run = 1;
	luat_rtos_event_recv(g_s_ftp.task_handle, FTP_EVENT_LOGIN, &task_event, NULL, LUAT_WAIT_FOREVER);
	if (ftp_login()){
		DBG("ftp login fail");
		ftp_state = FTP_ERROR;
		l_ftp_cb(ftp_state);
		luat_ftp_release();
		g_s_ftp.task_handle = NULL;
		luat_rtos_task_delete(task_handle);
		return;
	}else{
		l_ftp_cb(ftp_state);
	}
    while (g_s_ftp.is_run) {
    	is_timeout = 0;
    	ret = network_wait_event(g_s_ftp.network->cmd_netc, &task_event, 3600000, &is_timeout);
    	if (ret < 0){    		
			DBG("ftp network error");
    		goto wait_event_and_out;
    	}else if (is_timeout || !task_event.ID){
        	if (g_s_ftp.network->cmd_netc->new_rx_flag){
        		network_rx(g_s_ftp.network->cmd_netc, g_s_ftp.network->cmd_recv_data, 1024, 0, NULL, NULL, &ret);
        		DBG("ftp rx %dbyte", ret);
        	}
    		continue;
    	}
    	ftp_state = FTP_SUCCESS_NO_DATA;
		switch (task_event.ID)
		{
		case FTP_EVENT_LOGIN:
			break;
		case FTP_EVENT_PULL:
			if (g_s_ftp.network->data_netc)
			{
				network_force_close_socket(g_s_ftp.network->data_netc);
				network_release_ctrl(g_s_ftp.network->data_netc);
				g_s_ftp.network->data_netc = NULL;
			}
			if(luat_ftp_pasv_connect(&g_s_ftp,FTP_SOCKET_TIMEOUT)){
				DBG("ftp pasv_connect fail");
				goto operation_failed;
			}
			snprintf_((char *)(g_s_ftp.network->cmd_send_data), FTP_CMD_SEND_MAX, "RETR %s\r\n",g_s_ftp.network->remote_name);
			luat_ftp_cmd_send(&g_s_ftp, g_s_ftp.network->cmd_send_data, strlen((const char *)g_s_ftp.network->cmd_send_data),FTP_SOCKET_TIMEOUT);
			ret = luat_ftp_cmd_recv(&g_s_ftp,g_s_ftp.network->cmd_recv_data,&g_s_ftp.network->cmd_recv_len,FTP_SOCKET_TIMEOUT);
			if (ret){
				goto operation_failed;
			}else{

				if (memcmp(g_s_ftp.network->cmd_recv_data, FTP_FILE_STATUS_OK, 3)){
					DBG("ftp RETR wrong");
					goto operation_failed;
				}
			}
			if (!g_s_ftp.network->data_netc_online)
			{
				g_s_ftp.network->cmd_recv_data[g_s_ftp.network->cmd_recv_len] = 0;
				DBG("ftp RETR maybe done!");
				if (strstr((const char *)(g_s_ftp.network->cmd_recv_data), "226 "))
				{
					DBG("ftp RETR ok!");
					
					if(g_s_ftp.is_fota)
					{
						if(luat_fota_done(g_s_ftp.context)){
							ftp_state = FTP_ERROR;
						}
						g_s_ftp.is_fota = 0;
					} else if (g_s_ftp.fd){
						luat_fs_fclose(g_s_ftp.fd);
						g_s_ftp.fd = NULL;
					}
					l_ftp_cb(ftp_state);
					break;
				}
			}
			ret = luat_ftp_cmd_recv(&g_s_ftp,g_s_ftp.network->cmd_recv_data,&g_s_ftp.network->cmd_recv_len,FTP_SOCKET_TIMEOUT);
			if (ret){
				goto operation_failed;
			}else{
				if (memcmp(g_s_ftp.network->cmd_recv_data, FTP_CLOSE_CONNECT, 3)){
					DBG("ftp RETR wrong");
					goto operation_failed;
				}
			}
			while (count<3 && g_s_ftp.network->data_netc_online!=0){
				luat_ftp_cmd_recv(&g_s_ftp,g_s_ftp.network->cmd_recv_data,&g_s_ftp.network->cmd_recv_len,FTP_SOCKET_TIMEOUT/3);
				count++;
			}
			
			if (g_s_ftp.is_fota){
				if(luat_fota_done(g_s_ftp.context)){
					ftp_state = FTP_ERROR;
				}
				g_s_ftp.is_fota = 0;
			} else if (g_s_ftp.fd){
				luat_fs_fclose(g_s_ftp.fd);
				g_s_ftp.fd = NULL;
			}
			l_ftp_cb(ftp_state);
			break;
		case FTP_EVENT_PUSH:
			if(luat_ftp_pasv_connect(&g_s_ftp,FTP_SOCKET_TIMEOUT)){
				DBG("ftp pasv_connect fail");
				goto operation_failed;
			}

			memset(g_s_ftp.network->cmd_send_data,0,FTP_CMD_SEND_MAX);
			snprintf_((char *)(g_s_ftp.network->cmd_send_data), FTP_CMD_SEND_MAX, "STOR %s\r\n",g_s_ftp.network->remote_name);
			luat_ftp_cmd_send(&g_s_ftp, g_s_ftp.network->cmd_send_data, strlen((const char *)g_s_ftp.network->cmd_send_data),FTP_SOCKET_TIMEOUT);
			ret = luat_ftp_cmd_recv(&g_s_ftp,g_s_ftp.network->cmd_recv_data,&g_s_ftp.network->cmd_recv_len,FTP_SOCKET_TIMEOUT);
			if (ret){
				goto operation_failed;
			}else{
				if (memcmp(g_s_ftp.network->cmd_recv_data, FTP_FILE_STATUS_OK, 3)){
					DBG("ftp STOR wrong");
					goto operation_failed;
				}
			}

			uint8_t* buff = luat_heap_malloc(PUSH_BUFF_SIZE);
			int offset = 0;
			g_s_ftp.network->upload_done_size = 0;
			while (1) {
				memset(buff, 0, PUSH_BUFF_SIZE);
				int len = luat_fs_fread(buff, sizeof(uint8_t), PUSH_BUFF_SIZE, g_s_ftp.fd);
				if (len < 1)
					break;
				luat_ftp_data_send(&g_s_ftp, buff, len);
				offset += len;
			}
			luat_heap_free(buff);
			DBG("offset:%d file_size:%d",offset,g_s_ftp.network->local_file_size);
			ret = luat_ftp_cmd_recv(&g_s_ftp,g_s_ftp.network->cmd_recv_data,&g_s_ftp.network->cmd_recv_len,FTP_SOCKET_TIMEOUT);
			if (g_s_ftp.network->upload_done_size != g_s_ftp.network->local_file_size)
			{
				DBG("upload not finish !!! %d,%d", g_s_ftp.network->upload_done_size, g_s_ftp.network->local_file_size);
			}
			if (ret){
				goto operation_failed;
			}else{
				if (memcmp(g_s_ftp.network->cmd_recv_data, FTP_CLOSE_CONNECT, 3)){
					DBG("ftp STOR wrong");
				}
			}
			while (count<3 && g_s_ftp.network->data_netc_online!=0){
				luat_ftp_cmd_recv(&g_s_ftp,g_s_ftp.network->cmd_recv_data,&g_s_ftp.network->cmd_recv_len,FTP_SOCKET_TIMEOUT/3);
				count++;
			}
			if (g_s_ftp.fd){
				luat_fs_fclose(g_s_ftp.fd);
				g_s_ftp.fd = NULL;
			}
			l_ftp_cb(ftp_state);
			break;
		case FTP_EVENT_CLOSE:
			g_s_ftp.is_run = 0;
			break;
		case FTP_EVENT_COMMAND:
			OS_DeInitBuffer(&g_s_ftp.result_buffer);
			if(!memcmp(g_s_ftp.network->cmd_send_data, "LIST", 4))
			{
				if(luat_ftp_pasv_connect(&g_s_ftp,FTP_SOCKET_TIMEOUT)){
					DBG("ftp pasv_connect fail");
					goto operation_failed;
				}
				luat_ftp_cmd_send(&g_s_ftp, g_s_ftp.network->cmd_send_data, strlen((const char *)(g_s_ftp.network->cmd_send_data)),FTP_SOCKET_TIMEOUT);
				ret = luat_ftp_cmd_recv(&g_s_ftp,g_s_ftp.network->cmd_recv_data,&g_s_ftp.network->cmd_recv_len,FTP_SOCKET_TIMEOUT / 3);
				if (ret){
					goto operation_failed;
				}else{
					if (memcmp(g_s_ftp.network->cmd_recv_data, FTP_FILE_STATUS_OK, 3)){
						DBG("ftp LIST wrong");
						goto operation_failed;
					}
				}
				if (!g_s_ftp.network->data_netc_online)
				{
					g_s_ftp.network->cmd_recv_data[g_s_ftp.network->cmd_recv_len] = 0;
					DBG("ftp LIST maybe done!");
					if (strstr((const char *)(g_s_ftp.network->cmd_recv_data), FTP_CLOSE_CONNECT))
					{
						DBG("ftp LIST ok!");
						ftp_state = FTP_SUCCESS_DATA;
						l_ftp_cb(ftp_state);
						break;
					}
				}
				ret = luat_ftp_cmd_recv(&g_s_ftp,g_s_ftp.network->cmd_recv_data,&g_s_ftp.network->cmd_recv_len,FTP_SOCKET_TIMEOUT);
				if (ret){
					goto operation_failed;
				}else{
					if (memcmp(g_s_ftp.network->cmd_recv_data, FTP_CLOSE_CONNECT, 3)){
						DBG("ftp LIST wrong");
						goto operation_failed;
					}
				}
				ftp_state = FTP_SUCCESS_DATA;
				l_ftp_cb(ftp_state);
				break;
			}
			luat_ftp_cmd_send(&g_s_ftp, g_s_ftp.network->cmd_send_data, strlen((const char *)(g_s_ftp.network->cmd_send_data)),FTP_SOCKET_TIMEOUT);
			ret = luat_ftp_cmd_recv(&g_s_ftp,g_s_ftp.network->cmd_recv_data,&g_s_ftp.network->cmd_recv_len,FTP_SOCKET_TIMEOUT);
			if (ret){
				goto operation_failed;
			}else{
				if (memcmp(g_s_ftp.network->cmd_send_data, "NOOP", 4)==0){
					if (memcmp(g_s_ftp.network->cmd_recv_data, FTP_COMMAND_OK, 3)){
						DBG("ftp COMMAND wrong");
					}
				}else if(memcmp(g_s_ftp.network->cmd_send_data, "TYPE", 4)==0){
					if (memcmp(g_s_ftp.network->cmd_recv_data, FTP_COMMAND_OK, 3)){
						DBG("ftp COMMAND wrong");
					}
				}else if(memcmp(g_s_ftp.network->cmd_send_data, "SYST", 4)==0){
					if (memcmp(g_s_ftp.network->cmd_recv_data, FTP_SYSTEM_TYPE, 3)){
						DBG("ftp COMMAND wrong");
					}
				}else if(memcmp(g_s_ftp.network->cmd_send_data, "PWD", 3)==0){
					if (memcmp(g_s_ftp.network->cmd_recv_data, FTP_PATHNAME_OK, 3)){
						DBG("ftp COMMAND wrong");
					}
				}else if(memcmp(g_s_ftp.network->cmd_send_data, "MKD", 3)==0){
					if (memcmp(g_s_ftp.network->cmd_recv_data, FTP_PATHNAME_OK, 3)){
						DBG("ftp COMMAND wrong");
					}
				}else if(memcmp(g_s_ftp.network->cmd_send_data, "CWD", 3)==0){
					if (memcmp(g_s_ftp.network->cmd_recv_data, FTP_FILE_REQUESTED_OK, 3)){
						DBG("ftp COMMAND wrong");
					}
				}else if(memcmp(g_s_ftp.network->cmd_send_data, "CDUP", 4)==0){
					if (memcmp(g_s_ftp.network->cmd_recv_data, FTP_FILE_REQUESTED_OK, 3)){
						DBG("ftp COMMAND wrong");
					}
				}else if(memcmp(g_s_ftp.network->cmd_send_data, "RMD", 3)==0){
					if (memcmp(g_s_ftp.network->cmd_recv_data, FTP_FILE_REQUESTED_OK, 3)){
						DBG("ftp COMMAND wrong");
					}
				}else if(memcmp(g_s_ftp.network->cmd_send_data, "DELE", 4)==0){
					if (memcmp(g_s_ftp.network->cmd_recv_data, FTP_FILE_REQUESTED_OK, 3)){
						DBG("ftp COMMAND wrong");
					}
				}else if (memcmp(g_s_ftp.network->cmd_recv_data, FTP_DATA_CON_FAIL, 3)==0){
					DBG("ftp need pasv_connect");


				}
			}
			OS_BufferWrite(&g_s_ftp.result_buffer, g_s_ftp.network->cmd_recv_data, g_s_ftp.network->cmd_recv_len);
			ftp_state = FTP_SUCCESS_DATA;
			l_ftp_cb(ftp_state);
			break;
		default:
			break;
		}
		continue;
operation_failed:
		if(g_s_ftp.is_fota)
		{
			luat_fota_done(g_s_ftp.context);
			g_s_ftp.is_fota = 0;
		}
		ftp_state = FTP_ERROR;
		l_ftp_cb(ftp_state);
	}
	ftp_state = FTP_SUCCESS_NO_DATA;
	luat_ftp_cmd_send(&g_s_ftp, (uint8_t*)"QUIT\r\n", strlen("QUIT\r\n"),FTP_SOCKET_TIMEOUT);
	ret = luat_ftp_cmd_recv(&g_s_ftp,g_s_ftp.network->cmd_recv_data,&g_s_ftp.network->cmd_recv_len,FTP_SOCKET_TIMEOUT);
	if (ret){
		ftp_state = FTP_ERROR;
	}else{
		if (memcmp(g_s_ftp.network->cmd_recv_data, FTP_CLOSE_CONTROL, 3)){
			DBG("ftp QUIT wrong");
			ftp_state = FTP_ERROR;
		}
	}
	OS_BufferWrite(&g_s_ftp.result_buffer, g_s_ftp.network->cmd_recv_data, g_s_ftp.network->cmd_recv_len);
	if (ftp_state == FTP_SUCCESS_NO_DATA) ftp_state = FTP_SUCCESS_DATA;
	l_ftp_cb(ftp_state);
	luat_ftp_release();
	g_s_ftp.task_handle = NULL;
	luat_rtos_task_delete(task_handle);
	return;
wait_event_and_out:
	while(1)
	{
		luat_rtos_event_recv(g_s_ftp.task_handle, 0, &task_event, NULL, LUAT_WAIT_FOREVER);
		if (task_event.ID >= FTP_EVENT_LOGIN && task_event.ID <= FTP_EVENT_CLOSE)
		{
			luat_ftp_release();
			ftp_state = FTP_ERROR;
			l_ftp_cb(ftp_state);
			g_s_ftp.task_handle = NULL;
			luat_rtos_task_delete(task_handle);
			return;
		}
	}
}
int luat_ftp_login(uint8_t adapter,const char * ip_addr,uint16_t port,const char * username,const char * password,luat_ftp_tls_t* luat_ftp_tls,luat_ftp_cb_t ftp_cb){
	if (g_s_ftp.network){
		DBG("ftp already login, please close first");
		return FTP_ERROR_STATE;
	}
	g_s_ftp.network = (luat_ftp_network_t *)luat_heap_malloc(sizeof(luat_ftp_network_t));
	if (!g_s_ftp.network){
		DBG("out of memory when malloc g_s_ftp.network");
		return FTP_ERROR_NO_MEM;
	}
	memset(g_s_ftp.network, 0, sizeof(luat_ftp_network_t));
	if (ftp_cb){
		g_s_ftp.network->ftp_cb = ftp_cb;
	}
	g_s_ftp.network->adapter_index = adapter;
	if (g_s_ftp.network->adapter_index >= NW_ADAPTER_QTY){
		DBG("bad network adapter index %d", g_s_ftp.network->adapter_index);
		return FTP_ERROR_STATE;
	}
	g_s_ftp.network->cmd_netc = network_alloc_ctrl(g_s_ftp.network->adapter_index);
	if (!g_s_ftp.network->cmd_netc){
		DBG("cmd_netc create fail");
		return FTP_ERROR_NO_MEM;
	}
	g_s_ftp.network->port = port;
	if (strlen(ip_addr) > 0 && strlen(ip_addr) < 64)
		memcpy(g_s_ftp.network->addr, ip_addr, strlen(ip_addr) + 1);
	if (strlen(username) > 0 && strlen(username) < 64)
		memcpy(g_s_ftp.network->username, username, strlen(username) + 1);
	if (strlen(password) > 0 && strlen(password) < 64)
		memcpy(g_s_ftp.network->password, password, strlen(password) + 1);
	if (luat_ftp_tls == NULL){
		network_deinit_tls(g_s_ftp.network->cmd_netc);
	}else{
		if (network_init_tls(g_s_ftp.network->cmd_netc, (luat_ftp_tls->server_cert || luat_ftp_tls->client_cert)?2:0)){
			return FTP_ERROR_CLOSE;
		}
		if (luat_ftp_tls->server_cert){
			network_set_server_cert(g_s_ftp.network->cmd_netc, (const unsigned char *)luat_ftp_tls->server_cert, strlen(luat_ftp_tls->server_cert)+1);
		}
		if (luat_ftp_tls->client_cert){
			network_set_client_cert(g_s_ftp.network->cmd_netc, (const unsigned char *)luat_ftp_tls->client_cert, strlen(luat_ftp_tls->client_cert)+1,
					(const unsigned char *)luat_ftp_tls->client_key, strlen(luat_ftp_tls->client_key)+1,
					(const unsigned char *)luat_ftp_tls->client_password, strlen(luat_ftp_tls->client_password)+1);
		}
	}
	g_s_ftp.network->ip_addr.type = 0Xff;
	// network_set_ip_invaild(&g_s_ftp.network->ip_addr);
	int result = luat_rtos_task_create(&g_s_ftp.task_handle, 2*1024, 10, "ftp", ftp_task, NULL, 16);
	if (result) {
		DBG("Failed to create ftp task!! %d", result);
		return result;
	}
	network_init_ctrl(g_s_ftp.network->cmd_netc,g_s_ftp.task_handle, ftp_task_cb, NULL);
	network_set_base_mode(g_s_ftp.network->cmd_netc, 1, 30000, 0, 0, 0, 0);
	network_set_local_port(g_s_ftp.network->cmd_netc, 0);
	luat_rtos_event_send(g_s_ftp.task_handle, FTP_EVENT_LOGIN, 0, 0, 0, LUAT_WAIT_FOREVER);
	return 0;
}

int luat_ftp_command(const char * command){
	if (!g_s_ftp.network){
		DBG("please login first");
		return -1;
	}
	if (memcmp(command, "NOOP", 4)==0){
		DBG("command: NOOP");
	}else if(memcmp(command, "SYST", 4)==0){
		DBG("command: SYST");
	}else if(memcmp(command, "MKD", 3)==0){
		DBG("command: MKD");
	}else if(memcmp(command, "CWD", 3)==0){
		DBG("command: CWD");
	}else if(memcmp(command, "CDUP", 4)==0){
		DBG("command: CDUP");
	}else if(memcmp(command, "RMD", 3)==0){
		DBG("command: RMD");
	}else if(memcmp(command, "PWD", 3)==0){
		DBG("command: RMD");
	}else if(memcmp(command, "DELE", 4)==0){
		DBG("command: DELE");
	}else if(memcmp(command, "TYPE", 4)==0){
		DBG("command: TYPE");
	}else if(memcmp(command, "LIST", 4)==0){
		DBG("command: LIST");
	}else{
		DBG("not support cmd:%s",command);
		return -1;
	}
	memset(g_s_ftp.network->cmd_send_data,0,FTP_CMD_SEND_MAX);
	snprintf_((char *)(g_s_ftp.network->cmd_send_data), FTP_CMD_SEND_MAX, "%s\r\n",command);
	luat_rtos_event_send(g_s_ftp.task_handle, FTP_EVENT_COMMAND, 0, 0, 0, 0);
	return 0;
}

int luat_ftp_pull(const char * local_name, const char * remote_name, uint8_t is_fota){
	if (!g_s_ftp.network){
		DBG("please login first");
		return -1;
	}

	if (is_fota == 1)
	{
		g_s_ftp.is_fota = 1;
		g_s_ftp.context = luat_fota_init();
	}
	else
	{
		luat_fs_remove(local_name);
		if (g_s_ftp.fd)
		{
			luat_fs_fclose(g_s_ftp.fd);
			g_s_ftp.fd = NULL;
		}
		g_s_ftp.fd = luat_fs_fopen(local_name, "wb+");
		if (g_s_ftp.fd == NULL) {
			DBG("open download file fail %s", local_name);
			return -1;
		}
	}
	// g_s_ftp.network->local_file_size = luat_fs_fsize(local_name);
	memcpy(g_s_ftp.network->remote_name, remote_name, strlen(remote_name) + 1);
	luat_rtos_event_send(g_s_ftp.task_handle, FTP_EVENT_PULL, 0, 0, 0, 0);
	return 0;
}

int luat_ftp_close(void){
	if (!g_s_ftp.network){
		DBG("please login first");
		return -1;
	}
	luat_rtos_event_send(g_s_ftp.task_handle, FTP_EVENT_CLOSE, 0, 0, 0, LUAT_WAIT_FOREVER);
	return 0;
}

int luat_ftp_push(const char * local_name,const char * remote_name){
	if (!g_s_ftp.network){
		DBG("please login first");
		return -1;
	}
	if (g_s_ftp.fd)
	{
		luat_fs_fclose(g_s_ftp.fd);
		g_s_ftp.fd = NULL;
	}
	g_s_ftp.fd = luat_fs_fopen(local_name, "rb");
	if (g_s_ftp.fd == NULL) {
		DBG("open download file fail %s", local_name);
		return -1;
	}
	g_s_ftp.network->local_file_size = luat_fs_fsize(local_name);
	memcpy(g_s_ftp.network->remote_name, remote_name, strlen(remote_name) + 1);
	luat_rtos_event_send(g_s_ftp.task_handle, FTP_EVENT_PUSH, 0, 0, 0, LUAT_WAIT_FOREVER);
	return 0;
}
