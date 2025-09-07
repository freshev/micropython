MicroPython port to the ESP32-CAM
=================================

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


Setting up ESP-IDF and the build environment
--------------------------------------------

MicroPython on ESP32 requires the Espressif IDF version 5 (IoT development
framework, aka SDK).  The ESP-IDF includes the libraries and RTOS needed to
manage the ESP32 microcontroller, as well as a way to manage the required
build environment and toolchains needed to build the firmware.

The ESP-IDF changes quickly and MicroPython only supports certain versions.
Currently MicroPython supports only v5.4.2.

To install the ESP-IDF the full instructions can be found at the
[Espressif Getting Started guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html#installation-step-by-step).

The Espressif instructions will guide you through using the `install.sh`
(or `install.bat`) script to download the toolchain and set up your environment.
The steps to take are summarised below.

To check out a copy of the IDF use git clone:

```bash
$ git clone -b v5.4.2 --recursive https://github.com/espressif/esp-idf.git
```

You can replace `v5.4.2` with any other supported version.
(You don't need a full recursive clone; see the `ci_esp32_setup` function in
`tools/ci.sh` in this repository for more detailed set-up commands.)

If you already have a copy of the IDF then checkout a version compatible with
MicroPython and update the submodules using:

```bash
$ cd esp-idf
$ git checkout v5.4.2
$ git submodule update --init --recursive
```

After you've cloned and checked out the IDF to the correct version, run the
`install.sh` script:

```bash
$ cd esp-idf
$ ./install.sh       # (or install.bat on Windows)
$ source export.sh   # (or export.bat on Windows)
```

The `install.sh` step only needs to be done once. You will need to source
`export.sh` for every new session.

Building the firmware
---------------------

To build MicroPython for the ESP32-CAM run:

```bash
$ cd ports/esp32
$ ./configure_cam
$ ./make_cam
```

This will produce a combined `merged-binary.bin` image in the `./build/`
folder (this firmware image is made up of: bootloader.bin, partitions.bin
and micropython.bin).

To flash the firmware you can use esptool.py

```bash
$ esptool.py --chip auto --port /dev/ttyUSB0 --baud 921600 write_flash -z 0x0000 build/merged-binary.bin
```

Note: If you get Permission error(13, ...) while burn under Windows with ESP32CAM motherboard, 
try to reinstall CH341 driver from ports/esp32/driver/CH34x_Install_Windows_v3_4.exe


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

To use [Thonny](https://thonny.org/) make changes into Thonny configuration.ini

```bash
[ESP32]
...
dtr = False
rts = False
```

Or use AMPY micropython tool from [here](https://github.com/freshev/Universal_AMPY).
This tool is proposed by [rt-thread VSCode extension for micropython](https://github.com/SummerGift/micropython-tools).

You can also use `idf.py monitor`.


Configuring camera with ESP32-CAM board
---------------------------------------

```python
import camera
from machine import Pin

camera.init(0, format = camera.JPEG)
flash = Pin(4, Pin.OUT, 0)
buf = camera.capture()
print(len(buf))
```

Configuring the I2C Slave mode with ESP32-CAM board
---------------------------------------------------

The callback version is:

```python
from machine import I2CTarget

uplink=I2CTarget(addr=44, scl=12, sda=13)
uplink.irq(callback_function, trigger = I2CTarget.IRQ_END_READ | I2CTarget.IRQ_END_WRITE)

def callback_function(i2c_target):
    flags = i2c_target.irq().flags()
    if flags & I2CTarget.IRQ_END_READ:
        pass

    if flags & I2CTarget.IRQ_END_WRITE:
        i2c_target.readinto(readbuffer)
        ...
        # compose writebuffer
        ...
        i2c_target.write(writebuffer)
```

Note: Maximum internal I2C buffers sizes in slave mode limited to 1027 bytes (See `machine_i2c_target.c`).
Use mem parameter in I2C_Target constructor to use other buffer sizes

Configuring the ESP-logging
---------------------------
set log tag (string) and verbosity (1-5)

```python
import esp
esp.loglevel("*",5)
esp.redirectlog()
...
esp.loglevel("[I2C]",1)
...
esp.restorelog()
```

Getting heap info
-----------------

```python
import esp
esp.heap_info()
```

Full example
------------
Slave part on ESP32-CAM module:
```python
# micropython esp32 camera module in I2C slave mode
import machine
from machine import WDT, Pin, I2CTarget
import time
import camera
import gc

class cam_slave:

    def __init__(self, scl = 12, sda = 13, wdt_timeout = 120000, addr = 44, maxdatasize = 1024):
        self.maxdatasize = maxdatasize
        self.wdt = WDT(timeout=wdt_timeout)
        self.flash = Pin(4, Pin.OUT, 0)
        self.counter = 0       # packet counter
        self.buffer = bytearray(0)
        self.readbuffer = bytearray(10)
        self.writebuffer = bytearray(0)
        self.status(1) # blink

        try:
            camera.init(0, format = camera.JPEG)
            print("Camera init success")
        except Exception as ex:
            print(ex)
            self.status(3) # blink 3 times
            print("Resetting...")
            time.sleep(2)
            machine.reset()

        try:
            self.uplink = I2CTarget(addr = addr, scl = scl, sda = sda, timeout=500000) # mem_addrsize=8, 
        except OSError as ex:
            self.status(5) # blink 5 times
            print("I2C init failed. Resetting...")
            time.sleep(2)
            machine.reset()
        if (self.uplink is not None):
            self.uplink.irq(self.i2ccb, trigger = I2CTarget.IRQ_END_READ | I2CTarget.IRQ_END_WRITE)
        print("uplink =", self.uplink)

    def status(self, num): # blink internal LED 'num' times
        led = Pin(33, Pin.OUT, 1)
        for i in range (0, num):
            led.off()
            time.sleep_ms(200)
            led.on()
            if (i < num - 1): time.sleep_ms(200)

    def i2ccb(self, i2c_target):
        if self.wdt is not None: self.wdt.feed()
        flags = i2c_target.irq().flags()
        if flags & I2CTarget.IRQ_END_READ:
            #print("writebuffer =", self.writebuffer)
            pass

        if flags & I2CTarget.IRQ_END_WRITE:
            i2c_target.readinto(self.readbuffer)
            com = self.readbuffer.replace(b'\0', b'')
            for i in range(len(self.readbuffer)): self.readbuffer[i] = 0

            if len(com) > 0:
                try: print(com.decode())
                except: pass

            if com == b'ping':
                self.counter
                self.ok(i2c_target)
            elif com == b'flash-on':  self.flash.on();  self.ok(i2c_target)
            elif com == b'flash-off': self.flash.off(); self.ok(i2c_target)
            elif com == b'capture':
                self.counter = 0
                self.buffer = camera.capture()
                self.ok(i2c_target)
                #print("Total size ", len(self.buffer))
            elif com == b'get':
                #send buffer part by part
                start = self.counter * self.maxdatasize
                stop =  (self.counter + 1) * self.maxdatasize
                if(stop < len(self.buffer)):
                    #print("Send bytes ", start, stop)
                    self.ok(i2c_target, self.buffer[start:stop])
                    self.counter += 1
                else:
                    if(start < len(self.buffer)):
                        #print("Send last ", start, len(self.buffer))
                        self.ok(i2c_target, self.buffer[start:])
                        self.counter += 1
                    else:
                        #print("Send empty buffer")
                        self.ok(i2c_target, bytearray(0)) # send empty buffer
                        self.counter = 0
            else: #command unknown
                print("command unknown:", com)
                self.failed(i2c_target)

    def failed(self, i2c_target, data = None):
        self.writebuffer = bytearray([100]) # send code 100 (failed)
        i2c_target.write(self.writebuffer)

    def ok(self, i2c_target, data = None):
        self.writebuffer = bytearray(0)
        if(data != None):
            self.writebuffer.append(201) # send code 201 (success with data)
            self.writebuffer.extend(len(data).to_bytes(2,'little')) # send data length (2 bytes)
            if(len(data) > 0): self.writebuffer.extend(data) # send buffer (max buffer length = 1027)
        else:
            self.writebuffer.append(200) # send code 200 (success)
        i2c_target.write(self.writebuffer)

# 120 seconds for WDT,
# auto reboot if no commands received,
# hard coded max I2C send buffer size = 1027 bytes
cam_slave(maxdatasize = 1024, wdt_timeout = 120000)
while(1): time.sleep(1) # infinite loop
```

Master part on [Ai-ThinkerM A9/A9G module](https://github.com/Ai-Thinker-Open/GPRS_C_SDK) 
with [this micropython port](https://github.com/freshev/micropython/tree/master/ports/gprs_a9):
```python
from machine import I2C
import time

i2c = I2C(2, freq = 100000, timeout=500000) # timeout 0.5s
devs = i2c.scan()

if len(devs) > 0:
    dev = devs[0]
    print("found dev =", dev)
    time.sleep_ms(20)

    com = "ping"
    print(com + " -> ", end='')
    res = i2c.writeto(dev, bytearray(com, "utf-8"))
    time.sleep_ms(10) # 10 ms minimum
    print(int(i2c.readfrom(dev, 1)[0]))

    time.sleep_ms(20) # 10 ms minimum

    com = "capture"
    print(com + " -> ", end="")
    res = i2c.writeto(dev, bytearray(com, "utf-8"))
    time.sleep_ms(10) # 10 ms minimum
    print(int(i2c.readfrom(dev, 1)[0]))

    time.sleep_ms(150) # 150 ms minimum

    file = open("photo.jpg", "wb")
    com = "get"
    print(com + " -> ", end="")
    rlen = 1
    while rlen > 0:
        res = i2c.writeto(dev, bytearray(com, "utf-8"))
        time.sleep_ms(180) # 150 ms minimum
        buffer = i2c.readfrom(dev, 1024 + 3)
        print(buffer[0], end="")
        rlen = int.from_bytes(buffer[1:3], 'little')
        print(" (" + str(rlen) + " bytes)")
        #print("received", buffer[3:3+rlen])
        if rlen > 0: res = file.write(buffer[3:3+rlen])
        time.sleep_ms(10) # 10 ms minimum
else:
    print("Camera not found")
```

Contribution
------------

Managed component esp32-camera slightly rewritten (to allow multiple camera init() calls without deinit()) and put to ./components folder
I2C_Target driver rewriten:
- use I2C slave internal buffers (without defining mem in I2C_Target constructor) with hard coded sizes
- add timeout parameter to I2C_Target constructor
- add bounds checking when receive or send data
- rewrite mp_machine_i2c_target_read_bytes
- ./extmod/machine_i2c_target.c not modified
Add camera pins component (select `ESP32 Camera Board` in ./configure_cam)
Add deploy component (select `Deploy` to configure copying built files to external folder)


Troubleshooting
---------------

Firmware can not be burned to ESP32-CAM module while GPIO12 pulled up or connected to something external.
Disconnect GPIO12 and try again.
Or you can disable GPIO12 boot behavior by using this [esptool command](https://docs.espressif.com/projects/esptool/en/latest/esp32/espefuse/set-flash-voltage-cmd.html)
to burn the efuse bits for 3.3V flash (VDD_SDIO) voltage selection as discribed [here](https://github.com/espressif/arduino-esp32/issues/7519).
```bash
python -m espefuse --chip auto --port COM6 set-flash-voltage 3.3V
```

Also this helps when cyclic reboot occurs:
```bash
rst:0x10 (RTCWDT_RTC_RESET),boot:0x33 (SPI_FAST_FLASH_BOOT)
invalid header: 0xffffffff
invalid header: 0xffffffff
...
```

"Camera init failed" error happens when no PSRAM available on board, or micropython compiled with no PSRAM support.

