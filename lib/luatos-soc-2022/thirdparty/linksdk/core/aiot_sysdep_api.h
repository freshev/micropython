/**
 * @file aiot_sysdep_api.h
 * @brief SDK Core system dependency header file, all system dependencies in Core are listed in this file
 * @date 2019-12-27
 *
 * @copyright Copyright (C) 2015-2018 Alibaba Group Holding Limited
 **/

#ifndef _AIOT_SYSDEP_API_H_
#define _AIOT_SYSDEP_API_H_

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>

typedef enum {
    AIOT_SYSDEP_NETWORK_CRED_NONE,
    AIOT_SYSDEP_NETWORK_CRED_SVRCERT_CA,
    AIOT_SYSDEP_NETWORK_CRED_SVRCERT_PSK,
    AIOT_SYSDEP_NETWORK_CRED_MAX
} aiot_sysdep_network_cred_option_t;

typedef struct {
    aiot_sysdep_network_cred_option_t option;  /*security policy*/
    uint32_t      max_tls_fragment;
    uint8_t       sni_enabled;
    const char   *x509_server_cert;     /*Must be located in the static storage area, no copying is done inside the SDK*/
    uint32_t      x509_server_cert_len;
    const char   *x509_client_cert;     /*Must be located in the static storage area, no copying is done inside the SDK*/
    uint32_t      x509_client_cert_len;
    const char   *x509_client_privkey;  /*Must be located in the static storage area, no copying is done inside the SDK*/
    uint32_t      x509_client_privkey_len;
    char         *tls_extend_info;
} aiot_sysdep_network_cred_t;

typedef enum {
    CORE_SYSDEP_SOCKET_TCP_CLIENT,
    CORE_SYSDEP_SOCKET_TCP_SERVER,
    CORE_SYSDEP_SOCKET_UDP_CLIENT,
    CORE_SYSDEP_SOCKET_UDP_SERVER
} core_sysdep_socket_type_t;

typedef struct {
    char *psk_id;
    char *psk;
} core_sysdep_psk_t;

typedef enum {
    CORE_SYSDEP_NETWORK_SOCKET_TYPE,             /*The socket type that needs to be established. Data type: (core_sysdep_socket_type_t *)*/
    CORE_SYSDEP_NETWORK_HOST,                    /*The domain name address or IP address used to establish a network connection. The memory is shared with the upper Modules. Data type: (char *)*/
    CORE_SYSDEP_NETWORK_BACKUP_IP,               /*When Jianlian DNS resolution fails, use this backup IP to try again.*/
    CORE_SYSDEP_NETWORK_PORT,                    /*Port number used to establish network connection Data type: (uint16_t *)*/
    CORE_SYSDEP_NETWORK_CONNECT_TIMEOUT_MS,      /*Timeout for establishing network connection Data type: (uint32_t *)*/
    CORE_SYSDEP_NETWORK_CRED,                    /*Used to set network layer security parameters Data type: (aiot_sysdep_network_cred_t *)*/
    CORE_SYSDEP_NETWORK_PSK,                     /*Used to match psk-id and psk data type in PSK mode: (core_sysdep_psk_t *)*/
    CORE_SYSDEP_NETWORK_MAX
} core_sysdep_network_option_t;

typedef struct {
    char addr[16]; /*IPv4 address dotted decimal string, maximum length 15 bytes.*/
    uint16_t port; /*port number*/
} core_sysdep_addr_t;

/*This is not a user-oriented compilation configuration switch. In most cases, users do not need to care.*/

/**
 * @brief A method structure used to describe to the SDK how to use the resources of its running hardware platform*/
typedef struct {
    /**
     * @brief apply for memory*/
    void    *(*core_sysdep_malloc)(uint32_t size, char *name);
    /**
     * @brief release memory*/
    void (*core_sysdep_free)(void *ptr);
    /**
     * @brief Get the current timestamp, SDK is used for difference calculation*/
    uint64_t (*core_sysdep_time)(void);
    /**
     * @brief Sleep for the specified number of milliseconds*/
    void (*core_sysdep_sleep)(uint64_t time_ms);
    /**
     * @brief Create a network session (L3 layer)*/
    void    *(*core_sysdep_network_init)(void);

    /**
     * @brief Configure the connection parameters of a network session*/
    int32_t (*core_sysdep_network_setopt)(void *handle, core_sysdep_network_option_t option, void *data);
    /**
     * @brief Establish a network session as the underlying bearer of protocols such as MQTT/HTTP*/
    int32_t (*core_sysdep_network_establish)(void *handle);
    /**
     * @brief Read from the specified network session*/
    int32_t (*core_sysdep_network_recv)(void *handle, uint8_t *buffer, uint32_t len, uint32_t timeout_ms,
                                        core_sysdep_addr_t *addr);
    /**
     * @brief Sent on the specified network session*/
    int32_t (*core_sysdep_network_send)(void *handle, uint8_t *buffer, uint32_t len, uint32_t timeout_ms,
                                        core_sysdep_addr_t *addr);
    /**
     * @brief Destroy 1 network session*/
    int32_t (*core_sysdep_network_deinit)(void **handle);
    /**
     * @brief random number generation method*/
    void (*core_sysdep_rand)(uint8_t *output, uint32_t output_len);
    /**
     * @brief Create a mutex lock*/
    void    *(*core_sysdep_mutex_init)(void);
    /**
     * @brief apply for mutex lock*/
    void (*core_sysdep_mutex_lock)(void *mutex);
    /**
     * @brief Release the mutex lock*/
    void (*core_sysdep_mutex_unlock)(void *mutex);
    /**
     * @brief destroy mutex lock*/
    void (*core_sysdep_mutex_deinit)(void **mutex);
} aiot_sysdep_portfile_t;

void aiot_sysdep_set_portfile(aiot_sysdep_portfile_t *portfile);
aiot_sysdep_portfile_t *aiot_sysdep_get_portfile(void);

#if defined(__cplusplus)
}
#endif

#endif

