# EinkBook-LuatOS

### Introduction

Use LuatOS-ESP32 to create an electronic paper book

#### Effect display

![](https://cdn.openluat-luatcommunity.openluat.com/images/20220313202435046_IMG_20220310_154336.jpg)

### hardware

+ Hezhou ESP32-C3 development board
+ MODEL_1in54 ink screen

### software

+ [LuatOS-ESP32](https://gitee.com/dreamcmi/LuatOS-ESP32/tree/master)
+ [GoFrame](https://goframe.org/display/gf)

### Deployment method

#### Server

> Please configure the Golang tool chain yourself first

+ Put the novel you want to read in the `Server\books` directory (currently only supports txt format)
+ Open the novel server program

```bat
cd Server
windows:
    ./run.bat
linux or macos:
    ./run.sh
```

#### Electronic paper book

> It is necessary to unlock GPIO11 of ESP32C3. For details, please refer here [ESP32C3 is unlocked using IO11](https://gitee.com/dreamcmi/LuatOS-ESP32/blob/master/doc/VDD_SPI_AS_GPIO.md)

> Note: You need to use firmware with `sarasa_regular_12` font. If there is no local LuatOS-ESP32 compilation environment, please refer to [Cloud Compilation](https://wiki.luatos.com/develop/compile/Cloud_compilation.html)

Use LuaTools to burn all files in the Scripts directory into the ESP32-C3 Modules

### How to use electronic paper books

Use the BOOT key (GPIO 9) as a function key

+ click: next
+ Double click: Previous
+ Long press: enter/exit
