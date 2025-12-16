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

#ifndef LUAT_RTOS_LEGACY_H
#define LUAT_RTOS_LEGACY_H

#include "luat_base.h"
/**
 * @brief event parameters*/
typedef struct
{
	uint32_t id;
	uint32_t param1;
	uint32_t param2;
	uint32_t param3;
}luat_event_t;
//The timer callback function needs to be defined as LUAT_RT_RET_TYPE fun_name(LUAT_RT_CB_PARAM)
//Required when the timer callback function exits, return LUAT_RT_RET;
#ifndef LUAT_RT_RET_TYPE
#define LUAT_RT_RET_TYPE	void
#endif

#ifndef LUAT_RT_RET
#define LUAT_RT_RET
#endif

#ifndef LUAT_RT_CB_PARAM
#define LUAT_RT_CB_PARAM	void
//#define LUAT_RT_CB_PARAM	void *param
//#define LUAT_RT_CB_PARAM void *pdata, void *param
#endif
/* ----------------------------------- thread ----------------------------------- */
LUAT_RET luat_send_event_to_task(void *task_handle, uint32_t id, uint32_t param1, uint32_t param2, uint32_t param3);
LUAT_RET luat_wait_event_from_task(void *task_handle, uint32_t wait_event_id, luat_event_t *out_event, void *call_back, uint32_t ms);
void *luat_get_current_task(void);


/*----------------------------------The semaphore simulates a mutex lock and can be unlocked during an interrupt--- ----------------------------*/
void *luat_mutex_create(void);
LUAT_RET luat_mutex_lock(void *mutex);
LUAT_RET luat_mutex_unlock(void *mutex);
void luat_mutex_release(void *mutex);


/* ----------------------------------- timer ----------------------------------- */
/**
 * @brief Create software timer
 * @param cb timing processing function
 * @param param parameter
 * @param task_handle[OUT] returns the timer handle
 * @return int =0 success, others failure*/
void *luat_create_rtos_timer(void *cb, void *param, void *task_handle);
/**
 * @brief Start software timer
 * @param timer timer handle
 * @param ms timing time
 * @param is_repeat whether it is periodic timing*/

int luat_start_rtos_timer(void *timer, uint32_t ms, uint8_t is_repeat);
/**
 * @brief Stop software timer
 * @param timer timer handle*/
void luat_stop_rtos_timer(void *timer);
/**
 * @brief reset software timer
 * @param timer timer handle*/
void luat_release_rtos_timer(void *timer);
/**
 * @brief suspend all tasks
 **/
void luat_task_suspend_all(void);
/**
 * @brief restore all tasks
 **/
void luat_task_resume_all(void);
#endif
