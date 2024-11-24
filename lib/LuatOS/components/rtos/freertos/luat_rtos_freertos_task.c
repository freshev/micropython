
#ifdef LUAT_FREERTOS_FULL_INCLUDE
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#else
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"
#endif


#include "luat_base.h"
#include "luat_rtos.h"

typedef struct
{
	uint32_t ID;
	uint32_t Param1;
	uint32_t Param2;
	uint32_t Param3;
}OS_EVENT;


typedef void (*IrqHandler)(int32_t IrqLine, void *pData);
typedef void (* TaskFun_t)( void * );
typedef void (* CommonFun_t)(void);
typedef void(* CBDataFun_t)(uint8_t *Data, uint32_t Len);
typedef int32_t(*CBFuncEx_t)(void *pData, void *pParam);
typedef uint64_t LongInt;

/**
 * @brief Create a task with an event sending and receiving mechanism. The event is a 16-byte queue. When creating the task, a queue is also created. See OS_EVENT for the event structure.
 *
 * @param task_fun task entry function
 * @param param entry parameter of task
 * @param stack_bytes The stack length of the task, in bytes, will force 4-byte alignment
 * @param priority task priority, note that it is a percentage, 0~100, 100 is the highest level allowed by the underlying OS, 0 is the lowest level allowed by the underlying OS
 * @param task_name task name
 * @param event_max_cnt, if the OS does not have mailbox, this parameter is needed to create the queue
 * @return void* The handle of the task. This parameter is required for subsequent sending and receiving events. NULL means the creation failed.*/
void *create_event_task(TaskFun_t task_fun, void *param, uint32_t stack_bytes, uint8_t priority, uint16_t event_max_cnt, const char *task_name);

/**
 * @brief Deleting a task with an event sending and receiving mechanism requires one more step than deleting the task normally to delete the event queue.
 *
 * @param task_handle task handle*/
void delete_event_task(void *task_handle);

/**
 * @brief Send an event to the task
 *
 * @param task_handle task handle
 * @param event is an event that has been constructed. If the incoming pointer is not NULL, the subsequent four parameters will be ignored. Otherwise, an event will be constructed from the subsequent four parameters. Each function parameter corresponds to the parameter with the same name in the event.
 * @param event_id the event id that needs to be constructed
 * @param param1 param1 that needs to be built
 * @param param2 param2 that needs to be built
 * @param param3 param3 that needs to be built
 * @param timeout_ms sent timeout, 0 does not wait, 0xffffffff waits forever, it is recommended to write 0 directly
 * @return int Returns 0 if successful, otherwise it will fail*/
int send_event_to_task(void *task_handle, OS_EVENT *event, uint32_t event_id, uint32_t param1, uint32_t param2, uint32_t param3, uint32_t timeout_ms);

/**
 * @brief Get an event and return it as needed
 * If target_event_id != 0 && != 0xffffffff, then it will be returned when the corresponding event id is received. If not, the callback will be handed over to the user for temporary processing. If the callback is empty, it will be discarded.
 * If target_event_id == 0, return after receiving the message
 * If target_event_id == 0xffffffff, when the message is received, the callback will be handed over to the user for temporary processing. If the callback is empty, it will be discarded.
 *
 * @param task_handle task handle
 * @param target_event_id specifies the received event id
 * @param event The space for caching events. When the required event is received, it is cached here.
 * @param callback When an unnecessary event is received, the callback is given to the user for processing. The first parameter in the callback function is the event pointer, and the second parameter is the task handle. This can be NULL to discard the event directly.
 * @param timeout_ms 0 and 0xffffffff wait forever, it is recommended to write 0 directly
 * @return int Returns 0 after receiving the required event*/
int get_event_from_task(void *task_handle, uint32_t target_event_id, OS_EVENT *event,  CBFuncEx_t callback, uint32_t timeout_ms);

int luat_rtos_task_create(luat_rtos_task_handle *task_handle, uint32_t stack_size, uint8_t priority, const char *task_name, luat_rtos_task_entry task_fun, void* user_data, uint16_t event_cout)
{
	if (!task_handle) return -1;
	*task_handle = create_event_task(task_fun, user_data, stack_size, priority, event_cout, task_name);
	return (*task_handle)?0:-1;
}

int luat_rtos_task_delete(luat_rtos_task_handle task_handle)
{
	if (!task_handle) return -1;
	delete_event_task(task_handle);
	return 0;
}

int luat_rtos_task_suspend(luat_rtos_task_handle task_handle)
{
	if (!task_handle) return -1;
	vTaskSuspend(task_handle);
	return 0;
}

int luat_rtos_task_resume(luat_rtos_task_handle task_handle)
{
	if (!task_handle) return -1;
	vTaskResume(task_handle);
	return 0;
}

void luat_rtos_task_sleep(uint32_t ms)
{
	vTaskDelay(ms);
}

void luat_task_suspend_all(void)
{
	vTaskSuspendAll();
}

void luat_task_resume_all(void)
{
	xTaskResumeAll();
}

void *luat_get_current_task(void)
{
	return xTaskGetCurrentTaskHandle();
}


int luat_rtos_event_send(luat_rtos_task_handle task_handle, uint32_t id, uint32_t param1, uint32_t param2, uint32_t param3, uint32_t timeout)
{
	if (!task_handle) return -1;
	return send_event_to_task(task_handle, NULL, id, param1, param2, param3, timeout);
}

int luat_rtos_event_recv(luat_rtos_task_handle task_handle, uint32_t wait_event_id, luat_event_t *out_event, luat_rtos_event_wait_callback_t *callback_fun, uint32_t timeout){
	if (!task_handle) return -1;
	return get_event_from_task(task_handle, wait_event_id, (void *)out_event, (CBFuncEx_t)callback_fun, timeout);
}

int luat_send_event_to_task(void *task_handle, uint32_t id, uint32_t param1, uint32_t param2, uint32_t param3)
{
	if (!task_handle) return -1;
	return send_event_to_task(task_handle, NULL, id, param1, param2, param3, LUAT_WAIT_FOREVER);
}

int luat_wait_event_from_task(void *task_handle, uint32_t wait_event_id, luat_event_t *out_event, void *call_back, uint32_t ms)
{
	if (!task_handle) return -1;
	return get_event_from_task(task_handle, wait_event_id, (void *)out_event, (CBFuncEx_t)call_back, ms);
}

/* ------------------------------------------------ critical begin----------------------------------------------- */
/**
 * @brief Enter critical protection
 *
 * @return uint32_t Parameters required to exit critical protection*/
uint32_t luat_rtos_entry_critical(void)
{
	luat_os_entry_cri();
    return 0;
}

/**
 * @brief Exit critical protection
 *
 * @param critical parameter returned when entering critical protection*/
void luat_rtos_exit_critical(uint32_t critical)
{
	luat_os_exit_cri();
}
