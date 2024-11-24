<h1 align="center"> LVGL - Lightweight and versatile graphics library</h1>
<p align="center">
<img src="https://lvgl.io/assets/images/lvgl_widgets_demo.gif">
</p>
<p align="center">
LVGL provides everything you need to create an embedded GUI with easy-to-use graphics, beautiful visual effects, and low memory consumption.
</p>
<h4 align="center">
<a href="https://lvgl.io">Site</a> ·
<a href="https://docs.lvgl.io/">Documentation</a> ·
<a href="https://forum.lvgl.io">Fórum</a> ·
<a href="https://lvgl.io/services">Serviços</a> ·
<a href="https://docs.lvgl.io/master/examples.html">Exemplos interativos</a>
</h4>

[English](./README.md) | [中文](./README_zh.md) | **Português do Brasil**

---

### Table of contents

- [Overview](#overview)
- [Starting](#starting)
- [Examples](#examples)
- [Services](#services)
- [Contributing](#contributing)

## Overview

### Features
* Powerful [widgets](https://docs.lvgl.io/master/widgets/index.html): buttons, graphs, lists, sliders, images, etc.
* Advanced graphics engine: animations, anti-aliasing, opacity, smooth scrolling, blending modes, etc.
* Support for [multiple input devices](https://docs.lvgl.io/master/overview/indev.html): touch screen, mouse, keyboard, encoder, buttons, etc.
* Support for [multiple monitors](https://docs.lvgl.io/master/overview/display.html)
* Can be used with any microcontroller and display, regardless of hardware
* Scalable to operate with low memory (64 kB Flash, 16 kB RAM)
* Multilingual support with UTF-8 handling, bidirectional alphabet support, Erabe and CJK (Chinese, Japanese and Korean)
* Fully customizable graphic elements through [CSS](https://docs.lvgl.io/master/overview/style.html)
* Layouts poderosos inspirados em CSS: [Flexbox](https://docs.lvgl.io/master/layouts/flex.html) e [Grid](https://docs.lvgl.io/master/layouts/grid.html)
* OS, external memory and GPU are supported but not required. (integrated support for STM32 DMA2D, SWM341 DMA2D and NXP PXP and VGLite)
* Smooth rendering even with a single frame buffer
* Written in C and compatible with C++
* Uso do LittlevGL com Micropython simplificado com [LVGL API in Micropython](https://blog.lvgl.io/2019-02-20/micropython-bindings)
* [Simulator](https://docs.lvgl.io/master/get-started/platforms/pc-simulator.html) to develop on PC without built-in hardware
* More than 100 [simple examples](https://github.com/lvgl/lvgl/tree/master/examples)
* [Documentation](http://docs.lvgl.io/) and API references online and in PDF

### Requirements
Basically every modern controller (that is capable of driving a display) is suitable for running LVGL. The minimum requirements are:

<table>
  <tr>
    <td>
        <strong>Nome</strong>
    </td>
    <td>
        <strong>Minnmo</strong>
    </td>
    <td>
        <strong>Recomendado</strong>
    </td>
  </tr>
  <tr>
    <td>
        <strong>Arquitetura</strong>
    </td>
<td colspan="2">16, 32 or 64-bit microcontroller or processor</td>
  </tr>
  <tr>
    <td>
        <strong>Clock</strong>
    </td>
<td>> 16 MHz</td>
<td>> 48 MHz</td>
  </tr>
  <tr>
    <td>
        <strong>Flash/ROM</strong>
    </td>
<td>> 64 kB</td>
<td>> 180 kB</td>
  </tr>

  <tr>
    <td>
<strong>RAM estetica</strong>
    </td>
<td>> 16 kB</td>
<td>> 48 kB</td>
  </tr>

  <tr>
    <td>
<strong>Draw buffer</strong>
    </td>
<td>> 1 × <em>hor. res.</em> pixels</td>
<td>>1/10 screen size</td>
  </tr>

  <tr>
    <td>
        <strong>Compilador</strong>
    </td>
<td colspan="2">Standard C99 or newer</td>
  </tr>
</table>

*Note that memory usage may vary depending on architecture, compiler, and compilation options.*

### Supported platforms
LVGL is completely platform independent and can be used with any MCU that meets the requirements.
Just to name a few platforms:

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

LVGL is also available for:
- [Arduino library](https://docs.lvgl.io/master/get-started/platforms/arduino.html)
- [PlatformIO package](https://registry.platformio.org/libraries/lvgl/lvgl)
- [Zephyr library](https://docs.zephyrproject.org/latest/reference/kconfig/CONFIG_LVGL.html)
- [ESP32 component](https://docs.lvgl.io/master/get-started/platforms/espressif.html)
- [NXP MCUXpresso component](https://www.nxp.com/design/software/embedded-software/lvgl-open-source-graphics-library:LITTLEVGL-OPEN-SOURCE-GRAPHICS-LIBRARY)
- [NuttX library](https://docs.lvgl.io/master/get-started/os/nuttx.html)
- [RT-Thread RTOS](https://docs.lvgl.io/master/get-started/os/rt-thread.html)

## Getting Started
This list shows the recommended way to learn about the library:

1. Check out the [online demos](https://lvgl.io/demos) to see LVGL in action (3 minutes)
2. Read the documentation [introduction](https://docs.lvgl.io/master/intro/index.html) (5 minutes)
3. Familiarize yourself with the basics of [Quick Overview](https://docs.lvgl.io/master/get-started/quick-overview.html) (15 minutes)
4. Configure um [simulador](https://docs.lvgl.io/master/get-started/platforms/pc-simulator.html) (10 minutos)
5. Try some [Examples](https://github.com/lvgl/lvgl/tree/master/examples)
6. Board for LVGL port. See the [porting] guide(https://docs.lvgl.io/master/porting/index.html) or check out the ready-to-use [Projects](https://github.com/lvgl?q=lv_port_)
7. Read the [overview](https://docs.lvgl.io/master/overview/index.html) to understand the library better (2-3 hours)
8. Check the [widgets](https://docs.lvgl.io/master/widgets/index.html) documentation to see their features and how to use them
9. If you have questions, go to [forum](http://forum.lvgl.io/)
10. Read the [contributing] guide(https://docs.lvgl.io/master/CONTRIBUTING.html) to see how you can help improve LVGL (15 minutes)

## Examples
For more examples, see the [examples](https://github.com/lvgl/lvgl/tree/master/examples) folder.

![Example of LVGL button with label](https://github.com/lvgl/lvgl/raw/master/docs/misc/btn_example.png)

### C

```c
lv_obj_t * button = lv_btn_create(lv_scr_act());                             /* Adds a button to the current screen */
lv_obj_set_pos(button, 10, 10);                                              /* Sets a position for the button on the screen */
lv_obj_set_size(button, 100, 50);                                            /* Sets the size */
lv_obj_add_event_cb(button, button_event_callback, LV_EVENT_CLICKED, NULL);  /* Atribui um retorno de chamada (callback) */

lv_obj_t * label = lv_label_create(button);                                  /* Add a label */
lv_label_set_text(label, "Click here");                                     /* Defines the label text */
lv_obj_center(label);                                                        /* Align the text to the center */
...

void button_event_callback(lv_event_t * e)
{
  printf("Clicado\n");
}
```

### Micropython
Saiba mais em [Micropython](https://docs.lvgl.io/master/get-started/bindings/micropython.html)

```python
def button_event_callback(event):
  print("Clicado")

# Create a button and a label
button = lv.btn(lv.scr_act())
button.set_pos(10, 10)
button.set_size(100, 50)
button.add_event_cb(button_event_callback, lv.EVENT.CLICKED, None)

label = lv.label(button)
label.set_text("Cliquq aqui")
label.center()
```

## Services
LVGL Kft was established to provide a solid foundation for the LVGL library. We offer different types of services
para ajude-lo no desenvolvimento da interface do usuerio:

- Design grefico
- UI implementation
- Consulting/Support

For more information, see [LVGL Services](https://lvgl.io/services). Feel free to get in touch
contact us if you have any questions.

## Contributing
LVGL is an open project and your contributions are very welcome. There are many ways to contribute, from simply
talking about your project, writing examples, improving documentation, fixing bugs even hosting your own
project under the LVGL organization.

For a detailed description of contribution opportunities, visit the [contributing](https://docs.lvgl.io/master/CONTRIBUTING.html) section of the documentation.
