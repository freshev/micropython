# Keypad and Encoder demo

## Overview

LVGL allows you to control the widgets with keypad and/or encoder without touchpad.
This demo shows how to handle buttons, drop-down lists, rollers, sliders, switches and text inputs without touchpad.
Learn more about the touchpad-less usage of LVGL [here](https://docs.lvgl.io/v7/en/html/overview/indev.html#keypad-and-encoder).

![Keypad and encoder navigation in LVGL embedded GUI library](https://github.com/lvgl/lv_examples/blob/master/src/lv_demo_keypad_encoder/screenshot1.gif?raw=true)

## Run the demo
- In `lv_ex_conf.h` set `LV_USE_DEMO_KEYPAD_AND_ENCODER 1`
- After `lv_init()` and initializing the drivers call `lv_demo_keypad_encoder()`
