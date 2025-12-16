#include "FreeRTOS.h"
#include "luat_rtos.h"
#include "common_api.h"
#include "luat_debug.h"
#include "luat_adc.h"
static luat_rtos_task_handle charge_task_handle;

static volatile uint16_t vbat = 0;

uint16_t get_vbat()
{
    return vbat;
}

static void charge_task(void *param)
{
    int origin, revert;
    luat_adc_open(LUAT_ADC_CH_VBAT, NULL);
    luat_adc_read(LUAT_ADC_CH_VBAT, &origin, &revert);
    luat_rtos_task_sleep(1000);
    while (1)
    {
        luat_adc_read(LUAT_ADC_CH_VBAT, &origin, &revert);
        vbat = revert;
        luat_rtos_task_sleep(60000);
    }
    luat_rtos_task_delete(charge_task_handle);
}


void charge_task_init(void)
{
    int result = luat_rtos_task_create(&charge_task_handle, 4096, 20, "charge task", charge_task, NULL, NULL);
    LUAT_DEBUG_PRINT("cloud_speaker_charge_task create task result %d", result);
}
