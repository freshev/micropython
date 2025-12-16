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
                utime.sleep(5)
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

