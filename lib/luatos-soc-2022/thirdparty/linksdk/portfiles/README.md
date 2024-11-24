The concept of `Portfiles` is a set of interfaces that describe how the SDK uses the underlying software and hardware resources of the current device. These resources include

+ mutex lock
+ clock
+ memory
+ Random number generator

---
When the SDK is ported to an embedded device running a different OS, the only code that needs to be changed is the C file in the `portfiles` directory. Below we provide some example documents of porting.

MQTT Lianyun based on MCU (running FreeRTOS) + 4G Modules:
+ [STM32+Hezhou Air724UG Modules transplantation](https://help.aliyun.com/document_detail/254820.html?spm=a2c4g.11174283.6.1045.11b14c07U31eqg)
