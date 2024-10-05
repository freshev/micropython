# Micropython a9g example
# Source: https://github.com/freshev/micropython
# Author: pulkin
# Modified: freshev
# Demonstrates how to use hardware watchdog
import machine
import time

wdt = machine.WDT(0,10000) # timeout in milliseconds
# Once on, watchdog expects resetting every 1 second in this case
time.sleep(5)
wdt.feed()
time.sleep(8)
print("Still online")
# Otherwise, it hard-resets
time.sleep(5)
print("This never prints")

