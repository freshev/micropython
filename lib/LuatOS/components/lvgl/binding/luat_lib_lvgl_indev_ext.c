/*@Moduleslvgl
@summary LVGL Image Library
@version 1.0
@date 2021.06.01*/
#include "luat_base.h"
#include "luat_lvgl.h"
#include "lvgl.h"
#include "luat_mem.h"

static lv_indev_data_t point_emulator_data = {0};
static lv_indev_data_t keyboard_emulator_data = {0};

static bool point_input_read(lv_indev_drv_t * drv, lv_indev_data_t*data) {
    memcpy(data, drv->user_data, sizeof(lv_indev_data_t));
    // if (((lv_indev_data_t*)drv->user_data)->state == LV_INDEV_STATE_PR){
    //     ((lv_indev_data_t*)drv->user_data)->state == LV_INDEV_STATE_REL;
    // }
    return false;
}

/*Register input device driver
@api lvgl.indev_drv_register(tp, dtp)
@string device type, currently supports "pointer", pointer type/touch type is acceptable, "keyboard", keyboard type
@string device model, currently supports "emulator", emulator type
@return bool returns true if successful, otherwise returns false
@usage
lvgl.indev_drv_register("pointer", "emulator")*/
int luat_lv_indev_drv_register(lua_State* L) {
    lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    const char* type = luaL_checkstring(L, 1);
    int ok = 0;
    const char* dtype;
    if (!strcmp("pointer", type)) {
        indev_drv.type = LV_INDEV_TYPE_POINTER;
        dtype = luaL_checkstring(L, 2);
        if (!strcmp("emulator", dtype)) {
            indev_drv.user_data = &point_emulator_data;
            memset(indev_drv.user_data, 0, sizeof(lv_indev_data_t));
            indev_drv.read_cb = point_input_read;
            lv_indev_drv_register(&indev_drv);
            ok = 1;
        }
        //else if(!strcmp("xpt2046", type)) {
        // // TODO Support xpt2046?
        //}
    }
    else if (!strcmp("keyboard", type)) {
        indev_drv.type = LV_INDEV_TYPE_KEYPAD;
        //dtype = luaL_checkstring(L, 2);
        //if (!strcmp("emulator", dtype)) {
        {
            indev_drv.user_data = &keyboard_emulator_data;
            memset(indev_drv.user_data, 0, sizeof(lv_indev_data_t));
            indev_drv.read_cb = point_input_read;
            lv_indev_drv_register(&indev_drv);
            ok = 1;
        }
    }
    lua_pushboolean(L, ok);
    return 1;
}

/*Update the coordinate data of an analog input device
@api lvgl.indev_point_emulator_update(x, y, state)
@int x coordinate, with the upper left corner as 0 and the lower right corner as the maximum value
@int y coordinate, with the upper left corner as 0 and the lower right corner as the maximum value
@int status, 0 is released, 1 is pressed
@return nil no return value
@usage
-- Simulate clicks on the screen, simulate long presses and short presses through timeout
sys.taskInit(function(x, y, timeout)
    lvgl.indev_point_emulator_update(x, y, 1)
    sys.wait(timeout)
    lvgl.indev_point_emulator_update(x, y, 0)
end, 240, 120, 50)*/
int luat_lv_indev_point_emulator_update(lua_State* L) {
    int x = luaL_checkinteger(L, 1);
    int y = luaL_checkinteger(L, 2);
    int state = luaL_optinteger(L, 3, 1);
    point_emulator_data.point.x = x;
    point_emulator_data.point.y = y;
    point_emulator_data.state = state;
    return 0;
}


/*Update the key values   of the keyboard input device
@api lvgl.indev_kb_update(key)
@int key value, default is 0, key is lifted
@return nil no return value
@usage*/
int luat_lv_indev_keyboard_update(lua_State* L) {
    int key = luaL_optinteger(L, 1, 0);
    keyboard_emulator_data.key = key;
    return 0;
}
