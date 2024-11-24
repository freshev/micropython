
# Steps and methods to adapt to LuatOS

## Basic steps

1. Integration of compilation environment
2. Adaptation of core functions
3. Adaptation of peripherals
4. Adaptation of network interface

## Integration of compilation environment

In this step, you need to add the files in the LuatOS specified directory to the compilation environment of the manufacturer's SDK.

Directories include:

```
$LUATOS
- lua # Lua virtual machine
- luat/Modules # lua library implementation
- luat/packages/vsprintf # Platform-independent printf implementation
- luat/packages/lua-cjson # Platform-independent json library
- luat/packages/heap # Platform-independent lua heap implementation
```

The .h files in the above directory need to be added to the include configuration, and the .c files need to be added to the compilation path.

Please make sure that the compilation can pass before taking the next step.

## Adaptation of core functions

The core functions are msgbus, timer, uart, fs, and several basic methods.

* `msgbus` and `timer`, if it is `freertos`, use the ready-made code in the luat/freertos directory, otherwise you need to implement `luat_msgbus.h` and `luat_timer.h`
* `uart`, corresponding to `luat_uart.h` needs to be implemented one by one
* `fs`, file system, if it supports posix style, it comes with its own implementation, otherwise it needs to implement `luat_fs.h`
* Several basic methods can be implemented one by one according to the error reported by the link during compilation, mainly in `luat_base.h`

After completing the integration of the compilation environment, add the following code at the entrance of the user program

```c
#include "bget.h"

bpool(ptr, size); // Lua vm requires a piece of memory for internal allocation, giving the first address and size.
luat_main(); // luat_main is the main entry point of LuatOS, this method usually does not return.
```

You can verify the startup process of LuatOS.

Recommended implementation steps
1. Complete the uart function first, this is essential
2. Imitate the rtt version of `luat_openlibs` to implement the loading of lua libraries. In the initial stage, you can only load the libraries that come with lua itself.
3. Then deal with the implementation of the file system. Even if you only do a short implementation, you must ensure that the compilation passes.
4. Add luat_main and start to check and fill in the gaps. After the supplement is complete, the startup will prompt that `main.luac` cannot be found. At this time, the next step is another step.

## Adaptation of peripherals

Peripherals usually refer to `gpio`/`i2c`/`spi`, just implement the corresponding .h file, and then load it in `luat_openlibs`

## Adaptation of network interface

This needs to implement `netclient.h`, which will be more complicated. Please refer to the implementation of rtt. If you have any questions, please report an issue or join the QQ group to communicate.
