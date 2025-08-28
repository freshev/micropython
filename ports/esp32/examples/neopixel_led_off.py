from machine import Pin
from neopixel import NeoPixel

pin = Pin(48, Pin.OUT)   # set GPIO48  to output to drive NeoPixel
neo = NeoPixel(pin, 1)   # create NeoPixel driver on GPIO48 for 1 pixel
neo[0] = (0, 0, 0) # set the first pixel to white
neo.write()              # write data to all pixels
r, g, b = neo[0]         # get first pixel colour
print(r, g, b)