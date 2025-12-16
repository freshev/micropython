# Compile library files without dependencies without this csdk

This document explains the guidelines for using the gcc chain to compile library files separately from csdk.

1. Only suitable for libraries that do not rely on this csdk at all
2. In addition to the basic functions provided by gcc, the corresponding library's dependencies on other functions are declared through dependency-free header files.

## Required toolchains

1. Download [gcc for arm tool chain](http://cdndownload.openluat.com/xmake/toolchains/gcc-arm/gcc-arm-none-eabi-10.3-2021.10-win32.zip)
2. To decompress, do not choose a directory that is too deep and does not contain Chinese characters and special symbols. It is recommended to decompress to the `D drive root directory`. The compressed package comes with a layer of directory `gcc-arm-none-eabi-10.3-2021.10`
3. Assume that the decompressed path is `D:\gcc-arm-none-eabi-10.3-2021.10`, check `D:\gcc-arm-none-eabi-10.3-2021.10\bin\arm-none-eabi- Does g++.exe` exist? If it does not exist, it must be an additional directory. **Be sure to check!!!**

## Parameters required for GCC compilation

```
-mcpu=cortex-m3 
-mthumb 
-std=gnu99 
-nostartfiles 
-mapcs-frame 
-ffunction-sections 
-fdata-sections 
-fno-isolate-erroneous-paths-dereference 
-freorder-blocks-algorithm=stc 
-gdwarf-2
-mslow-flash-data
```

1. The optimization level is recommended to be `-O2` or `-Os`, but it is not mandatory.
2. gnu99 is the minimum requirement, it can be higher

## How to use the generated .a file

1. Assume the name is `libabc.a`, put the .a file into the `lib` directory under the root directory of this SDK, the same as `libgt.a`
2. In xmake.lua in the project directory, add the following statement

```lua
LIB_USER = LIB_USER .. SDK_TOP .. "/lib/libabc.a "
```

## Additional instructions, rely on this csdk method to generate .a files

1. You should follow the example of project/example and put the source files and header files in the src/include directory.
2. Compile in the usual way, such as `build example`
3. After successful compilation, obtain the `.a` file in build/example
