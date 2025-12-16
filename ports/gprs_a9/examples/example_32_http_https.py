# Micropython a9g example
# Source: https://github.com/freshev/micropython
# Author: freshev
# Demonstrates how to communicate via http using requests library

import cellular
import socket

cellular.gprs("internet", "", "")
print("IP", cellular.get_ipv4())

import urequests
r = urequests.get("http://httpbin.org/ip")
print(r.status_code, r.text)

r = urequests.get("https://httpbin.org/ip")
print(r.status_code, r.text)
