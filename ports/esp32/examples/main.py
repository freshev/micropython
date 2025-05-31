import uos
import camera
import time
import machine
from machine import I2C, Pin

try:
    camera.init(0, format = camera.JPEG)
except:
    camera.deinit()
    try:
        camera.init(0, format = camera.JPEG)
    except:
        print("Camera init failed. Resetting...")
        time.sleep(5)
        machine.reset()

flash = Pin(4, Pin.OUT, 0)
led = Pin(33, Pin.OUT, 1)

num = 10
for i in range (0, num):
    led.off();
    time.sleep_ms(200)
    led.on();
    if (i < num - 1): time.sleep_ms(200)

flash.on()
flash.off()

uplink = I2C(scl = 12, sda = 13, mode = I2C.SLAVE)
uplink.callback(lambda res:print(res.getcbdata()))
print(uplink)
