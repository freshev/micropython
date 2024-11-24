
#ifndef LUAT_CONF_BSP
#define LUAT_CONF_BSP

/*Attention!! This file is only for bsp/mini use!!!
Attention!! This file is only for bsp/mini use!!!
Attention!! This file is only for bsp/mini use!!!
Attention!! This file is only for bsp/mini use!!!
Attention!! This file is only for bsp/mini use!!!

If you are trying to modify the configuration of the LuatOS firmware of air780e, air101, air601, air780ep and other Moduless, please go to the corresponding bsp library to modify it.

Not this file, not this file, not this file, not this file, not this file, not this file, not this file!!

For example: 

Modify the luat_conf_bsp.h file in the air780e to luatos-soc-2022 library
air780ep to luatos-soc-2023 library to modify the luat_conf_bsp.h file inside
Modify the luat_conf_bsp.h file in the air101 to luatos-soc-air101 library

Not this file, not this file, not this file, not this file, not this file, not this file, not this file!!
Not this file, not this file, not this file, not this file, not this file, not this file, not this file!!
Not this file, not this file, not this file, not this file, not this file, not this file, not this file!!*/

#include "stdint.h"

#define LUAT_BSP_VERSION "V1111"
#define LUAT_USE_CMDLINE_ARGS 1
// Enable 64-bit virtual machine
// #define LUAT_CONF_VM_64bit
#define LUAT_RTOS_API_NOTOK 1
#define LUAT_RET int
#define LUAT_RT_RET_TYPE	void
#define LUAT_RT_CB_PARAM void *param

#define LUA_USE_VFS_FILENAME_OFFSET 1

#define LUAT_USE_FS_VFS 1

#define LUAT_USE_VFS_INLINE_LIB 1

#define LUAT_COMPILER_NOWEAK 1

#define LUAT_USE_LOG_ASYNC_THREAD 0


#endif
