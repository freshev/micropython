# Micropython a9g example
# Source: https://github.com/freshev/micropython
# Author: pulkin
# Demonstrates how to perform USSD request
import cellular

print(cellular.ussd("*149#"))
