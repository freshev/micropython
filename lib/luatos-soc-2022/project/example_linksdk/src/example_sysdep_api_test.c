/***************
	demo_sysdep_api_test
****************/


/** This routine is used by users to verify whether the transplantation is successful after transplantation.
 * Through a series of test cases, verify whether the transplanted related interfaces work properly;
 * + After normal completion, TEST SUCCESS will be output, otherwise TEST ERROR + ERRORCODE will be output.
 * + If it is stuck for a long time, it will also be a test failure.
 *
 * This testing tool will use multi-threading/multi-tasking capabilities, and needs to be adapted to task creation when used under RTOS.
 * The parts that require user attention or modification have been marked in comments with TODO
 **/
#include "common_api.h"
#include "aiot_sysdep_api.h"
#include "aiot_state_api.h"
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "luat_rtos.h"
#include "luat_debug.h"

/*Define entry function type*/
typedef void *(*TASK_FUNC)(void* argv);

/*TODO: Maximum heap space, in bytes*/
#define HEAP_MAX    ( 20 * 1024 )
/*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< TODO END <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/

/**
 * System interface test results*/
typedef enum {
    TEST_SUCCESS,
    TEST_ERR_RANDOM,
    TEST_ERR_MALLOC,
    TEST_ERR_HEAP,
    TEST_ERR_SLEEP,
    TEST_ERR_MUTEX,
    TEST_ERR_NETWORK,
    TEST_ERR_GENERIC,
} sysdep_test_result_t;

static const char *result_string[] = {
    "TEST_SUCCESS", 
    "TEST_ERR_RANDOM",
    "TEST_ERR_MALLOC",
    "TEST_ERR_HEAP", 
    "TEST_ERR_SLEEP",
    "TEST_ERR_MUTEX",
    "TEST_ERR_NETWORK",
    "TEST_ERR_GENERIC",
};

typedef struct {
    aiot_sysdep_portfile_t *sysdep;
    void* user_data;
    char* name;
} task_handler_input_t;

/**
 * sysdep test entrance prototype definition*/
typedef sysdep_test_result_t (*sysdep_test_func)(aiot_sysdep_portfile_t* sysdep);

typedef struct
{
    uint32_t total_size;//Total heap size
} heap_cfg_t;

typedef struct
{
    uint16_t count;
    /*Maximum number of repetitions*/
    uint16_t repeat_cnt_max;
} random_cfg_t;

typedef struct
{
    uint64_t sleep_ms;
    uint8_t  sleep_end;
} time_cfg_t;

typedef struct
{
    void* mutex;
    int32_t  value;
    uint32_t timeout_ms;
} mutex_cfg_t;

typedef struct
{
    char*       host;
    uint16_t    port;
    uint32_t    connect_timeout_ms;
    uint32_t    recv_timeout_ms;
    int32_t     recv_buf_len;
    uint32_t    send_timeout_ms;
    uint32_t    cycle_test_times;
} network_cfg_t;

static luat_rtos_task_handle sleep_task_handle1;
static luat_rtos_task_handle sleep_task_handle2;
static luat_rtos_task_handle mutex_task_handle1;
static luat_rtos_task_handle mutex_task_handle2;
static luat_rtos_task_handle linksdk_task_handle;

/**
 * Randomly generate a list of random numbers, the number of recurrences is not higher than the specified value*/
static sysdep_test_result_t random_repeat_test(aiot_sysdep_portfile_t* sysdep, random_cfg_t* rd)
{
    typedef  uint8_t random_value_t;
    random_value_t *random_values = NULL;
    int count = 0;
    random_value_t temp = 0;
    sysdep_test_result_t ret = TEST_SUCCESS;
    /*Difference*/
    int diff = 0;
    /*Gradient flag*/
    int flag = 0;

    random_values = sysdep->core_sysdep_malloc(sizeof(random_value_t) * rd->count, NULL);
    if(random_values == NULL){
        LUAT_DEBUG_PRINT("[linksdk]\r\nmalloc fail");
        ret = TEST_ERR_MALLOC;
        goto end;
    }
    memset(random_values, 0, sizeof(random_value_t) * rd->count);

    /*Generate random numbers and count the number of repetitions to calculate the difference*/
    for(count = 0; count < rd->count; count++) {
        sysdep->core_sysdep_rand((uint8_t*)&temp, sizeof(random_value_t));
        temp %= rd->count;
        
        /*Check whether it is a regular gradient*/
        if(count == 1 ){
            diff = random_values[count] - random_values[count - 1];
        }else if (count >= 2){
            if(diff == random_values[count] - random_values[count - 1]){
                flag++;
            }
        }
        
        if(random_values[temp] <= rd->repeat_cnt_max) {
            random_values[temp]++;
        }
        else {
            LUAT_DEBUG_PRINT("[linksdk]randowm value [%d] times [%d] over max counter [%d]", temp, random_values[temp], rd->repeat_cnt_max);
            ret = TEST_ERR_RANDOM;
            goto end;
        }
    }
    /*If the rule changes gradually, an error will be returned.*/
    if(rd->count >= 3 && flag >= rd->count - 2){
        LUAT_DEBUG_PRINT("[linksdk]random value error");
        ret = TEST_ERR_RANDOM;
        goto end;
    }
    ret = TEST_SUCCESS;
end:
    if(random_values != NULL){
        sysdep->core_sysdep_free(random_values);
    }
    return ret;
}

/**
 * Random number test entrance*/
sysdep_test_result_t random_test(aiot_sysdep_portfile_t* sysdep)
{
    random_cfg_t rd;
    /*The random number interface requires that the random number generated is random by bytes.*/
    rd.count = 256;
    /*The number of random number repetitions cannot exceed this value*/
    rd.repeat_cnt_max = 10;
    return random_repeat_test(sysdep, &rd);
}

static sysdep_test_result_t heap_malloc_max_test(aiot_sysdep_portfile_t* sysdep, heap_cfg_t* hp)
{
    /**
     * Maximum space application and release*/
    uint32_t user_malloc_max = hp->total_size - 100;
    void * ptr = sysdep->core_sysdep_malloc(user_malloc_max, "");
    if(ptr == NULL) {
        LUAT_DEBUG_PRINT("[linksdk]malloc error");
        return TEST_ERR_MALLOC;
    }
    /*spatial literacy test*/
    uint8_t read_value;
    for(int i = 0; i < user_malloc_max; i++) {
        ((uint8_t*)ptr)[i] = (uint8_t)i;
        read_value = ((uint8_t*)ptr)[i];
        if(read_value != (uint8_t)i) {
            sysdep->core_sysdep_free(ptr);
            LUAT_DEBUG_PRINT("[linksdk]heap read and write fail");
            return TEST_ERR_MALLOC;
        }
    }
    sysdep->core_sysdep_free(ptr);
    return TEST_SUCCESS;
}

/**
 * Heap recycling test*/
static sysdep_test_result_t heap_recycle_test(aiot_sysdep_portfile_t* sysdep, heap_cfg_t* hp)
{
    /*The size of the heap space to be applied for*/
    const uint32_t size_list[]= {1,2,4,8,16,32,64,128,256,512,1024,10*1024};
    /*storage heap pointer*/
    void* malloc_list[sizeof(size_list) / sizeof(uint32_t)] = {NULL};
    int malloc_list_size = sizeof(size_list) / sizeof(uint32_t);
    /*Apply for space*/
    int malloc_cnt = 0;
    int i = 0;
    for(i = 0; i < malloc_list_size; i++) {
        malloc_list[i] = sysdep->core_sysdep_malloc(size_list[i], "");
        if(malloc_list[i] == NULL) {
            break;
        }
    }
    malloc_cnt = i;
    /**
     * Release of non-adjacent space*/
    for(i = 0; i < malloc_cnt; i+=2) {
        sysdep->core_sysdep_free(malloc_list[i]);
    }
    for(i = 1; i < malloc_cnt; i+=2) {
        sysdep->core_sysdep_free(malloc_list[i]);
    }
    /**
     *Finally apply for the maximum space*/
    return heap_malloc_max_test(sysdep, hp);
}

/**
 * Heap test entrance*/

sysdep_test_result_t heap_test(aiot_sysdep_portfile_t* sysdep)
{
    sysdep_test_result_t ret = TEST_SUCCESS;
    heap_cfg_t hp;
    hp.total_size = HEAP_MAX;
    ret = heap_malloc_max_test(sysdep,&hp);
    if(ret != TEST_SUCCESS) {
        return ret;
    }
    return heap_recycle_test(sysdep, &hp);
}

static sysdep_test_result_t network_tcp_test(aiot_sysdep_portfile_t* sysdep, network_cfg_t* cfg)
{
    void* network_hd = sysdep->core_sysdep_network_init();
    core_sysdep_socket_type_t type = CORE_SYSDEP_SOCKET_TCP_CLIENT;
    sysdep->core_sysdep_network_setopt(network_hd, CORE_SYSDEP_NETWORK_SOCKET_TYPE, &type);
    sysdep->core_sysdep_network_setopt(network_hd, CORE_SYSDEP_NETWORK_HOST, cfg->host);
    sysdep->core_sysdep_network_setopt(network_hd, CORE_SYSDEP_NETWORK_PORT, (void*)&cfg->port);
    sysdep->core_sysdep_network_setopt(network_hd, CORE_SYSDEP_NETWORK_CONNECT_TIMEOUT_MS, &cfg->connect_timeout_ms);

    /**
     *Connection test*/
    if(0 != sysdep->core_sysdep_network_establish(network_hd)) {
        LUAT_DEBUG_PRINT("[linksdk]network establish test error:%d", TEST_ERR_NETWORK);
        return TEST_ERR_NETWORK;
    }

    /**
     * Receive wait timeout test*/
    char* buf = sysdep->core_sysdep_malloc(cfg->recv_buf_len + 1, "");
    if(buf == NULL){
        LUAT_DEBUG_PRINT("[linksdk]malloc fail");
        return TEST_ERR_MALLOC;
    }
    uint64_t start = sysdep->core_sysdep_time();
    sysdep->core_sysdep_network_recv(network_hd, (uint8_t*)buf, cfg->recv_buf_len, cfg->recv_timeout_ms, NULL);
    uint64_t stop = sysdep->core_sysdep_time();
    if(stop - start < cfg->recv_timeout_ms) {
        LUAT_DEBUG_PRINT("[linksdk]network receive test error:%d, start [%ld], end [%ld]", TEST_ERR_NETWORK, start, stop);
        return TEST_ERR_NETWORK;
    }

    /**
     * Verify whether TCP is working properly through Http Get request*/
    char* http_get = "GET / HTTP/1.1\r\n"\
                     "Host: www.aliyun.com\r\n"\
                     "Connection: keep-alive\r\n"\
                     "User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/96.0.4664.110 Safari/537.36\r\n"\
                     "Origin: https://www.aliyun.com\r\n"\
                     "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9\r\n"\
                     "\r\n";
                     
    int32_t send_len = strlen(http_get);
    int32_t send_result = sysdep->core_sysdep_network_send(network_hd, (uint8_t*)http_get, send_len, cfg->send_timeout_ms, NULL);
    if(send_result != send_len) {
        LUAT_DEBUG_PRINT("[linksdk]network send length test error:%d, send [%d], result [%d]", TEST_ERR_NETWORK, send_len, send_result);
        return TEST_ERR_NETWORK;
    }

    int32_t buf_len = 0;
    uint8_t byte_read;
    int32_t content_len = 0;
    int32_t ret;
    char* str_ptr = NULL;
    char* target_str = "Content-Length:";
    int state = 0;
    while(1) {
        ret = sysdep->core_sysdep_network_recv(network_hd, (uint8_t*)&byte_read, 1, cfg->recv_timeout_ms, NULL);
        /*Need to support byte reading*/
        if(ret == 1) {
            if(state == 0) {
                /*Extract a row*/
                buf[buf_len++] = byte_read;
                if(byte_read != '\n') {
                    continue;
                }
                buf[buf_len] = '\0';
                /*A blank line indicates the end of the header*/
                if(buf_len == 2 && buf[0] == '\r' && buf[1] == '\n') {
                    state = 1;
                    buf_len = 0;
                    continue;
                }
                /*Search response length*/
                str_ptr = strstr(buf,target_str);
                if(str_ptr == NULL) {
                    buf_len = 0;
                    continue;
                }
                str_ptr += strlen(target_str);
                if(str_ptr >= &buf[buf_len]) {
                    buf_len = 0;
                    continue;
                }
                if( 1 == sscanf(str_ptr,"%d",&content_len)) {
                    buf_len = 0;
                }
            } else {
                buf[buf_len++] = byte_read;
            }
        }
        else {
            break;
        }
    }
    sysdep->core_sysdep_network_deinit(&network_hd);
    sysdep->core_sysdep_free(buf);
    buf = NULL;
    if(network_hd != NULL){
        LUAT_DEBUG_PRINT("[linksdk]network deinit error");
        return TEST_ERR_NETWORK;
    }
    /*http body length check*/
    if(buf_len == 0 || buf_len != content_len) {
        LUAT_DEBUG_PRINT("[linksdk][NETWORK_TEST.RECV] test error:%d, send [%d], result [%d]", TEST_ERR_NETWORK, buf_len, content_len);
        return TEST_ERR_NETWORK;
    } else {
        LUAT_DEBUG_PRINT("[linksdk][NETWORK_TEST.RECV] test success");
        return TEST_SUCCESS;
    }
}

sysdep_test_result_t network_tcp_cycle_test(aiot_sysdep_portfile_t* sysdep, network_cfg_t* cfg)
{
    void* network_hd =  NULL;
    int count = cfg->cycle_test_times;
    while(count-- > 0) {
        network_hd = sysdep->core_sysdep_network_init();
        if(network_hd == NULL) {
            LUAT_DEBUG_PRINT("[linksdk]network tcp init test error");
            return TEST_ERR_NETWORK;
        }
        core_sysdep_socket_type_t type = CORE_SYSDEP_SOCKET_TCP_CLIENT;
        sysdep->core_sysdep_network_setopt(network_hd, CORE_SYSDEP_NETWORK_SOCKET_TYPE, &type);
        sysdep->core_sysdep_network_setopt(network_hd, CORE_SYSDEP_NETWORK_HOST, cfg->host);
        sysdep->core_sysdep_network_setopt(network_hd, CORE_SYSDEP_NETWORK_PORT, (void*)&cfg->port);
        sysdep->core_sysdep_network_setopt(network_hd, CORE_SYSDEP_NETWORK_CONNECT_TIMEOUT_MS, &cfg->connect_timeout_ms);

        if(0 != sysdep->core_sysdep_network_establish(network_hd)) {
            LUAT_DEBUG_PRINT("[linksdk]network tcp establish test error:%d", TEST_ERR_NETWORK);
            return TEST_ERR_NETWORK;
        }
        sysdep->core_sysdep_network_deinit(&network_hd);
    }
    if( TEST_SUCCESS != heap_test(sysdep) ) {
        LUAT_DEBUG_PRINT("[linksdk]network deinit then heap test error:%d", TEST_ERR_NETWORK);
        return TEST_ERR_NETWORK;
    } else {
        return TEST_SUCCESS;
    }
}

static sysdep_test_result_t network_tcp_send_length_test(aiot_sysdep_portfile_t* sysdep, network_cfg_t* cfg)
{
    sysdep_test_result_t ret = TEST_SUCCESS;
    void* network_hd = sysdep->core_sysdep_network_init();

    core_sysdep_socket_type_t type = CORE_SYSDEP_SOCKET_TCP_CLIENT;
    sysdep->core_sysdep_network_setopt(network_hd, CORE_SYSDEP_NETWORK_SOCKET_TYPE, &type);
    sysdep->core_sysdep_network_setopt(network_hd, CORE_SYSDEP_NETWORK_HOST, cfg->host);
    sysdep->core_sysdep_network_setopt(network_hd, CORE_SYSDEP_NETWORK_PORT, (void*)&cfg->port);
    sysdep->core_sysdep_network_setopt(network_hd, CORE_SYSDEP_NETWORK_CONNECT_TIMEOUT_MS, &cfg->connect_timeout_ms);

    /**
     *Connection test*/
    if(0 != sysdep->core_sysdep_network_establish(network_hd)) {
        LUAT_DEBUG_PRINT("[linksdk]network establish test error:%d", TEST_ERR_NETWORK);
        return TEST_ERR_NETWORK;
    }
    /**
     * Use Http Get request to verify whether the TCP sending interface has the ability to send long packets*/
    const char* http_get = "GET / HTTP/1.1\r\n"\
                     "Host: www.aliyun.com\r\n"\
                     "Connection: keep-alive\r\n"\
                     "User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/96.0.4664.110 Safari/537.36\r\n"\
                     "Origin: https://www.aliyun.com\r\n"\
                     "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9\r\n"\
                     "\r\n"\
                     "invalid test append data,invalid test append data,invalid test append data,invalid test append data,"\
                     "invalid test append data,invalid test append data,invalid test append data,invalid test append data,"\
                     "invalid test append data,invalid test append data,invalid test append data,invalid test append data,"\
                     "invalid test append data,invalid test append data,invalid test append data,invalid test append data,"\
                     "invalid test append data,invalid test append data,invalid test append data,invalid test append data,"\
                     "invalid test append data,invalid test append data,invalid test append data,invalid test append data,"\
                     "invalid test append data,invalid test append data,invalid test append data,invalid test append data,"\
                     "invalid test append data,invalid test append data,invalid test append data,invalid test append data,"\
                     "invalid test append data,invalid test append data,invalid test append data,invalid test append data,"\
                     "invalid test append data,invalid test append data,invalid test append data,invalid test append data,"\
                     "invalid test append data,invalid test append data,invalid test append data,invalid test append data,"\
                     "invalid test append data,invalid test append data,invalid test append data,invalid test append data,"\
                     "invalid test append data,invalid test append data,invalid test append data,invalid test append data,"\
                     "invalid test append data,invalid test append data,invalid test append data,invalid test append data,"\
                     "invalid test append data,invalid test append data,invalid test append data,invalid test append data,"\
                     "invalid test append data,invalid test append data,invalid test append data,invalid test append data,"\
                     "invalid test append data,invalid test append data,invalid test append data,invalid test append data,"\
                     "invalid test append data,invalid test append data,invalid test append data,invalid test append data,"\
                     "invalid test append data,invalid test append data,invalid test append data,invalid test append data,"\
                     "invalid test append data,invalid test append data,invalid test append data,invalid test append data,";
                                 
    int32_t send_len = strlen(http_get);
    int32_t send_result = sysdep->core_sysdep_network_send(network_hd, (uint8_t*)http_get, send_len, cfg->send_timeout_ms, NULL);
    if(send_result != send_len) {
        LUAT_DEBUG_PRINT("[linksdk]network send length test error:%d, send [%d], result [%d]", TEST_ERR_NETWORK, send_len, send_result);
        ret = TEST_ERR_NETWORK;
    }
    sysdep->core_sysdep_network_deinit(&network_hd);
    return ret;
}


sysdep_test_result_t network_test(aiot_sysdep_portfile_t* sysdep)
{
    sysdep_test_result_t ret;
    sysdep->core_sysdep_sleep(100);

    network_cfg_t cfg;
    cfg.host = "www.aliyun.com";
    cfg.port = 80;
    cfg.connect_timeout_ms = 5000;
    cfg.recv_timeout_ms = 5000;
    cfg.recv_buf_len = 1024;
    cfg.send_timeout_ms = 5000;
    cfg.cycle_test_times = 10;

    ret = network_tcp_test(sysdep, &cfg);
    if(ret != TEST_SUCCESS) {
        return ret;
    }

    ret = network_tcp_send_length_test(sysdep,&cfg);
    if(ret != TEST_SUCCESS){
        return ret;
    }
    ret = network_tcp_cycle_test(sysdep, &cfg);
    return ret;
}

static void enter_sleep1(void* user_data)
{
    task_handler_input_t* user = (task_handler_input_t*)user_data;
    time_cfg_t* tm = user->user_data;

    LUAT_DEBUG_PRINT("[linksdk]%s enter wanna sleep: %ldms", user->name, tm->sleep_ms);
    user->sysdep->core_sysdep_sleep(tm->sleep_ms);

    tm->sleep_end = 1;
    luat_rtos_task_delete(sleep_task_handle1);
    return 0;
}

static void enter_sleep2(void* user_data)
{
    task_handler_input_t* user = (task_handler_input_t*)user_data;
    time_cfg_t* tm = user->user_data;

    LUAT_DEBUG_PRINT("[linksdk]%s enter wanna sleep: %ldms", user->name, tm->sleep_ms);
    user->sysdep->core_sysdep_sleep(tm->sleep_ms);

    tm->sleep_end = 1;
    luat_rtos_task_delete(sleep_task_handle2);
    return 0;
}

sysdep_test_result_t time_sleep_test(aiot_sysdep_portfile_t* sysdep)
{
    uint64_t start,stop;
    time_cfg_t time_cfg;
    sysdep_test_result_t ret= TEST_SUCCESS;
    time_cfg_t* sleep1 = NULL;
    time_cfg_t* sleep2 = NULL;
    task_handler_input_t input1;
    task_handler_input_t input2;
    int64_t wait_total;
    uint64_t temp;

    time_cfg.sleep_ms = 10 * 1000;
    LUAT_DEBUG_PRINT("[linksdk]sleep %ldms test",time_cfg.sleep_ms);
    start = sysdep->core_sysdep_time();
    sysdep->core_sysdep_sleep(time_cfg.sleep_ms);
    stop = sysdep->core_sysdep_time();
    LUAT_DEBUG_PRINT("[linksdk]sleep start %ld stop %ld ms test",start,stop);
    /*Abnormal sleep*/
    if((stop - start) * 1.1 < time_cfg.sleep_ms) {
        LUAT_DEBUG_PRINT("[linksdk]sleep test error: %d",TEST_ERR_SLEEP);
        ret = TEST_ERR_SLEEP;
        goto end;
    }

    /*Sleeping for more than 10% of the time*/
    if(stop - start > (uint64_t)(time_cfg.sleep_ms * 1.1)) {
        LUAT_DEBUG_PRINT("[linksdk]sleep test error: %d",TEST_ERR_SLEEP);
        ret = TEST_ERR_SLEEP;
        goto end;
    }

    /**
     * Concurrent sleep test, the time consumed by two concurrent sleeps should be less than the sum of the two sleep times*/
    sleep1 = sysdep->core_sysdep_malloc(sizeof(time_cfg_t),"");
    sleep2 = sysdep->core_sysdep_malloc(sizeof(time_cfg_t),"");
    if(sleep1 == NULL || sleep2 == NULL){
        ret = TEST_ERR_SLEEP;
        goto end;
    }

    sleep1->sleep_ms = 10 * 1000;
    sleep1->sleep_end = 0;
    sleep2->sleep_ms = 10 * 1000;
    sleep2->sleep_end = 0;

    input1.name = "sleep_test_task_1";
    input1.sysdep = sysdep;
    input1.user_data = sleep1;

    input2.name = "sleep_test_task_2";
    input2.sysdep = sysdep;
    input2.user_data = sleep2;

    start = sysdep->core_sysdep_time();
 
    luat_rtos_task_create(&sleep_task_handle1, 1024, 20, "", enter_sleep1, &input1, NULL);
    luat_rtos_task_create(&sleep_task_handle2, 1024, 20, "", enter_sleep2, &input2, NULL);

    /**
     * Wait for sleep to end*/
    wait_total = sleep1->sleep_ms + sleep2->sleep_ms + 100;
    while((sleep1->sleep_end == 0 || sleep2->sleep_end == 0) && wait_total-- > 0) {
        sysdep->core_sysdep_sleep(1);
    }

    /**
     * Check if all threads have exited*/
    while (sleep1->sleep_end == 0 || sleep2->sleep_end == 0){
        sysdep->core_sysdep_sleep(1);
    }

    /*Both threads should exit within the specified time*/
    if(wait_total < 0) {
        LUAT_DEBUG_PRINT("[linksdk]concurrent sleep test error: wait sleep timeout fail");
        ret = TEST_ERR_SLEEP;
        goto end;
    }

    /*The total sleep time should be less than the sum of each sleep time*/
    stop = sysdep->core_sysdep_time();
    temp = sleep1->sleep_ms + sleep2->sleep_ms;
    if(stop - start >= temp) {
        LUAT_DEBUG_PRINT("[linksdk]sleep %ldms start:[%ld] stop:[%ld]\r\n unexpected ", sleep1->sleep_ms, start, stop);
        ret = TEST_ERR_SLEEP;
    }
    else {
        LUAT_DEBUG_PRINT("[linksdk]sleep %ldms start:[%ld] stop:[%ld] expected ", sleep1->sleep_ms, start, stop);
        ret = TEST_SUCCESS;
    }
end:
    if(sleep1 != NULL)
        sysdep->core_sysdep_free(sleep1);
    if(sleep2 != NULL)
        sysdep->core_sysdep_free(sleep2);
    return ret;
}

static void mutex_synchronize_test1(void* user_data)
{
    task_handler_input_t* input = (task_handler_input_t*)user_data;
    mutex_cfg_t* cfg = input->user_data;
    while(1) {
        input->sysdep->core_sysdep_mutex_lock(cfg->mutex);
        if(cfg->value < 0) {
            input->sysdep->core_sysdep_mutex_unlock(cfg->mutex);
            break;
        }
        cfg->value++;
        input->sysdep->core_sysdep_mutex_unlock(cfg->mutex);
        input->sysdep->core_sysdep_sleep(cfg->timeout_ms);
    }
    luat_rtos_task_delete(mutex_task_handle1);
    return NULL;
}
static void mutex_synchronize_test2(void* user_data)
{
    task_handler_input_t* input = (task_handler_input_t*)user_data;
    mutex_cfg_t* cfg = input->user_data;
    while(1) {
        input->sysdep->core_sysdep_mutex_lock(cfg->mutex);
        if(cfg->value < 0) {
            input->sysdep->core_sysdep_mutex_unlock(cfg->mutex);
            break;
        }
        cfg->value++;
        input->sysdep->core_sysdep_mutex_unlock(cfg->mutex);
        input->sysdep->core_sysdep_sleep(cfg->timeout_ms);
    }
    luat_rtos_task_delete(mutex_task_handle2);
    return NULL;
}

/**
 * Mutex lock test*/
sysdep_test_result_t mutex_test(aiot_sysdep_portfile_t* sysdep)
{
    sysdep_test_result_t ret;
    void* mutex = NULL;
    mutex_cfg_t *mutex_cfg_2 = NULL;
    task_handler_input_t *input_1 = NULL;
    task_handler_input_t *input_2 = NULL;
    int32_t wait_ms = 3000;
    /**
     * Mutex lock application release and memory leak testing*/
    for(int i = 0; i < 1000; i++) {
        mutex = sysdep->core_sysdep_mutex_init();
        sysdep->core_sysdep_mutex_lock(mutex);
        sysdep->core_sysdep_mutex_unlock(mutex);
        sysdep->core_sysdep_mutex_deinit(&mutex);
    }
    if( TEST_SUCCESS != heap_test(sysdep)) {
        LUAT_DEBUG_PRINT("[linksdk][MUTEX_TEST.CYCLE] test failed\r\n");
        return ret = TEST_ERR_MUTEX;
    }

    /**
     * Mutex lock function test*/
    mutex_cfg_t *mutex_cfg_1 = sysdep->core_sysdep_malloc(sizeof(mutex_cfg_t), "");
    if(mutex_cfg_1 == NULL) {
        LUAT_DEBUG_PRINT("[linksdk]mutex test but malloc fail");
        ret = TEST_ERR_MUTEX;
        goto end;
    }
    mutex_cfg_1->mutex = sysdep->core_sysdep_mutex_init();
    if(mutex_cfg_1->mutex == NULL) {
        ret = TEST_ERR_MALLOC;
        LUAT_DEBUG_PRINT("[linksdk][MUTEX_TEST]malloc fail:%d", ret);
        goto end;
    }
    input_1 = sysdep->core_sysdep_malloc(sizeof(task_handler_input_t), "");
    if(input_1 == NULL){
        LUAT_DEBUG_PRINT("[linksdk]mutex test but malloc fail");
        ret = TEST_ERR_MALLOC;
        goto end;
    }
    input_1->name = "mutex_test_task1";
    input_1->sysdep = sysdep;
    input_1->user_data = mutex_cfg_1;
    mutex_cfg_1->timeout_ms = 100;
    mutex_cfg_1->value = 0;
    luat_rtos_task_create(&mutex_task_handle1, 1024, 20, "", mutex_synchronize_test1, input_1, NULL);

    mutex_cfg_2 = sysdep->core_sysdep_malloc(sizeof(mutex_cfg_t), "");
    if(mutex_cfg_2 == NULL) {
        ret = TEST_ERR_MALLOC;
        LUAT_DEBUG_PRINT("[linksdk][MUTEX_TEST]malloc fail:%d", ret);
        goto end;
    }
    mutex_cfg_2->mutex = sysdep->core_sysdep_mutex_init();
    if(mutex_cfg_2->mutex == NULL) {
        ret = TEST_ERR_MALLOC;
        LUAT_DEBUG_PRINT("[linksdk][MUTEX_TEST]malloc fail:%d", ret);
        goto end;
    }
    input_2 = sysdep->core_sysdep_malloc(sizeof(task_handler_input_t), "");
    input_2->name = "mutex_test_task2";
    input_2->sysdep = sysdep;
    input_2->user_data = mutex_cfg_2;
    mutex_cfg_2->timeout_ms = 100;
    mutex_cfg_2->value = 0;
    luat_rtos_task_create(&mutex_task_handle2, 1024, 20, "", mutex_synchronize_test2, input_2, NULL);

    sysdep->core_sysdep_sleep(wait_ms);
    /**
     * Lock mutex_cfg_1->mutex, mutex_test_task1 will be locked, mutex_cfg_1->value should not change*/
    int32_t v1,v2;
    LUAT_DEBUG_PRINT("[linksdk]mutex lock task1, unlock task2 %d ms", wait_ms);
    sysdep->core_sysdep_mutex_lock(mutex_cfg_1->mutex);

    v1 = mutex_cfg_1->value;
    v2 = mutex_cfg_2->value;

    sysdep->core_sysdep_sleep(wait_ms);

    int32_t v1_,v2_;
    v1_ = mutex_cfg_1->value;
    v2_ = mutex_cfg_2->value;
    LUAT_DEBUG_PRINT("[linksdk]task1 value [%d --> %d], task2 value [%d --> %d] ", v1, v1_, v2, v2_);
    if(v1 != v1_ || v2 == v2_) {
        LUAT_DEBUG_PRINT("[linksdk]mutex test failed ");
        ret = TEST_ERR_MUTEX;
        goto end;
    }
    sysdep->core_sysdep_mutex_unlock(mutex_cfg_1->mutex);


    /**
     * Lock mutex_cfg_2->mutex, mutex_test_task2 will be locked, mutex_cfg_2->value should not change*/
    LUAT_DEBUG_PRINT("[linksdk]unlock task1, lock task2 %d ms", wait_ms);
    sysdep->core_sysdep_mutex_lock(mutex_cfg_2->mutex);
    v1 = mutex_cfg_1->value;
    v2 = mutex_cfg_2->value;

    sysdep->core_sysdep_sleep(wait_ms);

    v1_ = mutex_cfg_1->value;
    v2_ = mutex_cfg_2->value;

    LUAT_DEBUG_PRINT("[linksdk]task1 value [%d --> %d], task2 value [%d --> %d] ", v1, v1_, v2, v2_);
    if(v1 == v1_ || v2 != v2_) {
        LUAT_DEBUG_PRINT("[linksdk]mutex test failed");
        ret = TEST_ERR_MUTEX;
        goto end;
    }
    sysdep->core_sysdep_mutex_unlock(mutex_cfg_2->mutex);

    /**
     * mutex_cfg_2->mutex is not locked, mutex_cfg_1->mutex is not locked, and the corresponding value should change normally.*/
    LUAT_DEBUG_PRINT("[linksdk]unlock task1, lock task2 %d ms\r\n", wait_ms);
    v1 = mutex_cfg_1->value;
    v2 = mutex_cfg_2->value;
    sysdep->core_sysdep_sleep(wait_ms);
    v1_ = mutex_cfg_1->value;
    v2_ = mutex_cfg_2->value;
    LUAT_DEBUG_PRINT("[linksdk]task1 value [%d --> %d], task2 value [%d --> %d]\r\n", v1, v1_, v2, v2_);
    if(v1 == v1_ || v2 == v2_) {
        LUAT_DEBUG_PRINT("[linksdk][MUTEX_TEST.UNLOCK] test failed \r\n");
        ret = TEST_ERR_MUTEX;
        goto end;
    }
end:
    /**
     * Thread exits and returns*/
    if(mutex_cfg_1 != NULL) {
        mutex_cfg_1->value = -10;
        sysdep->core_sysdep_sleep(500);
        if(mutex_cfg_1->mutex != NULL){
            sysdep->core_sysdep_mutex_deinit(&(mutex_cfg_1->mutex));
        }

        sysdep->core_sysdep_free(mutex_cfg_1);
    }
    
    if(mutex_cfg_2 != NULL){
        mutex_cfg_2->value = -10;
        sysdep->core_sysdep_sleep(500);
        if(mutex_cfg_2->mutex != NULL){
            sysdep->core_sysdep_mutex_deinit(&(mutex_cfg_2->mutex));
        }
        sysdep->core_sysdep_free(mutex_cfg_2);
    }

    if(input_1 != NULL){
        sysdep->core_sysdep_free(input_1);
    }
    if(input_2 != NULL){
        sysdep->core_sysdep_free(input_2);
    }

    return TEST_SUCCESS;
}

typedef struct {
    char*            name;
    sysdep_test_func func;
} sysdep_test_suite;

/**
 * List of test items*/
sysdep_test_suite test_list[]= {
    {"RANDOM_TEST ", random_test},
    {"HEAP_TEST   ", heap_test},
    {"TIME_TEST   ", time_sleep_test},
    {"NETWORK_TEST", network_test},
    {"MUTEX_TEST  ", mutex_test},
};

/**
 * sysdep interface implementation, including system time, memory management, network, lock, random number, etc. interface implementation*/
extern aiot_sysdep_portfile_t g_aiot_sysdep_portfile;

static void demo_sysdep_api_test(void *param)
{
    luat_rtos_task_sleep(1000);
    aiot_sysdep_portfile_t *sysdep = &g_aiot_sysdep_portfile;
    int32_t size = sizeof(test_list) / sizeof(test_list[0]);
    sysdep_test_result_t ret = TEST_SUCCESS;
    aiot_sysdep_set_portfile(&g_aiot_sysdep_portfile);
    LUAT_DEBUG_PRINT("[linksdk]TOTAL TEST START");
    for(int32_t i = 0; i < size; i++) {
        LUAT_DEBUG_PRINT("[linksdk]TEST [%d/%d] [%s] .....................................[START]", i + 1, size, test_list[i].name);
        ret = (test_list[i].func)(sysdep);
        if(TEST_SUCCESS != ret) {
            LUAT_DEBUG_PRINT("[linksdk]TEST [%d/%d] [%s] .....................................[FAILED] [%s]", i + 1, size, test_list[i].name, result_string[ret]);
            break;
        } else {
            LUAT_DEBUG_PRINT("[linksdk]TEST [%d/%d] [%s] .....................................[SUCCESS]", i + 1, size, test_list[i].name);
        }
    }
    if(ret == TEST_SUCCESS) {
        LUAT_DEBUG_PRINT("[linksdk]TOTAL TEST SUCCESS");
    } else {
        LUAT_DEBUG_PRINT("[linksdk]TOTAL TEST FAILED %s", result_string[ret]);
    }
    luat_rtos_task_delete(linksdk_task_handle);
}

static void task_demo_init(void)
{
    luat_rtos_task_create(&linksdk_task_handle, 4096, 20, "", demo_sysdep_api_test, NULL, NULL);
}

INIT_TASK_EXPORT(task_demo_init, "1");
