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

#include "common_api.h"
#include "luat_base.h"
#include "luat_mem.h"
#include "luat_pm.h"
#include "luat_wdt.h"
#include "FreeRTOS.h"
#include "apmu_external.h"
#include "slpman.h"
#include "reset.h"
#include "pwrkey.h"
#include "luat_gpio.h"
#include "luat_gpio_legacy.h"
#include "platform_define.h"
#include "plat_config.h"
#include "driver_gpio.h"
extern void soc_usb_onoff(uint8_t onoff);
extern void soc_set_usb_sleep(uint8_t onoff);
extern int soc_power_mode(uint8_t main, uint8_t sub);
static uint32_t reportMode[LUAT_PM_SLEEP_MODE_STANDBY + 1][10] = {0};

int luat_pm_set_sleep_mode(int mode, const char *vote_tag)
{
    if (mode < LUAT_PM_SLEEP_MODE_NONE || mode > LUAT_PM_SLEEP_MODE_STANDBY || vote_tag == NULL)
    {
        return -1;
    }

    bool findFirst = true;
    for (int sleepMode = LUAT_PM_SLEEP_MODE_NONE; sleepMode < LUAT_PM_SLEEP_MODE_STANDBY + 1; sleepMode++)
    {
        for (int j = 0; j < 9; j++)
        {
            if (strcmp((const char *)reportMode[sleepMode][j], vote_tag) == 0)
            {
                findFirst = false;
                if (mode == sleepMode)
                {
                    break;
                }
                else
                {
                    luat_heap_free((uint32_t *)reportMode[sleepMode][j]);
                    reportMode[sleepMode][j] = (uint32_t)NULL;
                    reportMode[sleepMode][9] = reportMode[sleepMode][9] - 1;
                    if (reportMode[mode][9] < 9)
                    {
                        reportMode[mode][j] = (uint32_t)luat_heap_malloc(strlen(vote_tag) + 1);
                        memset((uint32_t *)reportMode[mode][j], 0x00, strlen(vote_tag) + 1);
                        memcpy((uint32_t *)reportMode[mode][j], vote_tag, strlen(vote_tag) + 1);
                        reportMode[mode][9] = reportMode[mode][9] + 1;
                    }
                    break;
                }
            }
        }
    }
    if (findFirst)
    {
        int count = 0;
        for (int sleepMode = LUAT_PM_SLEEP_MODE_NONE; sleepMode < LUAT_PM_SLEEP_MODE_STANDBY + 1; sleepMode++)
        {
            count += reportMode[sleepMode][9];
        }

        if (count >= 8)
            return -1;

        if (reportMode[mode][9] < 9)
        {
            reportMode[mode][reportMode[mode][9]] = (uint32_t)luat_heap_malloc(strlen(vote_tag) + 1);
            memset((uint32_t *)reportMode[mode][reportMode[mode][9]], 0x00, strlen(vote_tag) + 1);
            memcpy((uint32_t *)reportMode[mode][reportMode[mode][9]], vote_tag, strlen(vote_tag) + 1);
            reportMode[mode][9] = reportMode[mode][9] + 1;
        }
    }
    for (int sleepMode = LUAT_PM_SLEEP_MODE_NONE; sleepMode < LUAT_PM_SLEEP_MODE_STANDBY + 1; sleepMode++)
    {
        for (int j = 0; j < 9; j++)
        {
            if (reportMode[sleepMode][j] != (uint32_t)NULL)
            {
                switch (sleepMode)
                {
                case LUAT_PM_SLEEP_MODE_NONE:
                    apmuSetDeepestSleepMode(AP_STATE_ACTIVE);
                    break;
                case LUAT_PM_SLEEP_MODE_IDLE:
                    apmuSetDeepestSleepMode(AP_STATE_IDLE);
                    break;
                case LUAT_PM_SLEEP_MODE_LIGHT:
                    apmuSetDeepestSleepMode(AP_STATE_SLEEP1);
                    break;
                case LUAT_PM_SLEEP_MODE_DEEP:
                    apmuSetDeepestSleepMode(AP_STATE_SLEEP2);
                    break;
                case LUAT_PM_SLEEP_MODE_STANDBY:
                    apmuSetDeepestSleepMode(AP_STATE_HIBERNATE);
                    break;
                default:
                    apmuSetDeepestSleepMode(AP_STATE_IDLE);
                    break;
                }
                return 0;
            }
        }
    }
}

int luat_pm_get_sleep_mode(const char *vote_tag)
{
    for (int sleepMode = LUAT_PM_SLEEP_MODE_NONE; sleepMode < LUAT_PM_SLEEP_MODE_STANDBY + 1; sleepMode++)
    {
        for (int j = 0; j < 9; j++)
        {
            if (strcmp((const char *)reportMode[sleepMode][j], vote_tag) == 0)
            {
                return sleepMode;
            }
        }
    }
    return -1;
}

static luat_pm_sleep_callback_t g_s_user_pre_callback = NULL;
static luat_pm_sleep_callback_t g_s_user_post_callback = NULL;

static void sleep_pre_callback(void *param, slpManLpState mode)
{
    if (g_s_user_pre_callback != NULL)
    {
        g_s_user_pre_callback(mode);
    }
}

static void sleep_post_callback(void *param, slpManLpState mode)
{
    if (g_s_user_post_callback != NULL)
    {
        g_s_user_post_callback(mode);
    }
}

int luat_pm_sleep_register_pre_handler(luat_pm_sleep_callback_t callback_fun)
{
    g_s_user_pre_callback = callback_fun;
    return slpManRegisterUsrdefinedBackupCb(sleep_pre_callback, NULL);
}
int luat_pm_sleep_deregister_pre_handler(void)
{
    g_s_user_pre_callback = NULL;
    return slpManUnregisterUsrdefinedBackupCb(sleep_pre_callback);
}
int luat_pm_sleep_register_post_handler(luat_pm_sleep_callback_t callback_fun)
{
    g_s_user_post_callback = callback_fun;
    return slpManRegisterUsrdefinedRestoreCb(sleep_post_callback, NULL);
}
int luat_pm_sleep_deregister_post_handler(void)
{
    g_s_user_post_callback = NULL;
    return slpManUnregisterUsrdefinedRestoreCb(sleep_post_callback);
}

int luat_pm_wakeup_pad_set_callback(luat_pm_wakeup_pad_isr_callback_t callback_fun)
{
    // soc_set_pad_wakeup_callback(*((pad_wakeup_fun_t*)&callback_fun));
    soc_set_pad_wakeup_callback((pad_wakeup_fun_t)callback_fun);
    return 0;
}

int luat_pm_wakeup_pad_set(uint8_t enable, LUAT_PM_WAKEUP_PAD_E source_id, luat_pm_wakeup_pad_cfg_t *cfg)
{
#if 0
    if ((enable != 1 && enable != 0) || ((source_id < LUAT_PM_WAKEUP_PAD_0) && (source_id > LUAT_PM_WAKEUP_PAD_5)) || cfg == NULL)
    {
        return -1;
    }
    int pin;
    switch (source_id)
    {
    case LUAT_PM_WAKEUP_PAD_0:
        pin = HAL_WAKEUP_0;
        break;
    case LUAT_PM_WAKEUP_PAD_1:
        pin = HAL_WAKEUP_1;
        break;
    case LUAT_PM_WAKEUP_PAD_2:
        pin = HAL_WAKEUP_2;
        break;
    case LUAT_PM_WAKEUP_PAD_3:
        pin = HAL_GPIO_20;
        break;
    case LUAT_PM_WAKEUP_PAD_4:
        pin = HAL_GPIO_21;
        break;
    case LUAT_PM_WAKEUP_PAD_5:
        pin = HAL_GPIO_22;
        break;
    default:
        return -1;
        break;
    }

    if (1 == enable)
    {
        luat_gpio_cfg_t gpio_cfg = {0};
        gpio_cfg.irq_cb = NULL;
        gpio_cfg.irq_type = NULL;
        gpio_cfg.pin = pin;
        gpio_cfg.mode = LUAT_GPIO_IRQ;
        if (!cfg->pull_up_enable && !cfg->pull_down_enable)
        {
            gpio_cfg.pull = LUAT_GPIO_DEFAULT;
        }
        else if(cfg->pull_up_enable && !cfg->pull_down_enable)
        {
            gpio_cfg.pull = LUAT_GPIO_PULLUP;
        }
        else if(!cfg->pull_down_enable && cfg->pull_down_enable)
        {
            gpio_cfg.pull = Luat_GPIO_PULLDOWN;
        }
        else
        {
            return -1;
        }

        if(cfg->pos_edge_enable && cfg->neg_edge_enable)
        {
            gpio_cfg.irq_type = LUAT_GPIO_BOTH_IRQ;
        }
        else if(cfg->pos_edge_enable && !cfg->neg_edge_enable)
        {
            gpio_cfg.irq_type = LUAT_GPIO_RISING_IRQ;
        }
        else if(!cfg->pos_edge_enable && cfg->neg_edge_enable)
        {
            gpio_cfg.irq_type = LUAT_GPIO_FALLING_IRQ;
        }
        else
        {
            return -1;
        }
        luat_gpio_open(&gpio_cfg);
    }
    else
    {
        luat_gpio_close(pin);
    }
#else
    GPIO_GlobalInit(NULL);
    if (enable)
    {
        GPIO_WakeupPadConfig(HAL_WAKEUP_0 + source_id, cfg->pos_edge_enable, cfg->neg_edge_enable, cfg->pull_up_enable, cfg->pull_down_enable);
    }
    else
    {
        GPIO_WakeupPadConfig(HAL_WAKEUP_0 + source_id, 0, 0, 0, 0);
    }
#endif
    return 0;
}

int luat_pm_wakeup_pad_get_value(LUAT_PM_WAKEUP_PAD_E source_id)
{
#if 0
    if ((source_id < LUAT_PM_WAKEUP_PAD_0) && (source_id > LUAT_PM_WAKEUP_PAD_5))
    {
        return -1;
    }
    int pin;
    switch (source_id)
        {
        case LUAT_PM_WAKEUP_PAD_0:
            pin = HAL_WAKEUP_0;
            break;
        case LUAT_PM_WAKEUP_PAD_1:
            pin = HAL_WAKEUP_1;
            break;
        case LUAT_PM_WAKEUP_PAD_2:
            pin = HAL_WAKEUP_2;
            break;
        case LUAT_PM_WAKEUP_PAD_3:
            pin = HAL_GPIO_20;
            break;
        case LUAT_PM_WAKEUP_PAD_4:
            pin = HAL_GPIO_21;
            break;
        case LUAT_PM_WAKEUP_PAD_5:
            pin = HAL_GPIO_22;
            break;
        default:
            return -1;
            break;
        }
    return luat_gpio_get(pin);
#else
    return (slpManGetWakeupPinValue() & (1 << source_id))?1:0;
#endif
}

int luat_pm_set_pwrkey(LUAT_PM_POWERKEY_MODE_E mode, bool pullUpEn, luat_pm_pwrkey_cfg_t *cfg, luat_pm_pwrkey_callback_t callback)
{
    pwrKeyDly_t dlyCfg = {0};
    dlyCfg.longPressTimeout = cfg->long_press_timeout;
    dlyCfg.repeatTimeout = cfg->repeat_timeout;

    pwrKeyInit(mode, pullUpEn, dlyCfg, (pwrKeyCallback_t) callback);
    return 0;
}

int luat_pm_get_poweron_reason(void)
{
    LastResetState_e apRstState,cpRstState;
    ResetStateGet(&apRstState, &cpRstState);
    DBG("ap %d,cp %d", apRstState, cpRstState);
    int id = 0;

    switch(cpRstState)
    {
    case LAST_RESET_HARDFAULT:
    case LAST_RESET_ASSERT:
        return LUAT_PM_POWERON_REASON_EXCEPTION;
        break;
    case LAST_RESET_WDTSW:
    case LAST_RESET_WDTHW:
    case LAST_RESET_LOCKUP:
    case LAST_RESET_AONWDT:
        return LUAT_PM_POWERON_REASON_WDT;
        break;
    default:
        return LUAT_PM_POWERON_REASON_UNKNOWN;
        break;
    }

    switch(apRstState)
    {
    case LAST_RESET_POR:
    case LAST_RESET_NORMAL:
        id = LUAT_PM_POWERON_REASON_NORMAL;
        break;
    case LAST_RESET_SWRESET:
        id = LUAT_PM_POWERON_REASON_SWRESET;
        break;
    case LAST_RESET_HARDFAULT:
    case LAST_RESET_ASSERT:
        id = LUAT_PM_POWERON_REASON_EXCEPTION;
        break;
    case LAST_RESET_WDTSW:
    case LAST_RESET_WDTHW:
    case LAST_RESET_LOCKUP:
    case LAST_RESET_AONWDT:
        id = LUAT_PM_POWERON_REASON_WDT;
        break;
    case LAST_RESET_BATLOW:
    case LAST_RESET_TEMPHI:
        id = LUAT_PM_POWERON_REASON_EXTERNAL;
        break;
    case LAST_RESET_FOTA:
        id = LUAT_PM_POWERON_REASON_FOTA;
        break;
    case LAST_RESET_CPRESET:
        id = 100 + cpRstState;
        break;
    default:
        id = 200 + cpRstState;
        break;
    }
    return id;
}

int luat_pm_poweroff(void)
{
    luat_wdt_close();
    slpManStartPowerOff();
    return 0;
}

int luat_pm_reboot(void)
{
    ResetECSystemReset();
    return 0;
}

int luat_pm_reset(void)
{
    ResetECSystemReset();
    return 0;
}


int luat_pm_get_vbus_status(uint8_t *status)
{
    *status = usb_portmon_vbuspad_level();
    return 0;
}


int luat_pm_set_usb_power(uint8_t onoff)
{
    soc_set_usb_sleep(!onoff);
    soc_usb_onoff(onoff);
}

int luat_pm_deep_sleep_mode_timer_start(LUAT_PM_DEEPSLEEP_TIMERID_E timer_id, uint32_t timeout)
{
    if (/*timer_id < LUAT_PM_DEEPSLEEP_TIMER_ID0 || */ timer_id > LUAT_PM_DEEPSLEEP_TIMER_ID6 || timeout <= 0)
        return -1;
    if ((timer_id == LUAT_PM_DEEPSLEEP_TIMER_ID0 || timer_id == LUAT_PM_DEEPSLEEP_TIMER_ID1) && (timeout > 9000000))
        timeout = 9000000;
    if ((timer_id >= LUAT_PM_DEEPSLEEP_TIMER_ID2 && timer_id <= LUAT_PM_DEEPSLEEP_TIMER_ID6) && (timeout > 2664000000))
        timeout = 2664000000;
    slpManDeepSlpTimerStart(timer_id, timeout);
    return 0;
}

int luat_pm_deep_sleep_mode_timer_stop(LUAT_PM_DEEPSLEEP_TIMERID_E timer_id)
{
    if (/*timer_id < LUAT_PM_DEEPSLEEP_TIMER_ID0 || */ timer_id > LUAT_PM_DEEPSLEEP_TIMER_ID6)
        return -1;
    slpManDeepSlpTimerDel(timer_id);
    return 0;
}

int luat_pm_deep_sleep_mode_timer_is_running(LUAT_PM_DEEPSLEEP_TIMERID_E timer_id)
{
    if (/*timer_id < LUAT_PM_DEEPSLEEP_TIMER_ID0 || */ timer_id > LUAT_PM_DEEPSLEEP_TIMER_ID6)
        return -1;
    return slpManDeepSlpTimerIsRunning(timer_id) == true ? 1 : 0;
}

int luat_pm_deep_sleep_mode_register_timer_cb(LUAT_PM_DEEPSLEEP_TIMERID_E timer_id, luat_pm_deep_sleep_mode_timer_callback_t callback)
{
    if (/*timer_id < LUAT_PM_DEEPSLEEP_TIMER_ID0 || */timer_id > LUAT_PM_DEEPSLEEP_TIMER_ID6 || callback == NULL)
        return -1;
    slpManDeepSlpTimerRegisterExpCb(timer_id, callback);
    return 0;
}

uint32_t luat_pm_get_deep_sleep_mode_timer_remain_time(LUAT_PM_DEEPSLEEP_TIMERID_E timer_id)
{
    if (/*timer_id < LUAT_PM_DEEPSLEEP_TIMER_ID0 || */ timer_id > LUAT_PM_DEEPSLEEP_TIMER_ID6)
        return 0xffffffff;
    return slpManDeepSlpTimerRemainMs(timer_id);
}

int luat_pm_get_wakeup_reason()
{
    return slpManGetWakeupSrc();
}

int luat_pm_set_gnss_power(uint8_t onoff)
{
    int ret=-1;
    if (1 == onoff)
    {
        luat_gpio_close(HAL_GPIO_13);
        luat_gpio_cfg_t cfg = {0};
        cfg.pin = HAL_GPIO_13;
        cfg.mode = LUAT_GPIO_OUTPUT;
        cfg.output_level = 1;
        cfg.alt_fun = 4;
        ret=luat_gpio_open(&cfg);
        return ret;
    }
    else if (0 == onoff)
    {
        ret=luat_gpio_set(HAL_GPIO_13, 0);
        return ret;
    }
    return ret;
}

int luat_pm_set_power_mode(uint8_t mode, uint8_t sub_mode)
{
    return soc_power_mode(mode, sub_mode);
}

void luat_pm_print_state(void)
{
    extern void slpManSlpFailedReasonCheck(slpManSlpState_t *DeepestSlpMode, slpManSlpState_t *psphyVoteState, slpManSlpState_t *appVoteState,  slpManSlpState_t *usrdefSlpMode,  uint32_t *drvVoteMap, uint32_t *drvVoteMask);
    extern void slpManGetSDKVoteDetail(uint32_t *sleepVoteFlag, uint32_t *sleep2VoteFlag, uint32_t *hibVoteFlag);
    extern void slpManGetCPVoteStatus(bool *cpSleeped, uint8_t *cpVote);

    slpManSlpState_t DeepestSlpMode;
    slpManSlpState_t psphyVoteState;
    slpManSlpState_t appVoteState;
    slpManSlpState_t usrdefSlpMode;
    uint32_t drvVoteMap;
    uint32_t drvVoteMask;
    slpManSlpState_t appVoteDetail;
    uint8_t vote_counter;
    uint32_t sdkSleep1Vote;
    uint32_t sdkSleep2Vote;
    uint32_t sdkHibVote;
    bool cpSleeped;
    uint8_t cpVote;

    slpManSlpFailedReasonCheck(&DeepestSlpMode, &psphyVoteState, &appVoteState, &usrdefSlpMode, &drvVoteMap, &drvVoteMask);
    slpManGetSDKVoteDetail(&sdkSleep1Vote, &sdkSleep2Vote, &sdkHibVote);
    DBG("%d,%d,%d,%d,%x,%x", DeepestSlpMode, psphyVoteState, appVoteState, usrdefSlpMode, drvVoteMap, drvVoteMask);
    DBG("%u,%u,%u", sdkSleep1Vote, sdkSleep2Vote, sdkHibVote);
    for(uint8_t i=0;i<32;i++)
    {
        if(slpManCheckVoteState(i, &appVoteDetail, &vote_counter) == RET_TRUE)
        {
            if(vote_counter != 0)
            {
                DBG("handle %d name %s state %d, cnt %d", i, slpManGetVoteInfo(i), appVoteDetail, vote_counter);
            }
            else
            {
                DBG("handle %d name %s state NULL, cnt %d", i, slpManGetVoteInfo(i), vote_counter);
            }
        }
    }
    slpManGetCPVoteStatus(&cpSleeped, &cpVote);
    DBG("cp %d,%d", cpSleeped, cpVote);
}

int luat_pm_iovolt_ctrl(int id, int val) {
    IOVoltageSel_t set;
    if (val > 3400)
    {
        set = IOVOLT_3_40V;
    }
    else if (val >= 2650)
    {
        set = (val - 2650)/50 + IOVOLT_2_65V;
    }
    else if (val > 2000)
    {
        DBG("iovolt: out of range %d %d", id, val);
        return -1;
    }
    else if (val >= 1650)
    {
        set = (val - 1650)/50;
    }
    else
    {
        set = IOVOLT_1_65V;
    }
    slpManNormalIOVoltSet(set);
    slpManAONIOVoltSet(set);
    return 0;

}


int luat_pm_set_powerkey_mode(uint8_t onoff)
{
    if(onoff != 0 && onoff != 1)
    {
        return -1;
    }
    if(BSP_GetPlatConfigItemValue(PLAT_CONFIG_ITEM_PWRKEY_MODE) != onoff)
    {
        DBG("powerkey mode %d to %d", BSP_GetPlatConfigItemValue(PLAT_CONFIG_ITEM_PWRKEY_MODE), onoff);
        BSP_SetPlatConfigItemValue(PLAT_CONFIG_ITEM_PWRKEY_MODE, onoff);
        BSP_SavePlatConfigToRawFlash();
    }
    return 0;
}
