
#ifndef LUAT_GPIO_LEGACY_H
#define LUAT_GPIO_LEGACY_H


#include "luat_base.h"
#ifdef __LUATOS__
#include "luat_msgbus.h"
int l_gpio_handler(lua_State *L, void* ptr);
#endif
typedef int (*luat_gpio_irq_cb)(int pin, void* args);


#define Luat_GPIO_LOW                 0x00
#define Luat_GPIO_HIGH                0x01

#define Luat_GPIO_OUTPUT         0x00
#define Luat_GPIO_INPUT          0x01
#define Luat_GPIO_IRQ            0x02

#define Luat_GPIO_DEFAULT        0x00
#define Luat_GPIO_PULLUP         0x01
#define Luat_GPIO_PULLDOWN       0x02

#define Luat_GPIO_RISING             0x00
#define Luat_GPIO_FALLING            0x01
#define Luat_GPIO_BOTH               0x02
#define Luat_GPIO_HIGH_IRQ			0x03	//High level interrupt
#define Luat_GPIO_LOW_IRQ			0x04	//Low level interrupt

#define Luat_GPIO_MAX_ID             255

typedef struct luat_gpio
{
    int pin;/**<pin*/
    int mode;/**<GPIO mode*/
    int pull;/**<GPIO pull-down mode*/
    int irq;/**<GPIO interrupt mode*/
    int lua_ref;
    luat_gpio_irq_cb irq_cb;/**<Interrupt handling function*/
    void* irq_args;
    int alt_func;
} luat_gpio_t;

int luat_gpio_setup(luat_gpio_t* gpio);
void luat_gpio_mode(int pin, int mode, int pull, int initOutput);
int luat_gpio_irq_default(int pin, void* args);

#endif