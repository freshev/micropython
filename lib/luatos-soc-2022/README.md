# CSDK for ABCDEGF

[![Github Actions](https://github.com/openLuat/luatos-soc-2022/actions/workflows/ci.yaml/badge.svg?branch=master)](https://github.com/openLuat/luatos-soc-2022/actions/workflows/ci.yaml)

## Precautions before use

1. **It is strongly recommended to use git to download this code base**. If you don’t know how to operate git from the command line, you can use [tortoiseGit](https://tortoisegit.org/download/)
2. There is no AT command at the bottom of this CSDK, and there is no such thing as "calling a certain AT command".

## Document Center

[CSDK software development information](https://doc.openluat.com/wiki/37?wiki_page_id=4544)

## Introduction

CSDK for ABCDEGF is applicable to all ABCDEGF-based Hezhou Moduless, including derivative models and overseas models:

* [Air780E](https://air780e.cn) Hezhou’s main recommended package
* [Air600E](https://air600e.cn) Compatible with packages from other suppliers
* [Air700E](https://air700e.cn) Extremely small package
* [Air780EG](https://air780eg.cn) with GNSS positioning
* [Air780EX](https://air780ex.cn) Stamp hole package

## Application documentation

1. Add custom data to the specified area and [synthesize binpkg](project/example_flash) for mass production flashing
2. [Compile library files without dependencies without this csdk] (doc/build_lib_by_gcc.md)
3. Linux/Mac flash tool (beta version) [ectool2py](https://github.com/openLuat/ectool2py)
4. [Tools required for differential upgrade, support docker] (tools/dtools)
5. [Upgrade package format and generation method](doc/upgrade_pkg.md)

## Directory description

* PLAT packaged SDK, including protocol stack, partial source code, header files, and packaging tools
* project project file, each different project represents a different demo or turnkey solution
* xmake.lua main compilation script, generally does not need to be modified

## Compilation instructions

1. Install xmake, **select to add to PATH**, it is recommended to install to C:\Program Files, other directories may cause problems;
xmake download address: [xmake official website](https://xmake.io/#/guide/installation)
You can access it directly under windows: [Hezhou Cloud Disk](https://pan.air32.cn/s/DJTr?path=%2F%E5%B8%B8%E7%94%A8%E5%B7%A5% E5%85%B7), just download the xmake-2.7.3-win32.exe or xmake-2.7.3-win64.exe corresponding to the system. Versions higher than 2.7.3 are also possible.

**Note: Environment variables need to restart the computer to take effect**

2. If you want to compile the example, execute `build.bat example` in the root directory of this code base to compile it.

3. The generated binpkg is located in the `out` directory, and the log database file is located in the `PLAT` directory.

## How to add your own project

It is recommended to copy `project/example` as a verification project

1. Create a new directory for the project. The directory name must be the same as the project name. Create xmake.lua in the directory. The content is written according to the example. The core is that TARGET_NAME must be consistent with the project name.
2. Of course, the code path is not limited. It can be in any directory of the SDK or even elsewhere on the computer. The premise is that you will change the source code path in xmake.lua in the project.
3. Execute build.bat in the root directory with your project name

## New entry function in the project

There is no main function in this CSDK, and the entry function needs to be registered through macros.

* Add header file common_api.h
* Similar to `void main(void)`, but can have any name, cannot have entry parameters and return parameters, and allows multiple entries at the same time
* After modifying the three macros INIT_HW_EXPORT INIT_DRV_EXPORT INIT_TASK_EXPORT, the function will be called during system initialization. Different macros involve different calling orders.
* `INIT_HW_EXPORT` > `INIT_DRV_EXPORT` > `INIT_TASK_EXPORT` on high priority level
* These three macros have a unified usage of `INIT_HW_EXPORT(function, level)`, where function is the entry function, level is the small priority, fill in "0", "1", "2"... (quotes must be written) , the smaller the number in the quotation marks, the higher the priority.

* `INIT_HW_EXPORT` is generally used for hardware initialization, GPIO and the like, it can be omitted
* `INIT_DRV_EXPORT` is generally used to initialize peripheral drivers, initialize external devices, turn on the power, provide clocks, etc., it can be omitted
* `INIT_TASK_EXPORT` is generally used to initialize tasks. User code basically runs in tasks. In principle, it must have

## Compilation method for storing project files outside the SDK

Assume that the project path is `D:\github\ABCDEGF-webabc` and the directory structure is as follows

```tree
D:\github
- ABCDEGF-webabc
- code
- xmake.lua
- src
                app_main.c
                task_mqtt.c
- inc
                app_main.h
                RTE_Device.h
- doc
- README.md
```

Compilation method:

```cmd
set PROJECT_DIR=D:\github\ABCDEGF-webabc\code
build webabc
```

Note: TARGET_NAME in code\xmake.lua corresponds to the project name on the command line, `webabc`

Restore the default project search logic, and then return to project/xxx to search for xmake.lua

```cmd
set PROJECT_DIR=
build luatos
```

## Supplementary instructions for initial compilation in a non-networked environment

In a network-connected environment, xmake will download the gcc tool chain on its own. However, if the network is not available or the network is restricted, there will usually be this prompt:

```cmd
error: fatal: not a git repository
```

Or a prompt for git/http connection failure. Therefore, here is a method to download and compile the offline gcc tool chain.

1. Download the gcc for arm tool chain [windows version](http://cdndownload.openluat.com/xmake/toolchains/gcc-arm/gcc-arm-none-eabi-10.3-2021.10-win32.zip)/[linux Version](http://cdndownload.openluat.com/xmake/toolchains/gcc-arm/gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2)
2. To decompress, do not choose a directory that is too deep and does not contain Chinese characters and special symbols. It is recommended to decompress to the `D drive root directory`. The compressed package comes with a layer of directory `gcc-arm-none-eabi-10.3-2021.10`
3. Assume that the decompressed path is `D:\gcc-arm-none-eabi-10.3-2021.10`, check `D:\gcc-arm-none-eabi-10.3-2021.10\bin\arm-none-eabi- Does g++.exe` exist? If it does not exist, it must be an additional directory. **Be sure to check!!!**
4. Use a text editor (such as vscode) to open `build.bat` of `this code library` and modify the content as follows

```cmd
Original content:
rem set GCC_PATH=E:\gcc_mcu
Modify it to a statement starting with set. Note that rem is removed and the value is modified.
set GCC_PATH=D:\gcc-arm-none-eabi-10.3-2021.10
```

`Re-open` a `command line cmd`, enter `this code library`, execute `build.bat` and it will compile normally.

5. After the test is ok, you can consider putting the above environment variable GCC_PATH into the system's environment variable settings, and then restore `build.bat`

## Additional instructions for compiling under Linux

To install Xmake, you can use the official one-click script

```shell
curl -fsSL https://xmake.io/shget.text | bash #Use curl to install
wget https://xmake.io/shget.text -O - | bash #Use wget to install
```

Currently only Ubuntu 16.04 and Ubuntu 20.04 amd64 versions are tested

Additional 32-bit support needs to be installed, otherwise the file will not be saved when fcelf is executed. 64bit fcelf may be provided in the future.

```shell
dpkg --add-architecture i386 && apt update
apt-get install -y lib32z1 binutils:i386 libc6:i386 libgcc1:i386 libstdc++5:i386 libstdc++6:i386 p7zip-full
```

## Additional instructions for compiling under macos

Compile docker for wine environment

```shell
./build-wine-docker.sh
```

## License Agreement

[MIT License](LICENSE)
