# Micropython Air780eg example
# Source: https://github.com/freshev/micropython
# Author: pulkin
# Demonstrates how to retrieve GPS coordinates from the built-in GPS module

import gps
gps.on()
print("Location", gps.get_location())
print("Satellites (tracked, visible)", gps.get_satellites())
gps.off()

