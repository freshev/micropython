#include "luat_base.h"
#include "luat_mem.h"

#include "luat_gpio.h"
#include "common_api.h"
#include "luat_i2c.h"
#include "luat_spi.h"

#include "u8g2_port.h"
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"

#define MAX_RETRY 3

uint8_t u8x8_luat_gpio_and_delay_default(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
    uint8_t i;
    switch(msg)
    {
        case U8X8_MSG_DELAY_NANO:            // delay arg_int * 1 nano second
            __asm__ volatile("nop");
            break;

        case U8X8_MSG_DELAY_100NANO:        // delay arg_int * 100 nano seconds
            __asm__ volatile("nop");
            break;

        case U8X8_MSG_DELAY_10MICRO:        // delay arg_int * 10 micro seconds
            for (uint16_t n = 0; n < 320; n++)
            {
                __asm__ volatile("nop");
            }
        break;

        case U8X8_MSG_DELAY_MILLI:            // delay arg_int * 1 milli second
            vTaskDelay(arg_int);
            break;

        case U8X8_MSG_GPIO_AND_DELAY_INIT:
            // Function which implements a delay, arg_int contains the amount of ms

            luat_gpio_mode(u8x8->pins[U8X8_PIN_I2C_DATA],Luat_GPIO_OUTPUT, Luat_GPIO_DEFAULT, Luat_GPIO_HIGH);
            luat_gpio_mode(u8x8->pins[U8X8_PIN_I2C_CLOCK],Luat_GPIO_OUTPUT, Luat_GPIO_DEFAULT, Luat_GPIO_HIGH);
        
            break;

        case U8X8_MSG_DELAY_I2C:
            // arg_int is the I2C speed in 100KHz, e.g. 4 = 400 KHz
            // arg_int=1: delay by 5us, arg_int = 4: delay by 1.25us
            for (uint16_t n = 0; n < (arg_int<=2?160:40); n++)
            {
                __asm__ volatile("nop");
            }
            break;

        //case U8X8_MSG_GPIO_D0:                // D0 or SPI clock pin: Output level in arg_int
        //case U8X8_MSG_GPIO_SPI_CLOCK:

        //case U8X8_MSG_GPIO_D1:                // D1 or SPI data pin: Output level in arg_int
        //case U8X8_MSG_GPIO_SPI_DATA:

        case U8X8_MSG_GPIO_D2:                // D2 pin: Output level in arg_int
            luat_gpio_set(u8x8->pins[U8X8_PIN_D2],arg_int);
            break;

        case U8X8_MSG_GPIO_D3:                // D3 pin: Output level in arg_int
            luat_gpio_set(u8x8->pins[U8X8_PIN_D3],arg_int);
            break;

        case U8X8_MSG_GPIO_D4:                // D4 pin: Output level in arg_int
            luat_gpio_set(u8x8->pins[U8X8_PIN_D4],arg_int);
            break;

        case U8X8_MSG_GPIO_D5:                // D5 pin: Output level in arg_int
            luat_gpio_set(u8x8->pins[U8X8_PIN_D5],arg_int);
            break;

        case U8X8_MSG_GPIO_D6:                // D6 pin: Output level in arg_int
            luat_gpio_set(u8x8->pins[U8X8_PIN_D6],arg_int);
            break;

        case U8X8_MSG_GPIO_D7:                // D7 pin: Output level in arg_int
            luat_gpio_set(u8x8->pins[U8X8_PIN_D7],arg_int);
            break;

        case U8X8_MSG_GPIO_E:                // E/WR pin: Output level in arg_int
            luat_gpio_set(u8x8->pins[U8X8_PIN_E],arg_int);
            break;

        case U8X8_MSG_GPIO_I2C_CLOCK:
            // arg_int=0: Output low at I2C clock pin
            // arg_int=1: Input dir with pullup high for I2C clock pin
            luat_gpio_set(u8x8->pins[U8X8_PIN_I2C_CLOCK],arg_int);
            break;

        case U8X8_MSG_GPIO_I2C_DATA:
            // arg_int=0: Output low at I2C data pin
            // arg_int=1: Input dir with pullup high for I2C data pin
            luat_gpio_set(u8x8->pins[U8X8_PIN_I2C_DATA],arg_int);
            break;

        case U8X8_MSG_GPIO_SPI_CLOCK:
            //Function to define the logic level of the clockline
            luat_gpio_set(u8x8->pins[U8X8_PIN_SPI_CLOCK],arg_int);
            break;

        case U8X8_MSG_GPIO_SPI_DATA:
            //Function to define the logic level of the data line to the display
            luat_gpio_set(u8x8->pins[U8X8_PIN_SPI_DATA],arg_int);
            break;

        case U8X8_MSG_GPIO_CS:
            // Function to define the logic level of the CS line
            luat_gpio_set(u8x8->pins[U8X8_PIN_CS],arg_int);
            break;

        case U8X8_MSG_GPIO_DC:
            //Function to define the logic level of the Data/ Command line
            luat_gpio_set(u8x8->pins[U8X8_PIN_DC],arg_int);
            break;

        case U8X8_MSG_GPIO_RESET:
            //Function to define the logic level of the RESET line
            luat_gpio_set(u8x8->pins[U8X8_PIN_RESET],arg_int);
            break;

        default:
            //A message was received which is not implemented, return 0 to indicate an error
            if ( msg >= U8X8_MSG_GPIO(0) )
            {
                i = u8x8_GetPinValue(u8x8, msg);
                if ( i != U8X8_PIN_NONE )
                {
                    if ( u8x8_GetPinIndex(u8x8, msg) < U8X8_PIN_OUTPUT_CNT )
                    {
                        luat_gpio_set(i, arg_int);
                    }
                    else
                    {
                        if ( u8x8_GetPinIndex(u8x8, msg) == U8X8_PIN_OUTPUT_CNT )
                        {
                            // call yield() for the first pin only, u8x8 will always request all the pins, so this should be ok
                            // yield();
                        }
                        u8x8_SetGPIOResult(u8x8, luat_gpio_get(i) == 0 ? 0 : 1);
                    }
                }
                break;
            }
            return 0;
    }
    return 1;
}
