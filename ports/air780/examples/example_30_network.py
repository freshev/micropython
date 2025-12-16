# Micropython Air780e example
# Source: https://github.com/freshev/micropython
# Author: pulkin
# Modified: freshev
# Demonstrates how to use TCP sockets
import cellular
import socket

cellular.gprs("internet", "", "")
host = "www.myip.com"
port = 80
s = socket.socket()
s.connect((host, port))
message = "GET /200 HTTP/1.1\r\nHost: {}\r\nConnection: close\r\n\r\n"
s.write(message.format(host))
print(s.read(256))
s.close()
cellular.gprs(False)

