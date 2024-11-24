#include "luat_base.h"
#include "luat_network_adapter.h"
#include "luat_mem.h"
#include "luat_msgbus.h"
#include "luat_crypto.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/select.h>

#define LUAT_LOG_TAG "network"
#include "luat_log.h"

#include "luat_network_posix.h"

CBFuncEx_t posix_network_cb;
void * posix_network_param;
uint8_t posix_network_ready;

static luat_rtos_mutex_t* master_lock;

static void posix_send_event(int id, int p1, int p2, int p3) {
    luat_network_cb_param_t params = {0};
    params.tag = 0;
    params.param = posix_network_param;
    //Trigger the callback
    // if (ready) {
    OS_EVENT event = {
        .ID = id,
        .Param1 = p1,
        .Param2 = p2,
        .Param3 = p3
    };
    LLOGD("posix event %d %d %d %d", id, p1, p2, p3);
    posix_network_cb(&event, &params);
}

void posix_network_client_thread_entry(posix_socket_t *ps) {

    luat_network_cb_param_t params = {0};
    params.tag = 0;
    params.param = posix_network_param;
    //Trigger the callback
    // if (ready) {
    OS_EVENT event = {0};

    struct sockaddr_in sockaddr = {0};
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons(ps->remote_port);
    sockaddr.sin_addr.s_addr = ps->remote_ip.ipv4;
    luat_rtos_task_sleep(50);

    LLOGD("ready to connect %d", ps->socket_id);
    int ret = connect(ps->socket_id, (struct sockaddr*)&sockaddr, sizeof(sockaddr));

    LLOGD("connect ret %d", ret);
    if (ret) {
        // failed
        LLOGD("connect FAIL ret %d", ret);
        posix_send_event(EV_NW_SOCKET_ERROR, ps->socket_id, 0, 0);
        luat_heap_free(ps);
        return;
    }
    //Send system message to notify successful connection
    posix_send_event(EV_NW_SOCKET_CONNECT_OK, ps->socket_id, 0, 0);
    LLOGD("wait data now");

    fd_set readfds;
    fd_set writefds;
    fd_set errorfds;
    int maxsock;
    struct timeval tv;
    maxsock = ps->socket_id;
    // timeout setting
    tv.tv_sec = 0;
    tv.tv_usec = 3000; //Just 3ms for now
    while (1) {
        // initialize file descriptor set
        FD_ZERO(&readfds);
        // FD_ZERO(&writefds);
        FD_ZERO(&errorfds);
        FD_SET(ps->socket_id, &readfds);
        // FD_SET(ps->socket_id, &writefds);
        FD_SET(ps->socket_id, &errorfds);

        if (master_lock)
            if (luat_rtos_mutex_lock(master_lock, 100))
                continue;
        ret = select(maxsock + 1, &readfds, NULL, &errorfds, &tv);
        if (master_lock)
            luat_rtos_mutex_unlock(master_lock);

        if (ret < 0) {
            LLOGE("select ret %d", ret);
            break;
        } else if (ret == 0) {
            //printf("select timeout\n");
            continue;
        }

        if (FD_ISSET(maxsock, &readfds)) {
            //Send message, it is readable
        }
        // if (FD_ISSET(maxsock, &writefds)) {
        // // Send message, sending completed??
        // }
        if (FD_ISSET(maxsock, &errorfds)) {
            //Send message, something went wrong
            break;
        }
    }

    luat_heap_free(ps);
    LLOGI("socket thread exit");
}

void posix_network_set_ready(uint8_t ready) {
    LLOGD("CALL posix_network_set_ready");
    posix_network_ready = ready;
    luat_network_cb_param_t params = {0};
    params.tag = 0;
    params.param = posix_network_param;
    //Trigger the callback
    // if (ready) {
        OS_EVENT event = {
            .ID = EV_NW_STATE,
            .Param1 = 0,
            .Param2 = ready,
            .Param3 = 0
        };
        posix_network_cb(&event, &params);
    // }
}

//Check whether the network is ready, return non-0 ready, user_data is the user_data during registration, passed to the underlying API
uint8_t (posix_check_ready)(void *user_data) {
    LLOGD("CALL posix_check_ready %d", posix_network_ready);
    return posix_network_ready;
};

//Create a socket and set it to non-blocking mode. User_data is passed to the corresponding adapter, and tag is used as the legal basis of the socket for check_socket_vaild comparison.
//Return socketid on success, <0 on failure
int (posix_create_socket)(uint8_t is_tcp, uint64_t *tag, void *param, uint8_t is_ipv6, void *user_data) {
    // TODO support IPV6
    int s = socket(AF_INET, is_tcp ? SOCK_STREAM : SOCK_DGRAM, is_tcp ? IPPROTO_TCP : IPPROTO_UDP);
    LLOGD("CALL posix_create_socket %d %d", s, is_tcp);
    return s;
}

//Bind a port as a client and connect to the server corresponding to remote_ip and remote_port
//Return 0 on success, <0 on failure
int (posix_socket_connect)(int socket_id, uint64_t tag, uint16_t local_port, luat_ip_addr_t *remote_ip, uint16_t remote_port, void *user_data) {
    LLOGD("CALL posix_socket_connect %d", socket_id);
    posix_socket_t *ps = luat_heap_malloc(sizeof(posix_socket_t));
    if (ps == NULL) {
        LLOGE("out of memory when malloc posix_socket_t");
        return -1;
    }
    ps->socket_id = socket_id;
    ps->tag = tag;
    ps->local_port = local_port;
    memcpy(&ps->remote_ip, remote_ip, sizeof(luat_ip_addr_t));
    ps->remote_port = remote_port;
    ps->user_data = user_data;

    int ret = network_posix_client_thread_start(ps);
    LLOGD("socket thread start %d", ret);

    if (ret) {
        luat_heap_free(ps);
    }
    return ret;
}
//Bind a port as a server and start listening
//Return 0 on success, <0 on failure
int (posix_socket_listen)(int socket_id, uint64_t tag, uint16_t local_port, void *user_data) {
    // Not yet supported
    return -1;
}
//Accept a client as a server
//Return 0 on success, <0 on failure
int (posix_socket_accept)(int socket_id, uint64_t tag, luat_ip_addr_t *remote_ip, uint16_t *remote_port, void *user_data) {
    // Not yet supported
    return -1;
}

//To proactively disconnect a tcp connection, the entire tcp process needs to be completed. The user needs to receive the close ok callback to confirm the complete disconnection.
//Return 0 on success, <0 on failure
int (posix_socket_disconnect)(int socket_id, uint64_t tag, void *user_data) {
    return close(socket_id);
}

//Release control of the socket, except for tag exceptions, it must take effect immediately
//Return 0 on success, <0 on failure
int (posix_socket_close)(int socket_id, uint64_t tag, void *user_data) {
    return close(socket_id);
}

//Forcibly release control of the socket, which must take effect immediately
//Return 0 on success, <0 on failure
int (posix_socket_force_close)(int socket_id, void *user_data) {
    return close(socket_id);
}

//In tcp, remote_ip and remote_port are not needed. If buf is NULL, the amount of data in the current buffer is returned. When the return value is less than len, it means that the reading has been completed.
//When udp is used, only 1 block of data is returned, and it needs to be read multiple times until there is no data.
//Return the actual read value on success, <0 on failure
int (posix_socket_receive)(int socket_id, uint64_t tag, uint8_t *buf, uint32_t len, int flags, luat_ip_addr_t *remote_ip, uint16_t *remote_port, void *user_data) {
    
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 1000; //Just 1ms for now
    setsockopt(socket_id, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    if (master_lock)
        if (luat_rtos_mutex_lock(master_lock, 100))
            return -1;
    int ret = recv(socket_id, buf, len, flags);
    if (master_lock)
        luat_rtos_mutex_unlock(master_lock);
    return ret;
}

//In tcp, remote_ip and remote_port are not required
//Return >0 len successfully, buffer full = 0, failure < 0, if an empty packet with len=0 is sent, it will also return 0, pay attention to judgment
int (posix_socket_send)(int socket_id, uint64_t tag, const uint8_t *buf, uint32_t len, int flags, luat_ip_addr_t *remote_ip, uint16_t remote_port, void *user_data) {
    
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 1000; //Just 1ms for now
    setsockopt(socket_id, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    if (master_lock)
        if (luat_rtos_mutex_lock(master_lock, 100))
            return -1;
    int ret = send(socket_id, buf, len, flags);
    if (master_lock)
        luat_rtos_mutex_unlock(master_lock);
    return ret;
}

//Check the validity of the socket, return 0 if successful, <0 if failed
int (posix_socket_check)(int socket_id, uint64_t tag, void *user_data) {
    // TODO by select errorfds?
    LLOGD("CALL posix_socket_check %d %lld", socket_id, tag);
    return 0;
}

//Keep valid sockets and close invalid sockets
void (posix_socket_clean)(int *vaild_socket_list, uint32_t num, void *user_data) {
    
}

int (posix_getsockopt)(int socket_id, uint64_t tag, int level, int optname, void *optval, uint32_t *optlen, void *user_data) {
    return getsockopt(socket_id, level, optname, optval, optlen);
}

int (posix_setsockopt)(int socket_id, uint64_t tag, int level, int optname, const void *optval, uint32_t optlen, void *user_data) {
    return setsockopt(socket_id, level, optname, optval, optlen);
}

//Non-posix socket, use this to set parameters according to actual hardware
int (posix_user_cmd)(int socket_id, uint64_t tag, uint32_t cmd, uint32_t value, void *user_data) {
    return 0; // None of these things
}


int (posix_dns)(const char *domain_name, uint32_t len, void *param,  void *user_data) {
    LLOGD("CALL posix_dns %.*s", len, domain_name);
    return -1; // DNS is not supported yet
}

int (posix_set_dns_server)(uint8_t server_index, luat_ip_addr_t *ip, void *user_data) {
    return 0; // Setting DNS is not supported yet
}

#ifdef LUAT_USE_LWIP
int (posix_set_mac)(uint8_t *mac, void *user_data);
int (posix_set_static_ip)(luat_ip_addr_t *ip, luat_ip_addr_t *submask, luat_ip_addr_t *gateway, luat_ip_addr_t *ipv6, void *user_data);
#endif
int (posix_get_local_ip_info)(luat_ip_addr_t *ip, luat_ip_addr_t *submask, luat_ip_addr_t *gateway, void *user_data) {
    ip->ipv4 = 0;
    submask->ipv4 = 0;
    gateway->ipv4 = 0;
    return 0;
}

//All network messages are passed through cb_fun callback
//The first parameter in the cb_fun callback is OS_EVENT, which contains the necessary information about the socket. The second parameter is luat_network_cb_param_t, where the param is the param passed in here (that is, the adapter serial number)
//OS_EVENT ID is EV_NW_XXX, param1 is the socket id, param2 is the respective parameter, param3 is the socket_param passed in by create_soceket (that is, network_ctrl *)
//The dns result is special, the ID is EV_NW_SOCKET_DNS_RESULT, param1 is the amount of IP data obtained, 0 means failure, param2 is the ip group, dynamically allocated, param3 is the param passed in by dns (that is, network_ctrl *)
void (posix_socket_set_callback)(CBFuncEx_t cb_fun, void *param, void *user_data) {
    LLOGD("call posix_socket_set_callback %p %p", cb_fun, param);
    if (master_lock == NULL)
        luat_rtos_mutex_create(master_lock);
    posix_network_cb = cb_fun;
    posix_network_param = param;
}


network_adapter_info network_posix = {
    .check_ready = posix_check_ready,
    .create_soceket = posix_create_socket,
    .socket_connect  = posix_socket_connect,
    .socket_accept = posix_socket_accept,
    .socket_disconnect  = posix_socket_disconnect,
    .socket_close = posix_socket_close,
    .socket_force_close = posix_socket_force_close,
    .socket_receive = posix_socket_receive,
    .socket_send = posix_socket_send,
    .socket_clean = posix_socket_clean,
    .getsockopt = posix_getsockopt,
    .setsockopt = posix_setsockopt,
    .user_cmd  = posix_user_cmd,
    .dns = posix_dns,
    .set_dns_server = posix_set_dns_server,
    .get_local_ip_info = posix_get_local_ip_info,
    .socket_set_callback = posix_socket_set_callback,
    .name = "posix",
    .max_socket_num = 4,
    .no_accept = 1, // Receiving is not supported temporarily
    .is_posix = 1,
};
