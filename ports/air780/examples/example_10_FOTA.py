# Micropython Air780e example
# Source: https://github.com/freshev/micropython
# Modified: freshev
# Demonstrates how to use firmware over the air interface
import os
import machine

print(os.uname().release)
machine.OTA('v1.2')
