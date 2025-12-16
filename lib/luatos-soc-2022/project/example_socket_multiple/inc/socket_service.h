#ifndef SOCKET_SERVICE_H
#define SOCKET_SERVICE_H

typedef enum SOCKET_EVENT
{
	SOCKET_EVENT_CONNECT,
    SOCKET_EVENT_SEND,
    SOCKET_EVENT_RECEIVE,
}SOCKET_EVENT_E;

typedef union socket_event_param
{	
	int connect_result; //>=0 succeeds; other values   fail
	struct
    {
        int result;//>0 succeeds; other values   fail
        int user_param;
    } send_cnf_t;
    struct
    {
        int len;
        char *data;
    } recv_ind_t;
    
} socket_event_param_t;

typedef void (*socket_service_event_callback_t)(int app_id, SOCKET_EVENT_E event, socket_event_param_t param);

//Create a socket and connect to the server
//Return the execution synchronization result: 0 means success, the rest means failure;
//The asynchronous connection result is notified through the callback function callback
// app_id: user connection id, starting from 0, the maximum value is USER_SOCKET_MAX_COUNT-1
int socket_connect(int app_id, int protocol, char *address, int port, socket_service_event_callback_t callback);

//The result of synchronous insertion into the queue is judged by the return value: 0 is successful, others fail.
//After the insertion is successful, the result is sent asynchronously and notified through callback
int socket_send(int app_id, uint32_t len, const char *data, int user_param);


#endif
