#ifndef __COMMON_API_H__
#define __COMMON_API_H__
#include "stdint.h"
#include "stdarg.h"
#include "string.h"
#include "stdio.h"
#include "stdbool.h"
#include "bsp_common.h"
#include "platform_define.h"

#ifdef LOW_SPEED_SERVICE_ONLY
#define PS_DIAL_PS_UP_MEM_SIZE      248000
#else   //Full speed version
#define PS_DIAL_PS_UP_MEM_SIZE      421000
#endif

enum
{
	I2S_MODE_I2S,
	I2S_MODE_LSB,
	I2S_MODE_MSB,
	I2S_FRAME_SIZE_16_16,
	I2S_FRAME_SIZE_16_32,
	I2S_FRAME_SIZE_24_32,
	I2S_FRAME_SIZE_32_32,
};

typedef void (* usb_serial_in)(uint8_t channel, uint8_t *input, uint32_t len);
typedef void (*pad_wakeup_fun_t)(uint32_t pad_num);
/**
 * @brief Formatted print output log, which is customer luatos in the EPAT tool
 * @param fmt printing format
 * @param ... parameters*/
void soc_printf(const char *fmt, ...);

/**
 * @brief Directly output string, which is customer luatos in the EPAT tool
 *
 * @param string The string to be output
 * @param size string length, can be selected as 0*/
void soc_debug_out(char *string, uint32_t size);

/**
 * @brief Set the callback function for USB serial port input
 *
 * @param cb_fun callback function*/
void soc_set_usb_serial_input_callback(usb_serial_in cb_fun);
#define set_usb_serial_input_callback(x)	soc_set_usb_serial_input_callback(x)

/**
 * @brief USB serial port output
 *
 * @param channel USB serial port channel, currently the default is 4, or consistent with the input channel * @param output Output data
 * @param len the output data length
 * @return int 0 success, others failure*/
int soc_usb_serial_output(uint8_t channel, uint8_t *output, uint32_t len);
#define usb_serial_output(x,y,z)	soc_usb_serial_output(x,y,z)
/**
 * @brief Sets the wakeuppad to wake up the interrupt callback in low power mode. Note that this is the interrupt callback
 *
 * @param cb_fun*/
void soc_set_pad_wakeup_callback(pad_wakeup_fun_t cb_fun);
#define set_pad_wakeup_callback(x)	soc_set_pad_wakeup_callback(x)
/**
 * @brief Create a task with an event sending and receiving mechanism. The event is a 16byte item.
 *
 * @param task_fun task entry function
 * @param param entry parameter of task
 * @param stack_bytes The stack length of the task, in bytes, will force 4-byte alignment
 * @param priority task priority, note that it is a percentage, 0~100, 100 is the highest level allowed by the underlying OS, 0 is the lowest level allowed by the underlying OS
 * @param task_name task name
 * @param event_max_cnt, if the OS allows malloc in interrupts, this parameter is invalid. If the OS does not allow malloc in interrupts, fill in the pre-allocated event space here for use in interrupts. If you write 0, the public event will be used.
 * @return void* The handle of the task. This parameter is required for subsequent sending and receiving events. NULL means the creation failed.*/
void *create_event_task(TaskFun_t task_fun, void *param, uint32_t stack_bytes, uint8_t priority, uint16_t event_max_cnt, const char *task_name);

/**
 * @brief Deleting a task with an event sending and receiving mechanism requires one more step than deleting the task normally to delete the event mailbox.
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
 * @param timeout_ms This has been deprecated
 * @return int Returns 0 successfully, otherwise it will fail. Now, failure will be returned only when the pre-allocated events in the interrupt are used up.*/
int send_event_to_task(void *task_handle, OS_EVENT *event, uint32_t event_id, uint32_t param1, uint32_t param2, uint32_t param3, uint32_t timeout_ms);

/**
 * @brief Get an event and return it as needed. If more than 1024 events are cached, it will assert
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

/**
 * @brief Get the number of unprocessed events in the task
 *
 * @param task_handle task handle
 * @return uint32_t The number of unprocessed events in the task*/
uint32_t get_event_cnt(void *task_handle);


/**
 * @brief Get the ms time from boot to now
 *
 * @return uint64_t*/
uint64_t soc_get_poweron_time_ms(void);

uint64_t soc_get_poweron_time_tick(void);
/**
 * @brief Get heap information
 *
 * @param total total amount
 * @param total_free remaining amount
 * @param min_free The historical minimum running amount, which also corresponds to the historical maximum usage*/
void soc_get_heap_info(uint32_t *total, uint32_t *total_free, uint32_t *min_free);
/**
 * @brief Request to change the global low power state
 *
 * @param state 0 full speed 1IDLE 2SLEEP*/
void soc_require_lowpower_state(uint8_t state);

/**
 * @brief User manually controls USB switch
 *
 * @param onoff 1 is on, 0 is off*/
void soc_usb_onoff(uint8_t onoff);

uint8_t soc_usb_stack_onoff_state(void);
uint32_t soc_get_utc(void);
uint64_t soc_get_utc_ms(void);
int soc_get_sn(char *sn, uint8_t buf_size);
/**
 * @brief Freeing dynamically allocated ram in an interrupt is essentially freeing it in a special task.
 *
 * @param point pointer obtained by malloc
 * @return 0 success, others failure*/
int soc_free_later(void *point);
/** The function is run in the system service, with a small stack and high priority.*/
int soc_call_function_in_service(CBDataFun_t CB, uint32_t data, uint32_t param, uint32_t timeout);
/** The function is run in the audio service. The stack is large and the priority is low. Audio must be initialized first.*/
int soc_call_function_in_audio(CBDataFun_t CB, uint32_t data, uint32_t param, uint32_t timeout);
/**
 * @brief Formatted printing with function name and position
 **/
#define DBG(X,Y...) soc_printf("%s %d:"X, __FUNCTION__,__LINE__,##Y)

#endif
