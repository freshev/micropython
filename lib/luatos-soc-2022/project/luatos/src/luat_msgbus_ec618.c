#include "luat_base.h"
#include "luat_msgbus.h"
#include "luat_rtos.h"
#include "luat_rtos_legacy.h"
#include "common_api.h"

#if 0
#include "FreeRTOS.h"
#include "queue.h"
#define QUEUE_LENGTH 0xFF
#define ITEM_SIZE sizeof(rtos_msg_t)

static StaticQueue_t xStaticQueue = {0};
static QueueHandle_t xQueue = {0};

#if configSUPPORT_STATIC_ALLOCATION
static uint8_t ucQueueStorageArea[ QUEUE_LENGTH * ITEM_SIZE ] __attribute__((aligned (16)));
#endif

void luat_msgbus_init(void) {
    if (!xQueue) {
        #if configSUPPORT_STATIC_ALLOCATION
        xQueue = xQueueCreateStatic( QUEUE_LENGTH,
                                 ITEM_SIZE,
                                 ucQueueStorageArea,
                                 &xStaticQueue );
        #else
        xQueue = xQueueCreate(QUEUE_LENGTH, ITEM_SIZE);
        #endif
    }
}
uint32_t luat_msgbus_put(rtos_msg_t* msg, size_t timeout) {
    if (xQueue == NULL)
        return 1;
    return xQueueSendFromISR(xQueue, msg, NULL) == pdTRUE ? 0 : 1;
}
uint32_t luat_msgbus_get(rtos_msg_t* msg, size_t timeout) {
    if (xQueue == NULL)
        return 1;
    return xQueueReceive(xQueue, msg, timeout) == pdTRUE ? 0 : 1;
}
uint32_t luat_msgbus_freesize(void) {
    if (xQueue == NULL)
        return 1;
    return 1;
}
#else
#include "luat_rtos.h"
static void * prvTaskHandle;
void luat_msgbus_init(void) {
	prvTaskHandle = luat_rtos_get_current_handle();
}

uint32_t luat_msgbus_put(rtos_msg_t* msg, size_t timeout) {
	return send_event_to_task(prvTaskHandle, msg, 0, 0, 0, 0, 0);
}

uint32_t luat_msgbus_get(rtos_msg_t* msg, size_t timeout) {
    return get_event_from_task(prvTaskHandle, 0, msg, NULL, timeout);
}

uint32_t luat_msgbus_freesize(void) {
    return 1024 - get_event_cnt(prvTaskHandle);
}

uint8_t luat_msgbus_is_empty(void) {return !get_event_cnt(prvTaskHandle);}

uint8_t luat_msgbus_is_ready(void) {
	return prvTaskHandle?1:0;
}
#endif
