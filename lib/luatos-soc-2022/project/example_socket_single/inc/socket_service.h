#ifndef SOCKET_SERVICE_H
#define SOCKET_SERVICE_H

//result: 0 successful, 1 socket not connected; the remaining error values   are the error cause values   returned by the lwip send interface
typedef void (*socket_service_send_data_callback_t)(int result, uint32_t callback_param);


void socket_service_init(void);
void socket_service_set_remote_port(int port);
//The result of synchronous insertion into the queue is judged by the return value: 0 is successful, others fail.
//After the insertion is successful, the asynchronous sending result is judged by the result in callback_func(int result, void *callback_param): 0 is successful, others fail.
int socket_service_send_data(const char *data, uint32_t len, socket_service_send_data_callback_t callback_func, uint32_t callback_param);


#endif
