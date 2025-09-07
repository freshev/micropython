import sys
import socket
import tls

def a2s(array):
        if('encode' in dir('')): return array.decode('utf-8') #python >= 2.7
        else: return array #python <= 1.5.2

class Response:
    def __init__(self, f):
        self.raw = f
        self._cached = None

    def close(self):
        if self.raw:
            self.raw.close()
            self.raw = None
        self._cached = None

    def _getcontent(self):
        if self._cached is None:
            try:
                self._cached = self.raw.read()
            finally:
                self.raw.close()
                self.raw = None
        return self._cached

    def _gettext(self):
        return str(self.content)

    def __getattr__(self, attr):
        if attr == 'text': return self._gettext()
        elif attr == 'content': return self._getcontent()
        else: return self.__dict__[attr]

def request(method, url, data=None, headers={}, timeout=None):
    redirect = None
    chunked_data = data and getattr(data, '__iter__', None) and not getattr(data, '__len__', None)

    try:
        proto, dummy, host, path = url.split('/', 3)
    except ValueError:
        proto, dummy, host = url.split('/', 2)
        path = ''
    if proto == 'http:': port = 80
    elif proto == 'https:': port = 443
    else: raise ValueError('Unsupported protocol: ' + proto)

    if ':' in host:
        host, port = host.split(':', 1)
        port = int(port)

    ai = socket.getaddrinfo(host, port, 0, 1) # socket.SOCK_STREAM = 1
    ai = ai[0]

    resp_d = {}
    s = socket.socket(ai[0], 1, ai[2]) # socket.SOCK_STREAM
    if timeout is not None: s.settimeout(timeout)

    try:
        s.connect(ai[-1])

        if proto == 'https:': 
            #s = ssl.wrap_socket(s, server_hostname=host)
            ctx = tls.SSLContext(tls.PROTOCOL_TLS_CLIENT)
            ctx.verify_mode = tls.CERT_NONE
            s = ctx.wrap_socket(s, server_hostname=host)
        s.write('%s /%s HTTP/1.0\r\n' % (method, path))
        s.write('Host: %s\r\n' % host)
        for k in headers:
            s.write(k)
            s.write(b": ")
            s.write(headers[k])
            s.write(b"\r\n")
        if data:
            if chunked_data: s.write('Transfer-Encoding: chunked\r\n')
            else: s.write('Content-Length: %d\r\n' % len(data))
        s.write('Connection: close\r\n\r\n')
        if data:
            if chunked_data:
                for chunk in data:
                    s.write('%x\r\n' % len(chunk))
                    s.write(chunk)
                    s.write('\r\n')
                s.write('0\r\n\r\n')
            else: s.write(data)

        l = a2s(s.readline())
        #l = ''.join(map(chr, s.readline()))
        l = l.split(None, 2)
        if len(l) < 2: raise ValueError('HTTP error: BadStatusLine:\n%s' % l)
        status = int(l[1])
        reason = ''
        if len(l) > 2: reason = l[2].rstrip()
        while 1:
            l = a2s(s.readline())
            #l = ''.join(map(chr, s.readline()))
            llist = list(l)
            if len(llist) == 0: break
            if len(llist) == 2 and ord(llist[0]) == 0x0D and ord(llist[1]) == 0x0A: break

            if l.startswith('Transfer-Encoding:'):
                if l.find('chunked') >= 0:
                    raise ValueError('Unsupported ' + l)
            elif l.startswith('Location:') and not 200 <= status <= 299:
                if status in [301, 302, 303, 307, 308]:
                    redirect = l[10:-2]
                else:
                    raise NotImplementedError('Redirect %d not yet supported' % status)
            if(l.find(':') > 0):
              k, v = l.split(':', 1)
              resp_d[k] = v.strip()
    except:
        s.close()
        raise

    if redirect:
        s.close()
        if status in [301, 302, 303]:
            return request('GET', redirect, data, timeout)
        else:
            return request(method, redirect, data, timeout)
    else:
        resp = Response(s)
        resp.status_code = status
        resp.reason = reason
        resp.headers = resp_d
        return resp

def get(url, data=None, headers={}, timeout=None):
    return request('GET', url, data, headers, timeout)
def head(url, data=None, headers={}, timeout=None):
    return request('HEAD', url, data, headers, timeout)
def post(url, data=None, headers={}, timeout=None):
    return request('POST', url, data, headers, timeout)
def put(url, data=None, headers={}, timeout=None):
    return request('PUT', url, data, headers, timeout)
def patch(url, data=None, headers={}, timeout=None):
    return request('PATCH', url, data, headers, timeout)
def delete(url, data=None, headers={}, timeout=None):
    return request('DELETE', url, data, headers, timeout)
