# Micropython esp32 example
# Source: https://github.com/freshev/micropython
# Author: freshev
# Demonstrates I2C interface in slave mode

# -----------------------------
# Slave device (ESP32s3) script
# -----------------------------
import time
from machine import I2CTarget, Pin

mem = bytearray(32)
def irq_handler(i2c_target):
    flags = i2c_target.irq().flags()
    if flags & I2CTarget.IRQ_END_READ:
        print("controller read target at addr", i2c_target.memaddr)        
    if flags & I2CTarget.IRQ_END_WRITE:
        print("controller wrote target at addr", i2c_target.memaddr)
        print("received = ", mem)
        for i in range(len(mem)): mem[i] = 0xFF

uplink = I2CTarget(scl = 12, sda = 13, addr=44, mem_addrsize=0, mem=mem)
uplink.irq(irq_handler)
print(uplink)


# -----------------------------
# Master device (A9/A9G) script
# -----------------------------

# from machine import I2C
# import time

# i2c = I2C(2, freq = 100000)
# devs = i2c.scan()
# if len(devs) > 0:
#     dev = devs[0]
#     print("found dev =", dev)
#     com = "ping"
#     i2c.writeto(dev, bytearray(com, "utf-8"))
#     time.sleep_ms(100)
#     print("receive", i2c.readfrom(dev, 1))

