#ifndef LUAT_KEYBOARD_H
#define LUAT_KEYBOARD_H
#include "luat_base.h"

typedef struct luat_keyboard_ctx
{
    uint16_t port;      // Reserved for multiple keyboards, default is 0
    uint16_t pin_data;  //pin data needs to be reversed according to pin_map
    uint32_t state;     // 1 pressed, 0 release
    void* userdata;
}luat_keyboard_ctx_t;

typedef void(*luat_keyboard_irq_cb)(luat_keyboard_ctx_t* ctx);

typedef struct luat_keyboard_conf {
    uint16_t port;       // Reserved for multiple keyboards, default is 0
    uint16_t pin_conf;   //Mask for pins that need to be enabled
    uint16_t pin_map;    //Input/output configuration of the pin that needs to be enabled
    uint16_t debounce;   //Debounce configuration
    luat_keyboard_irq_cb cb;
    void* userdata;
}luat_keyboard_conf_t;

int luat_keyboard_init(luat_keyboard_conf_t *conf);

int luat_keyboard_deinit(luat_keyboard_conf_t *conf);
#endif
