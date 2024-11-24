#ifndef LUAT_RTOS_LEGACY_H
#define LUAT_RTOS_LEGACY_H

#include "luat_base.h"
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
void *luat_create_rtos_timer(void *cb, void *param, void *task_handle);
int luat_start_rtos_timer(void *timer, uint32_t ms, uint8_t is_repeat);
int luat_start_rtos_timer_us(void *timer, uint32_t us);
void luat_stop_rtos_timer(void *timer);
void luat_release_rtos_timer(void *timer);

void luat_task_suspend_all(void);
void luat_task_resume_all(void);
#endif
