# Micropython a9g example
# Source: https://github.com/freshev/micropython
# Author: freshev
# Demonstrates how to wrap sockets into ssl tunnel

import cellular
import socket
import tls

cellular.gprs("internet", "", "")
print("IP", cellular.get_ipv4())

url = "https://httpbin.org/"
proto, dummy, host, path = url.split('/', 3)

if proto == 'http:': port = 80
elif proto == 'https:': port = 443
else: raise ValueError('Unsupported protocol: ' + proto)

if ':' in host:
    host, port = host.split(':', 1)
    port = int(port)

ai = socket.getaddrinfo(host, port, 0, 1)
ai = ai[0]

s = socket.socket(ai[0], 1, ai[2])
s.connect((host, port))

ctx = tls.SSLContext(tls.PROTOCOL_TLS_CLIENT)
ctx.verify_mode = tls.CERT_NONE
s = ctx.wrap_socket(s)

message = "GET /ip HTTP/1.1\r\nHost: {}\r\nConnection: close\r\n\r\n"
s.write(message.format(host))
print(s.read())
s.close()
cellular.gprs(False)
