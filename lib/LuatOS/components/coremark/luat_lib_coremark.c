/*@Modules  coremark
@summary 跑分
@version 1.0
@date    2022.01.11
@tag LUAT_USE_COREMARK*/
#include "luat_base.h"

#include "printf.h"

#define LUAT_LOG_TAG "coremark"
#include "luat_log.h"

void ee_main(void);

int ee_printf(const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    char buff[512];
    vsnprintf_(buff, 512, fmt, va);
    va_end(va);
    LLOGD("%s", buff);
    return 0;
}

/*Start running scores
@api coremark.run()
@return nil No return value, the result is printed directly in the log
@usage
-- In most cases, this library will not be included in the official version of the firmware
-- If you need to use it, you can refer to the wiki documentation to compile it yourself or use cloud compilation.
-- https://wiki.luatos.com/develop/compile.html

--The main.lua of the benchmark should remove the hard watchdog code to prevent restarting
-- If the device supports automatic sleep, the sleep function should be turned off
-- If the device supports more frequency operations, it is recommended to set it to the highest frequency
-- Using -O3 gives a higher score than -O2 -Os, usually

-- Will keep the thread exclusive until the execution is completed, and then output the results on the console
coremark.run()

-- Enjoy the running score chart ^_^*/
static int l_coremark_run(lua_State *L) {
    ee_main();
    return 0;
}

#include "rotable2.h"
static const rotable_Reg_t reg_coremark[] =
{
    { "run" ,         ROREG_FUNC(l_coremark_run)},
    { NULL,           ROREG_INT(0)}
};

LUAMOD_API int luaopen_coremark( lua_State *L ) {
    luat_newlib2(L, reg_coremark);
    return 1;
}
