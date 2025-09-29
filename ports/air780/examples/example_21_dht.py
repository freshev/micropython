# Micropython Air780e example
# Source: https://github.com/freshev/micropython
# Author: freshev
# Demonstrates how to retrieve temperature from different DHT sensors
# Use sensor Pin with PULLUP resistor 5-10 kOhm
# Tested with 3.3V/5V for sensor's VDD/PULLUP and GPIO9/GPIO11

import dht

d1 = dht.dht(11, type=dht.DHT11)
d1.read_temperature(0,0)
