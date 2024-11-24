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

/** PWM operation
 **/
#include "common_api.h"
#include "FreeRTOS.h"
#include "task.h"
#include "driver_gpio.h"
#include "luat_base.h"
#ifdef __LUATOS__
#include "luat_malloc.h"
#include "luat_msgbus.h"
#include "luat_timer.h"
#endif
#include "luat_pwm.h"
#include "luat_rtos.h"

#include "timer.h"
#include "hal_adc.h"
#include "ic.h"
#include "hal_trim.h"
#include "timer.h"
#include "ec618.h"
#include "clock.h"
#include "pad.h"
#include "luat_debug.h"
#include "RTE_Device.h"
#define EIGEN_TIMER(n)             ((TIMER_TypeDef *) (AP_TIMER0_BASE_ADDR + 0x1000*n))

/*
0 -- GPIO 23 -- PAD 43
1 -- GPIO 24 -- PAD 44
2 -- GPIO 25 -- PAD 45
4 -- GPIO 27 -- PAD 47
*/
/*
10 -- GPIO 01 -- PAD 16  ---> LCD_RST
11 -- GPIO 02 -- PAD 17
12 -- GPIO 16 -- PAD 31
14 -- GPIO 18 -- PAD 33  ---> UART1_RX
*/
/*Group 3, reuse i2s pins
20 -- GPIO 19 -- PAD 39 ---> I2S_MCLK
21 -- GPIO 29 -- PAD 35 ---> I2S_BCLK
22 -- GPIO 30 -- PAD 36 ---> I2S_LCLK
24 -- GPIO 18 -- PAD 38 ---> I2S_DOUT*/
/*Group 4, reuse uart0 pins
30 -- GPIO 14 -- PAD 20
31 -- GPIO 15 -- PAD 30*/
#if 0
static signed char if_initialized_timer(const int channel)
{
   if( 0 != EIGEN_TIMER(channel)->TCCR )
    {

        return -1;
    }

    return 0;
}

#define PWM_CH_MAX (6)


static int g_s_pnum_set[PWM_CH_MAX] = {0}; /*Set the number of PWM pulses*/
static volatile int g_s_pnum_update[PWM_CH_MAX] = {0};/*Current PWM number*/
/*The highest frequency should be 26M*/ 
#define MAX_FREQ (26*1000*1000)
typedef struct pwm_ctx
{
    TimerPwmConfig_t timer_config;
    uint32_t freq;
    uint32_t pulse;
    uint32_t pnum;
}pwm_ctx_t;

typedef struct pwm_map
{
    int pwm_ch; // pwm id displayed externally
    int clockId;
    int clockId_slect;
    int time_req;
    int pad;
}pwm_map_t;

static const pwm_map_t maps[] = {
#ifdef RTE_PWM0
    {.pwm_ch=0, .clockId=FCLK_TIMER0, .clockId_slect=FCLK_TIMER0_SEL_26M, .time_req=PXIC0_TIMER0_IRQn, .pad=RTE_PWM0},
#endif
#ifdef RTE_PWM1
    {.pwm_ch=1, .clockId=FCLK_TIMER1, .clockId_slect=FCLK_TIMER1_SEL_26M, .time_req=PXIC0_TIMER1_IRQn, .pad=RTE_PWM1},
#endif
#ifdef RTE_PWM2
    {.pwm_ch=2, .clockId=FCLK_TIMER2, .clockId_slect=FCLK_TIMER2_SEL_26M, .time_req=PXIC0_TIMER2_IRQn, .pad=RTE_PWM2},
#endif
// #ifdef RTE_PWM3
//     {.pwm_ch=3, .clockId=FCLK_TIMER3, .clockId_slect=FCLK_TIMER3_SEL_26M, .time_req=PXIC0_TIMER3_IRQn, .pad=RTE_PWM3},
// #endif
#ifdef RTE_PWM4
    {.pwm_ch=4, .clockId=FCLK_TIMER4, .clockId_slect=FCLK_TIMER4_SEL_26M, .time_req=PXIC0_TIMER4_IRQn, .pad=RTE_PWM4},
#endif
// #ifdef RTE_PWM5
//     {.pwm_ch=5, .clockId=FCLK_TIMER5, .clockId_slect=FCLK_TIMER5_SEL_26M, .time_req=PXIC0_TIMER5_IRQn, .pad=RTE_PWM5},
// #endif


    {.pwm_ch=10, .clockId=FCLK_TIMER0, .clockId_slect=FCLK_TIMER0_SEL_26M, .time_req=PXIC0_TIMER0_IRQn, .pad=16},
    {.pwm_ch=11, .clockId=FCLK_TIMER1, .clockId_slect=FCLK_TIMER1_SEL_26M, .time_req=PXIC0_TIMER1_IRQn, .pad=17},
    {.pwm_ch=12, .clockId=FCLK_TIMER2, .clockId_slect=FCLK_TIMER2_SEL_26M, .time_req=PXIC0_TIMER2_IRQn, .pad=31},
    // {.pwm_ch=13, .clockId=FCLK_TIMER3, .clockId_slect=FCLK_TIMER3_SEL_26M, .time_req=PXIC0_TIMER3_IRQn, .pad=32},
    {.pwm_ch=14, .clockId=FCLK_TIMER4, .clockId_slect=FCLK_TIMER4_SEL_26M, .time_req=PXIC0_TIMER4_IRQn, .pad=33},
    // {.pwm_ch=15, .clockId=FCLK_TIMER5, .clockId_slect=FCLK_TIMER5_SEL_26M, .time_req=PXIC0_TIMER5_IRQn, .pad=34},



    {.pwm_ch=20, .clockId=FCLK_TIMER0, .clockId_slect=FCLK_TIMER0_SEL_26M, .time_req=PXIC0_TIMER0_IRQn, .pad=39},
    {.pwm_ch=21, .clockId=FCLK_TIMER1, .clockId_slect=FCLK_TIMER1_SEL_26M, .time_req=PXIC0_TIMER1_IRQn, .pad=35},
    {.pwm_ch=22, .clockId=FCLK_TIMER2, .clockId_slect=FCLK_TIMER2_SEL_26M, .time_req=PXIC0_TIMER2_IRQn, .pad=36},
    // {.pwm_ch=23, .clockId=FCLK_TIMER3, .clockId_slect=FCLK_TIMER3_SEL_26M, .time_req=PXIC0_TIMER3_IRQn, .pad=37},
    {.pwm_ch=24, .clockId=FCLK_TIMER4, .clockId_slect=FCLK_TIMER4_SEL_26M, .time_req=PXIC0_TIMER4_IRQn, .pad=38},
};



static pwm_ctx_t pwms[PWM_CH_MAX];

static int luat_pwm_mapid(int channel) {
    int map_id = -1;
    for (size_t i = 0; i < sizeof(maps) / sizeof(pwm_map_t); i++)
    {
        if (maps[i].pwm_ch == channel) {
            map_id = i;
            break;
        }
    }
    return map_id;
}

PLAT_PA_RAMCODE static void Timer_ISR()
{

    volatile int i = 0;

    //PWM 3/5 channels need to be skipped, 3 is tick64, 5 is occupied by CP
    for(i = 0;i < 5;i++)
    {
        if (i == 3)
            continue;
        if (TIMER_getInterruptFlags(i) & TIMER_MATCH2_INTERRUPT_FLAG)
        {
            TIMER_clearInterruptFlags(i, TIMER_MATCH2_INTERRUPT_FLAG);
            g_s_pnum_update[i]++;
            if (g_s_pnum_update[i] >= g_s_pnum_set[i])
            {
                //luat_pwm_close(i);
                TIMER_updatePwmDutyCycle(i,0);
                //luat_pwm_update_dutycycle(i,0);
                TIMER_stop(i);
                //LUAT_DEBUG_PRINT("PWM STOP %d",g_s_pnum_update[i]);
                g_s_pnum_update[i] = 0;
            }
        }
    }
}

int luat_pwm_open(int channel, size_t freq,  size_t pulse, int pnum) {
    PadConfig_t config = {0};

    int map_id = luat_pwm_mapid(channel);
    if (map_id == -1)
        return -1;

    if (freq > MAX_FREQ)
        return -2;
    if (pulse > 100)
        pulse = 100;

    if(if_initialized_timer(channel % 10) < 0)
    {
        return -4;   // hardware timer is used
    }

    // LUAT_DEBUG_PRINT("luat_pwm_open get timer_id %d",timer_id);
    PAD_getDefaultConfig(&config);
    config.mux = PAD_MUX_ALT5;
    
    //To support reuse, only take the low bits
    channel = channel % 10;

    g_s_pnum_set[channel] = pnum;
    PAD_setPinConfig(maps[map_id].pad, &config);

    CLOCK_setClockSrc(maps[map_id].clockId, maps[map_id].clockId_slect);
    CLOCK_setClockDiv(maps[map_id].clockId, 1);
    TIMER_driverInit();

    pwms[channel].timer_config.pwmFreq_HZ = freq;
    pwms[channel].timer_config.srcClock_HZ = GPR_getClockFreq(maps[map_id].clockId);  
    pwms[channel].timer_config.dutyCyclePercent = pulse;
    TIMER_setupPwm(channel, &pwms[channel].timer_config);
    if(0 != pnum)
    {
        TIMER_interruptConfig(channel, TIMER_MATCH0_SELECT, TIMER_INTERRUPT_DISABLED);
        TIMER_interruptConfig(channel, TIMER_MATCH1_SELECT, TIMER_INTERRUPT_DISABLED);
        TIMER_interruptConfig(channel, TIMER_MATCH2_SELECT, TIMER_INTERRUPT_LEVEL);

        XIC_SetVector(maps[map_id].time_req,Timer_ISR);
        XIC_EnableIRQ(maps[map_id].time_req);
    }

    TIMER_start(channel);

    return 0;
}

int luat_pwm_update_dutycycle(int channel,size_t pulse)
{
    int map_id = luat_pwm_mapid(channel);
    if (map_id == -1)
        return -1;
    channel = channel % 10;

    pwms[channel].timer_config.dutyCyclePercent = pulse;
    TIMER_updatePwmDutyCycle(channel, pulse);
    return 0;
}

int luat_pwm_setup(luat_pwm_conf_t* conf)
{
    int channel = conf->channel;
    int map_id = luat_pwm_mapid(channel);
    if (map_id == -1)
        return -1;
    if (conf->precision != 100) {
        // If and only supports 100 resolution, wait until the driver is rebuilt before changing it.
        return -1;
    }
    channel = channel % 10;
    // Determine whether only the duty cycle has been modified. If and only if the frequencies are the same and pnum is 0 (that is, continuous output), individual changes are supported.
    if (pwms[channel].timer_config.pwmFreq_HZ == conf->period && conf->pnum == 0) {
        if (conf->pulse != pwms[channel].timer_config.dutyCyclePercent) {
            pwms[channel].timer_config.dutyCyclePercent = conf->pulse;
            TIMER_updatePwmDutyCycle(conf->channel % 10, conf->pulse);
            return 0;
        }
    }
    return luat_pwm_open(conf->channel,conf->period,conf->pulse,conf->pnum);
}

int luat_pwm_capture(int channel,int freq)
{
    int map_id = luat_pwm_mapid(channel);
    if (map_id == -1)
        return -1;
    return EIGEN_TIMER(channel % 10)->TMR[0];
}

int luat_pwm_close(int channel)
{
    int map_id = luat_pwm_mapid(channel);
    if (map_id == -1)
        return -1;
    luat_pwm_update_dutycycle(channel, 0);
    luat_rtos_task_sleep(1);
    TIMER_stop(channel % 10);
    pwms[channel % 10].timer_config.pwmFreq_HZ = 0; // as flag bit
    g_s_pnum_update[channel % 10] = 0;
    g_s_pnum_set[channel % 10] = 0;
    //Restore GPIO default configuration

    return 0;
}
#else

#define PWM_CH_MAX (5)

typedef struct
{
	uint8_t gpio;
	uint8_t gpio_alt;
}luat_pwm_ec618_pin_t;

typedef struct
{
	const int irq_line;
	const luat_pwm_ec618_pin_t pin[4];
	const ClockId_e id;
	const ClockSelect_e select;
	CBFuncEx_t callback;
	void *user_param;

	uint32_t pulse_total_num;
	uint32_t pulse_cnt;
	uint32_t freq;
	uint32_t last_pulse_rate;
	uint8_t update_period;
}luat_pwm_ctrl_t;

static luat_pwm_ctrl_t g_s_pwm_table[PWM_CH_MAX] =
{
		{
				PXIC0_TIMER0_IRQn,
				{
						{HAL_GPIO_23, 0}, {HAL_GPIO_1, 0}, {HAL_GPIO_19, 4}, {HAL_GPIO_14, 0}
				},
				FCLK_TIMER0,
				FCLK_TIMER0_SEL_26M,
				NULL,
				NULL,
				0,0,0,0
		},
		{
				PXIC0_TIMER1_IRQn,
				{
						{HAL_GPIO_24, 0}, {HAL_GPIO_2, 0}, {HAL_GPIO_29, 0}, {HAL_GPIO_15, 0}
				},
				FCLK_TIMER1,
				FCLK_TIMER1_SEL_26M,
				NULL,
				NULL,
				0,0,0,0
		},
		{
				PXIC0_TIMER2_IRQn,
				{
						{HAL_GPIO_25, 0}, {HAL_GPIO_16, 0}, {HAL_GPIO_30, 0}, {0xff, 0}
				},
				FCLK_TIMER2,
				FCLK_TIMER2_SEL_26M,
				NULL,
				NULL,
				0,0,0,0
		},
		{
				-1,
				{
						{0xff, 0}, {0xff, 0}, {0xff, 0}, {0xff, 0}
				},
				0,0,
				NULL,
				NULL,
				0,0,0,0
		},
		{
				PXIC0_TIMER4_IRQn,
				{
						{HAL_GPIO_27, 0}, {HAL_GPIO_18, 0}, {HAL_GPIO_18, 4}, {0xff, 0}
				},
				FCLK_TIMER4,
				FCLK_TIMER4_SEL_26M,
				NULL,
				NULL,
				0,0,0,0
		},
};

__ISR_IN_RAM__ static void s_timer_handle(uint32_t instance)
{
    if (EIGEN_TIMER(instance)->TSR & TIMER_MATCH1_INTERRUPT_FLAG)
    {
    	EIGEN_TIMER(instance)->TSR = TIMER_MATCH1_INTERRUPT_FLAG;
    	g_s_pwm_table[instance].pulse_cnt++;
        if (g_s_pwm_table[instance].pulse_total_num && (g_s_pwm_table[instance].pulse_cnt >= g_s_pwm_table[instance].pulse_total_num))
        {
            TIMER_stop(instance);
            if (g_s_pwm_table[instance].callback)
            {
            	g_s_pwm_table[instance].callback(instance, g_s_pwm_table[instance].user_param);
            }
        }
    }
}

__ISR_IN_RAM__ static void timer0_isr(void)
{
	s_timer_handle(0);
}
__ISR_IN_RAM__ static void timer1_isr(void)
{
	s_timer_handle(1);
}
__ISR_IN_RAM__ static void timer2_isr(void)
{
	s_timer_handle(2);
}
__ISR_IN_RAM__ static void timer4_isr(void)
{
	s_timer_handle(4);
}


int luat_pwm_set_callback(int channel, CBFuncEx_t callback, void *param)
{
	uint8_t instance = channel % 10;
	uint8_t alt = channel / 10;
	if (alt > 3) return -1;
	if ((instance >= PWM_CH_MAX) || (instance == 3) || g_s_pwm_table[instance].freq) return -1;
	g_s_pwm_table[instance].callback = callback;
	g_s_pwm_table[instance].user_param = param;
	return 0;
}

int luat_pwm_open(int channel, size_t freq,  size_t pulse, int pnum) {
	uint8_t instance = channel % 10;
	uint8_t alt = channel / 10;
	if (alt > 3) return -1;
	if ((instance >= PWM_CH_MAX) || (instance == 3) || (freq > 13000000)) return -1;
	CLOCK_setClockSrc(g_s_pwm_table[instance].id, g_s_pwm_table[instance].select);
	CLOCK_setClockDiv(g_s_pwm_table[instance].id, 1);
	XIC_DisableIRQ(g_s_pwm_table[instance].irq_line);
	TIMER_stop(instance);
	TIMER_deInit(instance);
	TimerConfig_t config;
	TIMER_getDefaultConfig(&config);

	TIMER_init(instance, &config);
	if (pulse > 1000) pulse = 1000;
	g_s_pwm_table[instance].freq = freq;
	g_s_pwm_table[instance].last_pulse_rate = pulse;
	g_s_pwm_table[instance].pulse_total_num = pnum;
	g_s_pwm_table[instance].pulse_cnt = 0;
	uint32_t period = 26000000 / freq;
	uint64_t temp = period;
	temp *= pulse;
	uint32_t low_cnt = period - temp / 1000;
//	DBG("%u,%u", period, low_cnt);
	/* Set PWM period */
	EIGEN_TIMER(instance)->TMR[1] = period - 1;
	switch(pulse)
	{
	case 0:
		EIGEN_TIMER(instance)->TMR[0] = period;
		break;
	case 1000:
		EIGEN_TIMER(instance)->TMR[0] = period - 1;
		break;
	default:
		EIGEN_TIMER(instance)->TMR[0] = low_cnt?(low_cnt - 1):0;
		break;
	}

	EIGEN_TIMER(instance)->TIVR = 0;

	/* Enable PWM out */
	EIGEN_TIMER(instance)->TCTLR = (2 << TIMER_TCTLR_MCS_Pos) | TIMER_TCTLR_PWMOUT_Msk;

    if(0 != pnum)
    {
    	EIGEN_TIMER(instance)->TCTLR |= TIMER_TCTLR_IE_1_Msk;
        switch(instance)
        {
        case 0:
        	XIC_SetVector(g_s_pwm_table[instance].irq_line,timer0_isr);
        	break;
        case 1:
        	XIC_SetVector(g_s_pwm_table[instance].irq_line,timer1_isr);
        	break;
        case 2:
        	XIC_SetVector(g_s_pwm_table[instance].irq_line,timer2_isr);
        	break;
        case 4:
        	XIC_SetVector(g_s_pwm_table[instance].irq_line,timer4_isr);
        	break;
        }
        XIC_ClearPendingIRQ(g_s_pwm_table[instance].irq_line);
        XIC_EnableIRQ(g_s_pwm_table[instance].irq_line);
        XIC_SuppressOvfIRQ(g_s_pwm_table[instance].irq_line);
    }
//    DBG("%x,%x,%x,%x", EIGEN_TIMER(instance)->TCTLR, EIGEN_TIMER(instance)->TMR[0], EIGEN_TIMER(instance)->TMR[1], EIGEN_TIMER(instance)->TMR[2]);
    if (pulse >= 1000)
    {
    	GPIO_Config(g_s_pwm_table[instance].pin[alt].gpio, 0, 1);
    	GPIO_IomuxEC618(GPIO_ToPadEC618(g_s_pwm_table[instance].pin[alt].gpio, g_s_pwm_table[instance].pin[alt].gpio_alt), g_s_pwm_table[instance].pin[alt].gpio_alt, 0, 0);
    	TIMER_start(instance);
    	delay_us(100);
    	GPIO_IomuxEC618(GPIO_ToPadEC618(g_s_pwm_table[instance].pin[alt].gpio, g_s_pwm_table[instance].pin[alt].gpio_alt), 5, 1, 0);
    	return 0;
    }
    GPIO_IomuxEC618(GPIO_ToPadEC618(g_s_pwm_table[instance].pin[alt].gpio, g_s_pwm_table[instance].pin[alt].gpio_alt), 5, 1, 0);
    TIMER_start(instance);
    return 0;
}

int luat_pwm_update_dutycycle(int channel,size_t pulse)
{
	uint8_t instance = channel % 10;
	uint8_t alt = channel / 10;
	if (alt > 3) return -1;
	if ((instance >= PWM_CH_MAX) || (instance == 3)) return -1;
	uint32_t period = 26000000 / g_s_pwm_table[instance].freq;
	uint64_t temp = period;
	temp *= pulse;
	uint32_t low_cnt = period - temp / 1000;
	if (g_s_pwm_table[instance].update_period)
	{
		while (EIGEN_TIMER(instance)->TCAR > 5) {;}
		EIGEN_TIMER(instance)->TMR[1] = period - 1;
	}
	//DBG("%u,%u,%u,%u,%u", pulse, g_s_pwm_table[instance].last_pulse_rate, low_cnt, EIGEN_TIMER(instance)->TMR[0], EIGEN_TIMER(instance)->TCAR);
	if ( (EIGEN_TIMER(instance)->TMR[0] < (EIGEN_TIMER(instance)->TMR[1] - 5)) && pulse && g_s_pwm_table[instance].last_pulse_rate && (g_s_pwm_table[instance].last_pulse_rate != 1000) && (low_cnt < EIGEN_TIMER(instance)->TMR[0]))
	{
		while (EIGEN_TIMER(instance)->TCAR <= EIGEN_TIMER(instance)->TMR[0]) {;}
	}
	switch(pulse)
	{
	case 0:
		EIGEN_TIMER(instance)->TMR[0] = period;
		break;
	case 1000:
		EIGEN_TIMER(instance)->TMR[0] = period - 1;
		break;
	default:
		EIGEN_TIMER(instance)->TMR[0] = low_cnt;
		break;
	}
	g_s_pwm_table[instance].last_pulse_rate = pulse;
    return 0;
}

int luat_pwm_setup(luat_pwm_conf_t* conf)
{
    int channel = conf->channel;
	uint8_t instance = channel % 10;
	switch(conf->precision)
	{
	case 100:
		conf->pulse *= 10;
		break;
	case 256:
		conf->pulse = (conf->pulse * 1000) >> 8;
		break;
	case 1000:
		break;
	default:
		return -1;
	}
	if (conf->pulse && g_s_pwm_table[instance].freq)
	{
		if (!conf->pnum && !g_s_pwm_table[instance].pulse_total_num)
		{
			if ((g_s_pwm_table[instance].freq == conf->period) && (g_s_pwm_table[instance].last_pulse_rate == conf->pulse))
			{
//				DBG("same pwm, no change");
				return 0;
			}
//			DBG("update pwm period %u->%u rate %u->%u", g_s_pwm_table[instance].freq, conf->period, g_s_pwm_table[instance].last_pulse_rate, conf->pulse);
			if (g_s_pwm_table[instance].freq != conf->period)
			{
				g_s_pwm_table[instance].update_period = 1;
				g_s_pwm_table[instance].freq = conf->period;
			}
			luat_pwm_update_dutycycle(channel, conf->pulse);
			g_s_pwm_table[instance].update_period = 0;
			return 0;
		}
	}

    return luat_pwm_open(conf->channel,conf->period,conf->pulse,conf->pnum);
}

int luat_pwm_capture(int channel,int freq)
{
	return -1;
}

int luat_pwm_close(int channel)
{
	uint8_t instance = channel % 10;
	uint8_t alt = channel / 10;
	if ((instance >= PWM_CH_MAX) || (instance == 3)) return -1;
	XIC_DisableIRQ(g_s_pwm_table[instance].irq_line);
	TIMER_stop(instance);
	TIMER_deInit(instance);
	g_s_pwm_table[instance].freq = 0;
	g_s_pwm_table[instance].pulse_total_num = 0;
	g_s_pwm_table[instance].pulse_cnt = 0;
    return 0;
}


#endif
