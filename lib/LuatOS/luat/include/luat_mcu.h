#ifndef LUAT_MCU_H
#define LUAT_MCU_H
#include "luat_base.h"

enum
{
	LUAT_MCU_PERIPHERAL_UART,
	LUAT_MCU_PERIPHERAL_I2C,
	LUAT_MCU_PERIPHERAL_SPI,
	LUAT_MCU_PERIPHERAL_PWM,
	LUAT_MCU_PERIPHERAL_GPIO,
	LUAT_MCU_PERIPHERAL_I2S,
	LUAT_MCU_PERIPHERAL_LCD,
	LUAT_MCU_PERIPHERAL_CAM,
};

int luat_mcu_set_clk(size_t mhz);
int luat_mcu_get_clk(void);

const char* luat_mcu_unique_id(size_t* t);

long luat_mcu_ticks(void);
uint32_t luat_mcu_hz(void);

uint64_t luat_mcu_tick64(void);
int luat_mcu_us_period(void);
uint64_t luat_mcu_tick64_ms(void);
void luat_mcu_set_clk_source(uint8_t source_main, uint8_t source_32k, uint32_t delay);

/**
 * @brief Whether the user has set the IOMUX of the peripheral
 * @param type Peripheral type LUAT_MCU_PERIPHERAL_XXX
 * @param sn Peripheral serial number, 0~7
 * @return 0 configured by the user 1 not configured by the user*/
uint8_t luat_mcu_iomux_is_default(uint8_t type, uint8_t sn);
/**
 * @brief The user controls the IOMUX of the peripheral. If it is not configured or canceled, the default configuration will be used when the peripheral is initialized.
 * @param type Peripheral type LUAT_MCU_PERIPHERAL_XXX
 * @param sn Peripheral serial number, 0~7
 * @param pad_index pad serial number, depending on the chip, it may be the GPIO serial number or the PAD serial number. If it is -1, it means unconfiguring
 * @param alt Multiplexing function serial number, depending on the chip
 * @param is_input, whether it is a simple input function
 * @return none*/
void luat_mcu_iomux_ctrl(uint8_t type, uint8_t sn, int pad_index, uint8_t alt, uint8_t is_input);

void luat_mcu_set_hardfault_mode(int mode);
/**
 * @brief External crystal oscillator reference signal output
 * @param main_enable Main crystal oscillator reference signal output enable, 0 turns off, others turn on
 * @param slow_32k_enable Slow (usually 32K) crystal oscillator reference signal output enable, 0 off, others on
 * @return none*/
void luat_mcu_xtal_ref_output(uint8_t main_enable, uint8_t slow_32k_enable);
#endif

