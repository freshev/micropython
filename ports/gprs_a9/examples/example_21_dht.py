# Micropython a9g example
# Source: https://github.com/freshev/micropython
# Author: freshev
# Demonstrates how to retrieve temperature from different DHT sensors

import dht
import time

d1 = dht.dht(30,type=dht.DHT21) # use external pull-up resistor for this sensor!
for i in range(0,100):
    d1.read_temperature(0,0)
    time.sleep(1)

d2 = dht.dht(20,type=dht.DHT11)
for i in range(0,100):
    d2.read_temperature(0,0)
    time.sleep(1)
