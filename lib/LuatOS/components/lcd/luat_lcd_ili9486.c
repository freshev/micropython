#include "luat_base.h"
#include "luat_lcd.h"

#define LUAT_LOG_TAG "ili9486"
#include "luat_log.h"

static const uint16_t ili9486_init_cmds[] = {
    0x02E0,0x0300,0x0307,0x030f,0x030D,0x031B,0x030A,0x033c,0x0378,0x034A,0x0307,0x030E,0x0309,0x031B,0x031e,0x030f,
    0x02E1,0x0300,0x0322,0x0324,0x0306,0x0312,0x0307,0x0336,0x0347,0x0347,0x0306,0x030a,0x0307,0x0330,0x0337,0x030f,
    0x02C0,0x0310,0x0310,
    0x02C1,0x0341,
    0x02C5,0x0300,0x0322,0x0380,
    0x023A,0x0355,
    0x02B0,0x0300,
    0x02B1,0x03B0,0x0311,
    0x02B4,0x0302,
    0x02B6,0x0302,0x0302,
    0x02B7,0x03C6,
    0x02E9,0x0300,
    0x02F7,0x03A9,0x0351,0x032C,0x0382,
};


luat_lcd_opts_t lcd_opts_ili9486 = {
    .name = "ili9486",
    .init_cmds_len = sizeof(ili9486_init_cmds)/sizeof(ili9486_init_cmds[0]),
    .init_cmds = ili9486_init_cmds,
    .direction0 = 0x48,
    .direction90 = 0x88,
    .direction180 = 0x28,
    .direction270 = 0xE8
};
