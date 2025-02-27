# Micropython Air780e example
# Source: https://github.com/freshev/micropython
# Author: pulkin
# Modified: freshev
# Demonstrates how to blink the built-in LED on the pudding board
import machine
import time

# Built-in blue LED on the pudding board
led = machine.Pin(27, machine.Pin.OUT, 0)
value = 1
for i in range(4):
    led.value(value)
    time.sleep(1)
    value = 0 if (value==1) else 1

