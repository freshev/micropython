import uos
uos.remove("main.py")

import camera
from machine import I2C, Pin

camera.init(0, format = camera.JPEG)
flash = Pin(4, Pin.OUT, 0)

uplink = I2C(scl = 12, sda = 13, mode = I2C.SLAVE)
uplink.callback(lambda res:print(res.getcbdata()))
print(uplink)
