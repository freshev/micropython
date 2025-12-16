# Micropython a9g example
# Source: https://github.com/freshev/micropython
# Author: pulkin
# modified freshev
# Demonstrates how to use TCP sockets

import cellular
import socket

cellular.gprs("internet", "", "")
print("IP", cellular.get_ipv4())

host = "httpbin.org"
port = 80
s = socket.socket()

s.connect((host, port))
message = "GET /ip HTTP/1.1\r\nHost: {}\r\nConnection: close\r\n\r\n"
s.write(message.format(host))
print(s.read())
s.close()

cellular.gprs(False)

