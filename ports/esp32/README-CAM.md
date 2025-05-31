MicroPython port to the ESP32-CAM
================================

This is a port of MicroPython to the Espressif ESP32 series of
microcontrollers.  It uses the ESP-IDF framework and MicroPython runs as
a task under FreeRTOS.

Supported features include:
- Using board's PSRAM.
- Configurable hardware I2C. I2C slave mode support.
- Wrapper for ESP-logging.
- REPL (Python prompt) over UART0.
- 16k stack for the MicroPython task and approximately 100k Python heap.
- Many of MicroPython's features are enabled: unicode, arbitrary-precision
  integers, single-precision floats, complex numbers, frozen bytecode, as
  well as many of the internal modules.
- Internal filesystem using the flash (currently 2M in size).
- The machine module with GPIO, UART, SPI, software I2C, ADC, DAC, PWM,
  TouchPad, WDT and Timer.
- The network module with WLAN (WiFi) support.
- Bluetooth low-energy (BLE) support via the bluetooth module.

Initial development of this ESP32 port was sponsored in part by Microbric Pty Ltd.

PSRAM, I2C driver and ESP logging commonly used from 
[MicroPython_ESP32_psRAM_LoBo](https://github.com/loboris/MicroPython_ESP32_psRAM_LoBo)
rewritten to ESP-IDF v5.1.2 base.

Setting up ESP-IDF and the build environment
--------------------------------------------

MicroPython on ESP32 requires the Espressif IDF version 5 (IoT development
framework, aka SDK).  The ESP-IDF includes the libraries and RTOS needed to
manage the ESP32 microcontroller, as well as a way to manage the required
build environment and toolchains needed to build the firmware.

The ESP-IDF changes quickly and MicroPython only supports certain versions.
Currently MicroPython supports only v5.0.4, 5.1.2.

To install the ESP-IDF the full instructions can be found at the
[Espressif Getting Started guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html#installation-step-by-step).

The Espressif instructions will guide you through using the `install.sh`
(or `install.bat`) script to download the toolchain and set up your environment.
The steps to take are summarised below.



Building the firmware (Linux)
-----------------------------

To check out a copy of the IDF use git clone:

```bash
$ git clone -b v5.1.2 --recursive https://github.com/espressif/esp-idf.git
```

You can replace `v5.1.2` with any other supported version.
(You don't need a full recursive clone; see the `ci_esp32_setup` function in
`tools/ci.sh` in this repository for more detailed set-up commands.)

If you already have a copy of the IDF then checkout a version compatible with
MicroPython and update the submodules using:

```bash
$ cd esp-idf
$ git checkout v5.1.2
$ git submodule update --init --recursive
```

After you've cloned and checked out the IDF to the correct version, run the
`install.sh` script:

```bash
$ cd esp-idf
$ ./install.sh       
$ source export.sh   
```

The `install.sh` step only needs to be done once. You will need to source
`export.sh` for every new session.

To build MicroPython for the ESP32-CAM run:

```bash
$ cd ports/esp32
$ ./install.sh
$ ./make.sh
```


Building the firmware (Windows + VS Code)
-----------------------------------------
* Download and install VS Code ESP-IDF Extension 
* Configure ESP-IDF Extension at ESP-IDF:EXPLORER -> Advanced -> Configure ESP-IDF Extension to set v5.1.2 SDK
or select v5.1.2 SDK at ESP-IDF:EXPLORER -> Select current ESP-IDF version
* Set device target at ESP-IDF:EXPLORER -> Set Espressif Device Target to ESP32 chip (via ESP USB Bridge)
* At ESP-IDF:EXPLORER -> SDK Configuration Editor (menuconfig) find and select PSRAM support and SAVE
* Attention: Add PSRAM support each time after ESP-IDF:EXPLORER -> Full Clean

idf.py add-dependency --path main_esp32 "espressif/esp32-camera"



This will produce a combined `firmware_camera.bin` image in the `./firmware/`
folder (this firmware image is made up of: bootloader.bin, partitions.bin
and micropython.bin).

To flash the firmware you can use esptool.py

```bash
$ esptool.py --chip auto --port /dev/ttyUSB0 --baud 921600 write_flash -z 0x0000 firmware_camera.bin
```

Note: the above "make.sh" commands are thin wrappers for the underlying `idf.py`
build tool that is part of the ESP-IDF.  You can instead use `idf.py` directly,
for example:

```bash
$ idf.py -D MICROPY_BOARD=ESP32_CAM build
$ idf.py flash
```

Getting a Python prompt on the device
-------------------------------------

You can get a prompt via the serial port, via UART0, which is the same UART
that is used for programming the firmware.  The baudrate for the REPL is
115200 and you can use a command such as:

```bash
$ picocom -b 115200 /dev/ttyUSB0
```

or

```bash
$ miniterm.py /dev/ttyUSB0 115200
```

The UART protocol for ESP32-CAM board use disabled RTS and DTR.
So you can not use putty to get REPL prompt.

For use [Thonny](https://thonny.org/) make changes into Thonny configuration.ini
Usually it located at
```
C:\Users\USER\AppData\Roaming\Thonny\configuration.ini
```

```bash
[ESP32]
...
dtr = False
rts = False
```

To use AMPY micropython tool use [this](https://github.com/freshev/Universal_AMPY).
This tool is proposed by [rt-thread VSCode extension for micropython](https://github.com/SummerGift/micropython-tools).

You can also use `idf.py monitor`.


Configuring camera with ESP32-CAM board
---------------------------------------

```python
import camera
from machine import I2C, Pin

camera.init(0, format = camera.JPEG)
flash = Pin(4, Pin.OUT, 0)
buf = camera.capture()
print(len(buf))
```

Configuring the I2C Slave mode with ESP32-CAM board
---------------------------------------------------

The callback version is:

```python
from machine import I2C

uplink=I2C(scl=12, sda=13, mode=I2C.SLAVE)
uplink.callback(lambda res:print(res.getcbdata())) #read all bytes from I2C master node
```

or use getdata/setdata function

```python
from machine import I2C

uplink=I2C(scl=12, sda=13, mode=I2C.SLAVE, slave_wbuflen=32768)
uplink.getdata(0, 10) # read 10 bytes from I2C master node
uplink.setdata(bytearray(32768),0)
```
Note that maximum write buffer length in slave mode limited to 64kB.
Buffer length more then 32kB leads to unstable PSRAM read/write/operations, cyclic reboots etc. Use on your own risk.

Configuring the ESP-logging
---------------------------
set log tag (string) and verbosity (1-5)

```python
import machine
machine.loglevel("*",5)
machine.redirectlog()
...
machine.loglevel("[I2C]",1)
...
machine.restorelog()
```

Getting heap info
-----------------

```python
import machine
machine.heap_info()
```

Autorun
-------
In order to inject "main.py" script right into firmware you can use:
1) "default_main.py" script
2) "default_main.sh" build script

"default_main.py" transforms to "main.py" at the board filesystem during board boot process.

To do this add desired content to "default_main.py" and run:

```bash
$ ./default_main.sh
$ ./make
```

To disable this feature just remove script ./ports/esp32/modules/boot.py and remake:
```bash
$ ./mclean.sh
$ ./make
```

Full example
------------
Slave part on ESP32-CAM module:
```python
# micropython esp32 camera module in I2C slave mode
import machine
from machine import WDT, I2C, Pin
import camera
import utime

class cam_slave:

    def __init__(self, scl = 12, sda = 13, freq=100000, wdt_timeout = 120000):
        self.wdt = WDT(timeout=wdt_timeout)
        self.buffer = []
        self.status(1) # blink
        #init camera module
        try:
            camera.init(0, format = camera.JPEG)
        except:
            camera.deinit()
            self.status(1) # blink
            try:
                camera.init(0, format = camera.JPEG)
            except:
                self.status(10) # blink 10 times
                print("Camera init failed. Resetting...")

                machine.reset()

        self.flash = Pin(4, Pin.OUT, 0)
        self.counter = 0       # packet counter
        self.datamaxsize = 100 # 100 bytes in each packet, depends on master recive buffer length
        try:
            self.uplink = I2C(scl = scl, sda = sda, freq = freq, mode = I2C.SLAVE, slave_wbuflen=256)
        except OSError as ex:
            if(ex.errno == 'I2C bus already used'):
                print(ex)
                I2C.deinit_all()
                self.uplink = I2C(scl = scl, sda = sda, freq = freq, mode = I2C.SLAVE, slave_wbuflen=256)
            else: self.uplink = None
        if (self.uplink is not None):
            self.uplink.callback(self.i2ccb)
        #print(self.uplink)

    def i2ccb(self, res):
        com = res.getcbdata()
        #print(com)
        if com == b'ping':      self.ok(); return
        if com == b'flash-on':  self.flash.on(); self.ok(); return
        if com == b'flash-off': self.flash.off(); self.ok(); return
        if com == b'capture':
            self.counter = 0
            self.buffer = camera.capture();
            self.ok();
            #print("Total size ", len(self.buffer))
            return
        if com == b'get':
            #send buffer slice by slice
            start = self.counter * self.datamaxsize
            stop =  (self.counter + 1) * self.datamaxsize
            if(stop < len(self.buffer)):
                #print("Send bytes ", start, stop)
                self.ok(self.buffer[start:stop])
                self.counter += 1
            else:
                if(start < len(self.buffer)):
                    #print("Send last ", start, len(self.buffer))
                    self.ok(self.buffer[start:])
                    self.counter += 1
                else:
                    #print("Send empty buffer")
                    self.ok(bytearray([])) # send empty buffer
                    self.counter = 0
            return
        #command unknown
        if(self.wdt is not None): self.wdt.feed()
        self.uplink.setdata(bytearray([100]), 0) # send code 100 (failed)

    def ok(self, data = None):
        if(self.uplink != None):
            if(self.wdt is not None): self.wdt.feed()
            if(data != None):
                self.uplink.setdata(bytearray([201]), 0) # send code 201 (success with data)
                self.uplink.setdata(len(data).to_bytes(2,'little'), 0) # send 2 bytes
                if(len(data) > 0): self.uplink.setdata(data, 0) # send buffer (max buffer length = 65536, unstable, using PSRAM)
            else:
                self.uplink.setdata(bytearray([200]), 0) # send code 200 (success)

    def status(self, num): # blink internal LED 'num' times
        led = Pin(33, Pin.OUT, 1)
        for i in range (0, num):
            led.off();
            utime.sleep_ms(200)
            led.on();
            if (i < num - 1): utime.sleep_ms(200)

import machine
#machine.loglevel("*",5)
#machine.redirectlog()
cam_slave(scl = 12, sda = 13, freq=100000, wdt_timeout = 60000) #60 seconds for WDT, reboots if no commands received.
while(1): utime.sleep(1) # infinite loop
```

Master part on [Ai-ThinkerM A9/A9G module](https://github.com/Ai-Thinker-Open/GPRS_C_SDK) 
with [micropython port by pulkin](https://github.com/pulkin/micropython/tree/master/ports/gprs_a9):
```python
import i2c
import utime

dev = 44
# capture
com = 'capture'
i2c.init(2, 0) # Second I2C fo A9/A9G, frequency = 0-100kHz, 1 - 400kHZ
i2c.transmit(2, dev, bytearray(com))
utime.sleep_ms(50) # minimum wait time for slave can process callback
res = i2c.receive(2, dev, 1)[0]
print("Capture result =", res)

# get photo
buffer = bytearray()
blen = 100
while blen > 0:
  com = 'get'
  i2c.transmit(2, dev, bytearray(com))
  utime.sleep_ms(50) # minimum wait time for slave can process callback
  res = i2c.receive(2, dev, 1)[0]
  buf = i2c.receive(2, dev, 2)
  blen = int.from_bytes(buf, 'little')
  if(blen > 0):
    buf = i2c.receive(2, dev, blen, 100) # large packets are slow, so timeout = 100ms
    buffer = buffer + buf
print("Last get result =", res)
print("Got bytes =", len(buffer))

#f = open("photo.jpg", "w")
#f.write(buffer)
#f.close()
```

Troubleshooting
---------------

Firmware can not be burned to ESP32-CAM module while connected to I2C.
Disconnect external I2C device and try again.
Also this helps when cyclic reboot occurs:
```bash
rst:0x10 (RTCWDT_RTC_RESET),boot:0x33 (SPI_FAST_FLASH_BOOT)
invalid header: 0xffffffff
invalid header: 0xffffffff
...
```

You get error "I2C bus already used" when I2C interface already inited. Use
```python
I2C.deinit_all()
```

"Camera init failed" error happens when no PSRAM available on board, or micropython compiled with no PSRAM support.

Try
```bash
$ ./mclean.sh
$ ./make
```
