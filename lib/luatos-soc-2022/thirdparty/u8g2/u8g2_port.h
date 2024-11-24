#ifndef __U8G_PORT_H__
#define __U8G_PORT_H__

#ifdef __cplusplus
    extern "C" {
#endif

#include <string.h>
#include <u8g2.h>
#include <u8x8.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

uint8_t u8x8_luat_gpio_and_delay_default(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);

#ifdef __cplusplus
    }
#endif

#endif /* __U8G_PORT_H__ */
