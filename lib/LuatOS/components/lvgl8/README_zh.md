<h1 align="center"> LVGL - Light and Versatile Graphics Library</h1>
<h2 align="center"> LVGL - lightweight general-purpose graphics library</h2>



<p align="center">
<img src="https://lvgl.io/assets/images/lvgl_widgets_demo.gif">
</p>
<p align="center">
LVGL is a highly tailorable, low resource usage, beautiful interface and easy-to-use embedded system graphics library.
</p>


<h4 align="center">
<a href="https://lvgl.io">Official website</a> ·
<a href="https://docs.lvgl.io/">文档</a> ·
<a href="https://forum.lvgl.io">Forum</a> ·
<a href="https://lvgl.io/services">Services</a> ·
<a href="https://docs.lvgl.io/master/examples.html">例程</a>
</h4>


[English](./README.md) | **中文** | [Português do Brasil](./README_pt_BR.md)


---

#### Table of contents
- [Overview & Overview](#overview&overview)
- [How to get started](#How to get started)
- [routine](#routine)
- [Service](#service)
- [How to contribute to the community](#How to contribute to the community)

## Overview and Overview
### Features
* Rich and powerful modular [graphic components](https://docs.lvgl.io/master/widgets/index.html): buttons, charts, lists, sliders ), pictures, etc.
* Advanced graphics engine: animation, anti-aliasing, transparency, smooth scrolling, layer blending and other effects
* Supports multiple [input devices](https://docs.lvgl.io/master/overview/indev.html): touch screen, keyboard, encoder, keys, etc.
* Support [Multiple display devices](https://docs.lvgl.io/master/overview/display.html)
* Does not depend on a specific hardware platform and can run on any display
* Configuration can be tailored (minimum resource usage: 64 kB Flash, 16 kB RAM)
* Multi-language support based on UTF-8, such as Chinese, Japanese, Korean, Arabic, etc.
* Graphical interfaces can be designed and laid out using [CSS-like](https://docs.lvgl.io/master/overview/style.html) (for example: [Flexbox](https://docs.lvgl.io /master/layouts/flex.html), [Grid](https://docs.lvgl.io/master/layouts/grid.html))
* Support operating system, external memory, and hardware acceleration (LVGL has built-in support for STM32 DMA2D, SWM341 DMA2D, NXP PXP and VGLite)
* Even if there is only a [single buffer (frame buffer)](https://docs.lvgl.io/master/porting/display.html), the rendering can be guaranteed to be silky smooth
* All written in C and supports C++ calls
* Support Micropython programming, see: [LVGL API in Micropython](https://blog.lvgl.io/2019-02-20/micropython-bindings)
* Supports [simulator](https://docs.lvgl.io/master/get-started/platforms/pc-simulator.html) simulation, which can be developed without hardware support
* Rich and detailed [routines](https://github.com/lvgl/lvgl/tree/master/examples)
* Detailed [documentation](http://docs.lvgl.io/) and API reference manual, which can be viewed online or downloaded in PDF format

### Hardware requirements

<table>
  <tr>
<td> <strong>Requirements</strong> </td>
<td><strong>Minimum Requirements</strong></td>
<td><strong>Suggested requirements</strong></td>
  </tr>
  <tr>
<td><strong>Architecture</strong></td>
<td colspan="2">16, 32, 64-bit microcontroller or microprocessor</td>
  </tr>
  <tr>
<td> <strong>Clock</strong></td>
<td> > 16 MHz</td>
<td> > 48 MHz</td>
  </tr>

  <tr>
<td> <strong>Flash/ROM</strong></td>
<td> > 64 kB </td>
<td> > 180 kB</td>
  </tr>

  <tr>
<td> <strong>Static RAM</strong></td>
<td> > 16 kB </td>
<td> > 48 kB</td>
  </tr>

  <tr>
<td> <strong>Draw buffer</strong></td>
<td> > 1 × <em>hor. res.</em> pixels </td>
<td> > 1/10 screen size </td>
  </tr>

  <tr>
<td> <strong>Compiler</strong></td>
<td colspan="2"> C99 or newer </td>
  </tr>
</table>

*Note: The resource usage is related to the specific hardware platform, compiler and other factors. Only reference values   are given in the above table*

### Already supported platforms
LVGL itself does not depend on a specific hardware platform. Any microcontroller that meets the LVGL hardware configuration requirements can run LVGL.
Here are just some of them:

- NXP: Kinetis, LPC, iMX, iMX RT
- STM32F1, STM32F3, STM32F4, STM32F7, STM32L4, STM32L5, STM32H7
- Microchip dsPIC33, PIC24, PIC32MX, PIC32MZ
- [Linux frame buffer](https://blog.lvgl.io/2018-01-03/linux_fb) (/dev/fb)
- [Raspberry Pi](http://www.vk3erw.com/index.php/16-software/63-raspberry-pi-official-7-touchscreen-and-littlevgl)
- [Espressif ESP32](https://github.com/lvgl/lv_port_esp32)
- [Infineon Aurix](https://github.com/lvgl/lv_port_aurix)
- Nordic NRF52 Bluetooth Moduless
- Quectel modems
- [SYNWIT SWM341](https://www.synwit.cn/)

LVGL also supports:
- [Arduino library](https://docs.lvgl.io/master/get-started/platforms/arduino.html)
- [PlatformIO package](https://platformio.org/lib/show/12440/lvgl)
- [Zephyr library](https://docs.zephyrproject.org/latest/reference/kconfig/CONFIG_LVGL.html)
- [ESP32 component](https://docs.lvgl.io/master/get-started/platforms/espressif.html)
- [NXP MCUXpresso component](https://www.nxp.com/design/software/embedded-software/lvgl-open-source-graphics-library:LITTLEVGL-OPEN-SOURCE-GRAPHICS-LIBRARY)
- [NuttX library](https://docs.lvgl.io/master/get-started/os/nuttx.html)
- [RT-Thread RTOS](https://www.rt-thread.org/document/site/#/rt-thread-version/rt-thread-standard/packages-manual/lvgl-docs/introduction)


## How to get started
Please follow the following order to learn LVGL:
1. Use [webpage online routine](https://lvgl.io/demos) to experience LVGL (3 minutes)
2. Read the document [Introduction] (https://docs.lvgl.io/master/intro/index.html) chapter to get a preliminary understanding of LVGL (5 minutes)
3. Read the document Quick [Quick Overview] (https://docs.lvgl.io/master/get-started/quick-overview.html) chapter again to understand the basic knowledge of LVGL (15 minutes)
4. Learn how to use [simulator](https://docs.lvgl.io/master/get-started/platforms/pc-simulator.html) to simulate LVGL on your computer (10 minutes)
5. Try to practice some [routines](https://github.com/lvgl/lvgl/tree/master/examples)
6. Refer to the [Porting Guide](https://docs.lvgl.io/master/porting/index.html) to try to port LVGL to a development board. LVGL has also provided some good porting [projects](https ://github.com/lvgl?q=lv_port_)
7. Carefully read the document [Overview] (https://docs.lvgl.io/master/overview/index.html) chapter to gain a deeper understanding and familiarity with LVGL (2-3 hours)
8. Browse the documentation section [Widgets](https://docs.lvgl.io/master/widgets/index.html) to learn how to use them
9. If you have any questions, you can go to LVGL[Forum](http://forum.lvgl.io/) to ask
10. Read the document [How to contribute to the community] (https://docs.lvgl.io/master/CONTRIBUTING.html) to see what you can do to help the LVGL community to promote the continuous improvement of LVGL software quality (15 minute)

## Routine

For more routines, please see the [examples](https://github.com/lvgl/lvgl/tree/master/examples) folder.

![LVGL button with label example](https://github.com/lvgl/lvgl/raw/master/docs/misc/btn_example.png)

### C
```c
lv_obj_t * btn = lv_btn_create(lv_scr_act());                   /*Add a button to the current screen*/
lv_obj_set_pos(btn, 10, 10);                                    /*Set its position*/
lv_obj_set_size(btn, 100, 50);                                  /*Set its size*/
lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_CLICKED, NULL); /*Assign a callback to the button*/

lv_obj_t * label = lv_label_create(btn);                        /*Add a label to the button*/
lv_label_set_text(label, "Button");                             /*Set the labels text*/
lv_obj_center(label);                                           /*Align the label to the center*/
...

void btn_event_cb(lv_event_t * e)
{
  printf("Clicked\n");
}
```
### Micropython
For more information, please go to [Micropython official website](https://docs.lvgl.io/master/get-started/bindings/micropython.html).
```python
def btn_event_cb(e):
  print("Clicked")

# Create a Button and a Label
btn = lv.btn(lv.scr_act())
btn.set_pos(10, 10)
btn.set_size(100, 50)
btn.add_event_cb(btn_event_cb, lv.EVENT.CLICKED, None)

label = lv.label(btn)
label.set_text("Button")
label.center()
```

## Serve
LVGL Co., Ltd. was established to provide additional technical support to users using the LVGL graphics library. We are committed to providing the following services:

- Graphic design
- UI design
- Technical consultation and technical support

See https://lvgl.io/services for more information and feel free to contact us if you have any questions.


## How to contribute to the community
LVGL is an open source project, and you are welcome to participate in community contributions. There are many ways for you to contribute to improving LVGL, including but not limited to:

- Introduce your works or projects based on LVGL design
-Write some routines
- Modify and improve documentation
- bug fix

Please see the documentation section [How to contribute to the community](https://docs.lvgl.io/master/CONTRIBUTING.html) for more information.
