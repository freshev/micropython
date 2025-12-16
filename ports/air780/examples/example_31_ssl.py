# Micropython Air780e example
# Source: https://github.com/freshev/micropython
# Author: pulkin
# Modified: freshev
# Demonstrates how to wrap sockets into ssl tunnel
import cellular
import socket
import ssl
cellular.gprs("internet", "", "")
print("IP", cellular.get_ipv4())
host = "www.myip.com"
port = 443
s = socket.socket()
s.connect((host, port))
s = ssl.wrap_socket(s)
message = "GET / HTTP/1.1\r\nHost: {}\r\nConnection: close\r\n\r\n"
s.write(message.format(host))
print(s.read(256))
s.close()
cellular.gprs(False)

