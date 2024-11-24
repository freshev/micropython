
/*@Modulesnes
@summary nes simulator
@version 1.0
@date 2023.4.10
@tagLUAT_USE_NES*/

#include "luat_base.h"
#include "luat_rtos.h"
#include "luat_lcd.h"
#include "luat_mem.h"
#include "luat_rtos.h"

#include "nes.h"
#include "nes_port.h"

#define LUAT_LOG_TAG "nes"
#include "luat_log.h"

enum {
    Up = 1,
    Down,
    Left,
    Right,
    A,
    B,
    Start,
    Select
};

static luat_rtos_queue_t nes_thread;
static nes_t* nes = NULL;

void nes_task(void *param){
    nes_run(nes);
}

/*nes simulator initialization
@api nes.init(file_path)
@string file_path file path
@return bool returns true if successful, otherwise returns false
@usage
nes.init("/luadb/super_mario.nes")*/
static int l_nes_init(lua_State *L) {
    size_t nes_rom_len;
    const char* nes_rom = luaL_checklstring(L, 1, &nes_rom_len);
    nes = nes_load_file(nes_rom);
    if (!nes){
        return 0;
    }
    if (luat_rtos_task_create(&nes_thread, 4*1024, 50, "nes", nes_task, nes, 0)){
        return 0;
    }
    lua_pushboolean(L, 1);
    return 1;
}
/*nes simulator initialization
@api nes.key(key,val)
@number key key
@number val status 1 pressed 0 lifted
@return bool returns true if successful, otherwise returns false
@usage
nes.init("/luadb/super_mario.nes")*/
static int l_nes_key(lua_State *L) {
    int key = luaL_checkinteger(L, 1);
    int val = luaL_checkinteger(L, 2);
    switch (key){
        case Up:
            nes->nes_cpu.joypad.U1 = val;
            break;
        case Down:
            nes->nes_cpu.joypad.D1 = val;
            break;
        case Left:
            nes->nes_cpu.joypad.L1 = val;
            break;
        case Right:
            nes->nes_cpu.joypad.R1 = val;
            break;
        case A:
            nes->nes_cpu.joypad.A1 = val;
            break;
        case B:
            nes->nes_cpu.joypad.B1 = val;
            break;
        case Start:
            nes->nes_cpu.joypad.ST1 = val;
            break;
        case Select:
            nes->nes_cpu.joypad.SE1 = val;
            break;
        default:
            break;
    }
    return 0;
}

#include "rotable2.h"
static const rotable_Reg_t reg_nes[] =
{
    {"init",    ROREG_FUNC(l_nes_init)  },
    {"key",     ROREG_FUNC(l_nes_key)   },

    //@const Up button
    { "Up",     ROREG_INT(Up)           },
    //@const Down button
    { "Down",   ROREG_INT(Down)         },
    //@const Left button left
    { "Left",   ROREG_INT(Left)         },
    //@const Right button right
    { "Right",  ROREG_INT(Right)        },
    //@const A button A
    { "A",      ROREG_INT(A)            },
    //@const B button B
    { "B",      ROREG_INT(B)            },
    //@const Start button to start
    { "Start",  ROREG_INT(Start)        },
    //@const Select button selection
    { "Select", ROREG_INT(Select)       },
	{ NULL,     ROREG_INT(0)}
};
LUAMOD_API int luaopen_nes( lua_State *L ) {
    luat_newlib2(L, reg_nes);
    return 1;
}
