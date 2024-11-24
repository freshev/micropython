/**
 * @file aiot_sysdep_api.h
 * @brief SDK Core system dependency header file, all system dependencies in Core are listed in this file
 * @date 2021-07-14
 *
 * @copyright Copyright (C) 2015-2018 Alibaba Group Holding Limited
 **/

#ifndef _POXIS_PORT_H_
#define _POXIS_PORT_H_

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>
#include "aiot_sysdep_api.h"

/**
 * @brief A method structure used to describe to the SDK how to use the resources of its running hardware platform*/
typedef struct {
    /**
     * @brief apply for memory*/
    void *(*malloc)(uint32_t size);
    /**
     * @brief release memory*/
    void (*free)(void *ptr);
    /**
     * @brief Get the current timestamp, SDK is used for difference calculation*/
    uint64_t (*time)(void);
    /**
     * @brief Sleep for the specified number of milliseconds*/
    void (*sleep)(uint64_t time_ms);
    /**
     * @brief random number generation method*/
    void (*rand)(uint8_t *output, uint32_t output_len);
    /**
     * @brief Create a mutex lock*/
    void    *(*mutex_init)(void);
    /**
     * @brief apply for mutex lock*/
    void (*mutex_lock)(void *mutex);
    /**
     * @brief Release the mutex lock*/
    void (*mutex_unlock)(void *mutex);
    /**
     * @brief destroy mutex lock*/
    void (*mutex_deinit)(void **mutex);
} aiot_os_al_t;


/**
 * @brief A method structure used to describe to the SDK how to use the resources of its running hardware platform*/

typedef struct {
    /**
     * @brief Establish a network session as the underlying bearer of protocols such as MQTT/HTTP*/
    int32_t (*establish)(int type, char *host, int port, int timeout_ms, int *fd_out);
    /**
     * @brief Read from the specified network session*/
    int32_t (*recv)(int fd, uint8_t *buffer, uint32_t len, uint32_t timeout_ms, core_sysdep_addr_t *addr);
    /**
     * @brief Sent on the specified network session*/
    int32_t (*send)(int fd, uint8_t *buffer, uint32_t len, uint32_t timeout_ms,
                    core_sysdep_addr_t *addr);
    /**
     * @brief Destroy 1 network session*/
    int32_t (*close)(int fd);
} aiot_net_al_t;

void aiot_install_os_api(aiot_os_al_t *os);
void aiot_install_net_api(aiot_net_al_t *net);

void *core_sysdep_malloc(uint32_t size, char *name);
void core_sysdep_free(void *ptr);
uint64_t core_sysdep_time(void);
void core_sysdep_sleep(uint64_t time_ms);
void *core_sysdep_network_init(void);
int32_t core_sysdep_network_setopt(void *handle, core_sysdep_network_option_t option, void *data);
int32_t core_sysdep_network_establish(void *handle);
int32_t core_sysdep_network_recv(void *handle, uint8_t *buffer, uint32_t len, uint32_t timeout_ms,core_sysdep_addr_t *addr);
int32_t core_sysdep_network_send(void *handle, uint8_t *buffer, uint32_t len, uint32_t timeout_ms,core_sysdep_addr_t *addr);
int32_t core_sysdep_network_deinit(void **handle);
void core_sysdep_rand(uint8_t *output, uint32_t output_len);
void *core_sysdep_mutex_init(void);
void core_sysdep_mutex_lock(void *mutex);
void core_sysdep_mutex_unlock(void *mutex);
void core_sysdep_mutex_deinit(void **mutex);

#if defined(__cplusplus)
}
#endif
#endif
