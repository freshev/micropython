# Micropython a9g example
# Source: https://github.com/freshev/micropython
# Author: pulkin
# Demonstrates how to communicate via http using requests library

# Get online
import cellular
cellular.gprs("internet", "", "")

# Import requests (download if necessary)
try:
    import urequests
except ImportError:
    import upip
    upip.install("micropython-urequests")
    import urequests

r = urequests.get("http://httpstat.us/200")
print(r.status_code, r.text)

