# Micropython Air780e example
# Source: https://github.com/freshev/micropython
# Author: pulkin
# Modified: freshev
# Demonstrates how to communicate via http using requests library

# Get online
import cellular
cellular.gprs("internet", "", "")

# Import requests (download if necessary)
import urequests
r = urequests.get("https://httpstat.us/200")
print(r.status_code, r.text)

