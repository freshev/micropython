# Bluetooth ws2812 controller

Suitable for air101/air103, but need to be reminded that it is by no means low power consumption!!!

## About Air101/Air103 Bluetooth

Although the development board does not lead to an antenna, these two chips are actually equipped with Bluetooth, but their power consumption is very high and they can only do simple sending and receiving.

Air103 pinout diagram link https://wiki.luatos.com/chips/air103/mcu.html#pinout

Among them, 14 pins are marked NC, but are actually ANT, antenna pins, and can be used for flying wires.

Air101 pinout diagram link https://wiki.luatos.com/chips/air101/mcu.html

Eight of them are labeled NC, but are actually ANT.

## About firmware

In the release version of the firmware compressed package, only the firmware with the word BLE supports Bluetooth. You can also use cloud compilation to customize it yourself.

https://gitee.com/openLuat/LuatOS/releases

https://wiki.luatos.com/develop/compile/Cloud_compilation.html

## WeChat Mini Program

Just search for "LuatOS Bluetooth".

The Bluetooth name of the device starts with `LOS-` by default. If the antenna is not luat out, you can search it by attaching the mobile phone to the chip.

