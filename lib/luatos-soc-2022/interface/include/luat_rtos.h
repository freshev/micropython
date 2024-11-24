/*
 * Copyright (c) 2022 OpenLuat & AirM2M
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef LUAT_RTOS_H
#define LUAT_RTOS_H

#include "luat_base.h"
#include "luat_rtos_legacy.h"

/**
 * @defgroup luatos_os operating system interface
 * @{*/

/**
 * @brief LUAT_RTOS timeout enumeration value*/
typedef enum LUAT_RTOS_WAIT
{
	LUAT_NO_WAIT = 0,						 /**< timeout is 0*/
	LUAT_WAIT_FOREVER = (uint32_t)0xFFFFFFFF /**<Maximum timeout 0xFFFFFFFF*/
} LUAT_RTOS_WAIT_E;

typedef enum
{
	LUAT_FLAG_AND 		= 5,
	LUAT_FLAG_AND_CLEAR 	= 6,
	LUAT_FLAG_OR 			= 7,
	LUAT_FLAG_OR_CLEAR	= 8
} LUAT_FLAG_OP_E;

/* ------------------------------------------------ task begin------------------------------------------------ */
/**
 * @defgroup luatos_os_Task thread task interface function
 * @{*/

/**
 *@brief task entry function, function type*/
typedef void (*luat_rtos_task_entry) (void*);
/**
 *@brief defines task task handle*/
typedef void * luat_rtos_task_handle;
/**
 * @brief Create a task that can have a mailbox mechanism. Mailbox is the basis for messages and events, but has nothing to do with queues.
 *
 * @param task_handle[OUT] returns the created handle
 * @param stack_size The stack space size of the task, in byte, must be aligned with 4 bytes
 * @param priority priority, unit is percentage, 0%~100%, 100% is the highest level, the priority used by the specific implementation to the underlying SDK
 * @param task_name task name
 * @param task_fun task entry function
 * @param user_data task entry parameter
 * @param event_cout If the OS allows malloc in interrupts, or does not use message and event mechanisms, this parameter is ignored. If the OS does not allow malloc in interrupts, fill in the pre-allocated event space here for use in interrupts. If written 0 will use public events
 * @return int =0 success, others failure*/
int luat_rtos_task_create(luat_rtos_task_handle *task_handle, uint32_t stack_size, uint8_t priority, const char *task_name, luat_rtos_task_entry task_fun, void* user_data, uint16_t event_cout);

/**
 * @brief delete task
 *
 * @param task_handle
 * @return int =0 success, others failure*/
int luat_rtos_task_delete(luat_rtos_task_handle task_handle);

/**
 * @brief suspend a task
 *
 * @param task_handle task handle
 * @return int =0 success, others failure*/
int luat_rtos_task_suspend(luat_rtos_task_handle task_handle);

/**
 * @brief Resume suspended tasks
 *
 * @param task_handle task handle
 * @return int =0 success, others failure*/
int luat_rtos_task_resume(luat_rtos_task_handle task_handle);

/**
 * @brief suspend all tasks
 **/
void luat_rtos_task_suspend_all(void);

/**
 * @brief restore all tasks
 **/
void luat_rtos_task_resume_all(void);

/**
 * @brief task sleeps for a period of time
 *
 * @param ms sleep time, unit ms*/
void luat_rtos_task_sleep(uint32_t ms);

/**
 * @brief Get the handle of the current task
 *
 * @return luat_rtos_task_handle handle of the current task*/
luat_rtos_task_handle luat_rtos_get_current_handle(void);

/**
 * @brief Get the remaining minimum value of the task stack, called the "high water mark"
 *
 * @param luat_rtos_task_handle task handle
 * @return the remaining minimum value of the task stack, in words*/

uint32_t luat_rtos_task_get_high_water_mark(luat_rtos_task_handle task_handle);

/** @}*/
/* ------------------------------------------------ task   end------------------------------------------------ */
/**
 * @defgroup luatos_os_event message event function
 * @{*/
/* ----------------------------------------------- event begin---------------------------------------------- */
/**
 * @brief When waiting for an event, if the target event id is set and the arrival is not the target event id, it can be handed over to the user through the callback function.
 **/
typedef LUAT_RT_RET_TYPE (*luat_rtos_event_wait_callback_t)(LUAT_RT_CB_PARAM);

/**
 * @brief Send an event to the task's mailbox. Only tasks with mailbox enabled can receive it. If more than 1024 events are cached, an assertion will be made.
 *
 * @param task_handle The task handle that needs to receive the event
 * @param id event id
 * @param param1 event parameter 1
 * @param param2 event parameter 2
 * @param param3 event parameter 3
 * @param timeout sending timeout, has been abandoned
 * @return int =0 success, others failure*/
int luat_rtos_event_send(luat_rtos_task_handle task_handle, uint32_t id, uint32_t param1, uint32_t param2, uint32_t param3, uint32_t timeout);

/**
 * @brief receives an event, which can only be received in a task
 *
 * @param task_handle The task handle that needs to receive the event
 * @param wait_event_id ID of the target event, =0 means no limit, any event ID will be returned
 * @param out_event[OUT] received event
 * @param callback_fun When the ID of the event is not the target ID, the user callback function can be NULL to discard the event.
 * @param timeout receive timeout, unit ms, see LUAT_RTOS_WAIT_E for special values
 * @return int =0 success, others failure*/
int luat_rtos_event_recv(luat_rtos_task_handle task_handle, uint32_t wait_event_id, luat_event_t *out_event, luat_rtos_event_wait_callback_t *callback_fun, uint32_t timeout);

/* ----------------------------------------------- event end---------------------------------------------- */

/* ----------------------------------------------- message begin---------------------------------------------- */
/**
 * @brief Send a message to the task's mailbox. Only tasks with mailbox enabled can receive it. The message can be created dynamically and can be any size. If more than 1024 messages are cached, an assertion will be made.
 *
 * @param task_handle The task handle that needs to receive massage
 * @param message_id message id
 * @param p_message message content, passed in pointer, if created dynamically, it needs to be released when receiving
 * @return int =0 success, others failure*/
int luat_rtos_message_send(luat_rtos_task_handle task_handle, uint32_t message_id, void *p_message);

/**
 * @brief receives a message, which can only be received in a task
 *
 * @param task_handle The task handle that needs to receive massage
 * @param message_id[OUT] received message id
 * @param p_p_message[OUT] message content, output a void * pointer, if it is dynamically created when sending, it needs to be released
 * @param timeout receive timeout, unit ms, see LUAT_RTOS_WAIT_E for special values
 * @return int =0 success, others failure*/
int luat_rtos_message_recv(luat_rtos_task_handle task_handle, uint32_t *message_id, void **p_p_message, uint32_t timeout);
/** @}*/

/* ----------------------------------------------- message   end---------------------------------------------- */

/**
 * @defgroup luatos_os_semaphore semaphore interface function
 * @{*/

/* ---------------------------------------------- semaphore begin--------------------------------------------- */
/**
 * @brief defines the semaphore handle*/
typedef void * luat_rtos_semaphore_t;
/**
 * @brief The semaphore is created and can be released in the interrupt
 *
 * @param semaphore_handle[OUT] semaphore handle
 * @param init_count initial value
 * @return int =0 success, others failure*/
int luat_rtos_semaphore_create(luat_rtos_semaphore_t *semaphore_handle, uint32_t init_count);

/**
 * @brief delete semaphore
 *
 * @param semaphore_handle semaphore handle
 * @return int =0 success, others failure*/
int luat_rtos_semaphore_delete(luat_rtos_semaphore_t semaphore_handle);

/**
 * @brief The semaphore is waiting to be obtained
 *
 * @param semaphore_handle semaphore handle
 * @param timeout receive timeout, unit ms, see LUAT_RTOS_WAIT_E for special values
 * @return int =0 success, others failure*/
int luat_rtos_semaphore_take(luat_rtos_semaphore_t semaphore_handle, uint32_t timeout);

/**
 * @brief semaphore release and send
 *
 * @param semaphore_handle semaphore handle
 * @return int =0 success, others failure*/
int luat_rtos_semaphore_release(luat_rtos_semaphore_t semaphore_handle);
/* ---------------------------------------------- semaphore   end--------------------------------------------- */
/** @}*/

/**
 * @defgroup luatos_os_mutex mutex interface function
 * @{*/
/* ------------------------------------------------ mutex begin----------------------------------------------- */
/**
 * @brief defines mutex handle*/
typedef void * luat_rtos_mutex_t;
/**
 * @brief Mutex lock creation, cannot be unlocked during interrupt
 *
 * @param mutex_handle[OUT] Mutex lock handle
 * @return int =0 success, others failure*/
int luat_rtos_mutex_create(luat_rtos_mutex_t *mutex_handle);

/**
 * @brief Get the lock
 *
 * @param mutex_handle mutex lock handle
 * @param timeout timeout, unit ms, see LUAT_RTOS_WAIT_E for special values
 * @return int =0 success, others failure*/
int luat_rtos_mutex_lock(luat_rtos_mutex_t mutex_handle, uint32_t timeout);

/**
 * @brief release lock
 *
 * @param mutex_handle mutex lock handle
 * @return int =0 success, others failure*/
int luat_rtos_mutex_unlock(luat_rtos_mutex_t mutex_handle);

/**
 * @brief delete mutex lock
 *
 * @param mutex_handle mutex lock handle
 * @return int =0 success, others failure*/
int luat_rtos_mutex_delete(luat_rtos_mutex_t mutex_handle);

/* ------------------------------------------------ mutex   end----------------------------------------------- */
/** @}*/

/**
 * @defgroup luatos_os_queue queue interface function
 * @{*/
/* ------------------------------------------------ queue begin----------------------------------------------- */

/**
 * @brief defines the queue handle*/
typedef void * luat_rtos_queue_t;
/**
 * @brief Create queue
 *
 * @param queue_handle[OUT] The returned queue handle
 * @param msgcount The maximum number of elements in the queue
 * @param msgsize The size of a single element in the queue
 * @return int =0 success, others failure*/
int luat_rtos_queue_create(luat_rtos_queue_t *queue_handle, uint32_t item_count, uint32_t item_size);

/**
 * @brief delete queue
 *
 * @param queue_handle queue handle
 * @return int =0 success, others failure*/
int luat_rtos_queue_delete(luat_rtos_queue_t queue_handle);

/**
 * @brief Send an element to the queue
 *
 * @param queue_handle queue handle
 * @param item element pointer
 * @param item_size element size. This is a compatibility parameter. It must actually be consistent with the item_size when created, so it is ignored.
 * @param timeout timeout, unit ms, see LUAT_RTOS_WAIT_E for special values
 * @return int =0 success, others failure*/
int luat_rtos_queue_send(luat_rtos_queue_t queue_handle, void *item, uint32_t item_size, uint32_t timeout);

/**
 * @brief Take an element from the queue
 *
 * @param queue_handle queue handle
 * @param item element pointer
 * @param item_size element size. This is a compatibility parameter. It must actually be consistent with the item_size when created, so it is ignored.
 * @param timeout timeout, unit ms, see LUAT_RTOS_WAIT_E for special values
 * @return int =0 success, others failure*/
int luat_rtos_queue_recv(luat_rtos_queue_t queue_handle, void *item, uint32_t item_size, uint32_t timeout);

/**
 * @brief Query the number of unprocessed elements remaining in the queue
 *
 * @param queue_handle queue handle
 * @param item_cnt[OUT] returns the number of unprocessed elements
 * @return int =0 success, others failure*/
int luat_rtos_queue_get_cnt(luat_rtos_queue_t queue_handle, uint32_t *item_cnt);
/* ------------------------------------------------ queue   end----------------------------------------------- */
/** @}*/

/**
 * @defgroup luatos_os_flag event interface function
 * @{*/
/* ------------------------------------------------ flag begin----------------------------------------------- */

/**
 * @brief defines event handler*/
typedef void * luat_rtos_flag_t;
/**
 * @brief Create event
 *
 * @param flag_handle[OUT] The event handle returned
 * @return int =0 success, others failure*/
int luat_rtos_flag_create(luat_rtos_flag_t	*flag_handle);
/**
 * @brief Waiting for events
 * @param mask Waiting event mask
 * @param operation event triggering requirements (all operations need to be met before triggering, or only one operation triggers) and operation (whether to clear)
 * @param flags[OUT] Current event status value
 * @param timeout timeout time
 * @return int =0 success, others failure*/
int luat_rtos_flag_wait(luat_rtos_flag_t flag_handle, uint32_t mask, LUAT_FLAG_OP_E	operation, uint32_t *flags,uint32_t timeout);
/**
 * @brief set event
 * @param flag_handle event handle
 * @param mask set mask
 * @param operation event judgment (and or) and operation (whether to clear), freertos supports or operation LUAT_FLAG_OR
 * @return int =0 success, others failure*/
int luat_rtos_flag_release(luat_rtos_flag_t	flag_handle, uint32_t mask, LUAT_FLAG_OP_E operation);
/**
 * @brief delete event
 *
 * @param flag_handle event handle
 * @return int =0 success, others failure*/
int luat_rtos_flag_delete(luat_rtos_flag_t flag_handle);
/* ------------------------------------------------ flag  end----------------------------------------------- */
/** @}*/

/**
 * @defgroup luatos_os_timer software timer interface function
 * @{*/

/* ------------------------------------------------ timer begin----------------------------------------------- */
/**
 * @brief timer header data type*/
typedef void * luat_rtos_timer_t;
/**
 * @brief defines timer processing function*/
typedef LUAT_RT_RET_TYPE (*luat_rtos_timer_callback_t)(LUAT_RT_CB_PARAM);
/**
 * @brief Create software timer
 *
 * @param timer_handle[OUT] returns the timer handle
 * @return int =0 success, others failure*/
int luat_rtos_timer_create(luat_rtos_timer_t *timer_handle);

/**
 * @brief Delete software timer
 *
 * @param timer_handle timer handle
 * @return int =0 success, others failure*/
int luat_rtos_timer_delete(luat_rtos_timer_t timer_handle);

/**
 * @brief Start software timer
 *
 * @param timer_handle timer handle
 * @param timeout timeout time, unit ms, no special value
 * @param repeat 0 does not repeat, others repeat
 * @param callback_fun callback function after the scheduled time expires
 * @param user_param The last input parameter when calling back the function
 * @return int =0 success, others failure*/
int luat_rtos_timer_start(luat_rtos_timer_t timer_handle, uint32_t timeout, uint8_t repeat, luat_rtos_timer_callback_t callback_fun, void *user_param);

/**
 * @brief Stop software timer
 *
 * @param timer_handle timer handle
 * @return int =0 success, others failure*/
int luat_rtos_timer_stop(luat_rtos_timer_t timer_handle);

/**
 * @brief Check whether the software timer is activated
 *
 * @param timer_handle timer handle
 * @return int =0 is not activated, 1 is activated, others failed*/
int luat_rtos_timer_is_active(luat_rtos_timer_t timer_handle);

/*------------------------------------------------ timer   end----------------------------------------------- */
/** @}*/

/**
 * @defgroup luatos_os_critical critical protection interface function
 * @{*/

/* ------------------------------------------------ critical begin----------------------------------------------- */
/**
 * @brief Enter critical protection
 *
 * @return uint32_t Parameters required to exit critical protection*/
uint32_t luat_rtos_entry_critical(void);

/**
 * @brief Exit critical protection
 *
 * @param critical parameter returned when entering critical protection*/
void luat_rtos_exit_critical(uint32_t critical);
/*------------------------------------------------ critical   end----------------------------------------------- */
/** @}*/
/** @}*/
#endif
