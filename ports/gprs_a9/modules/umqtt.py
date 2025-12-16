import sys
import socket
import time

class MQTTClient:
    _mqtt_pid=1

    def pid_gen(self):
        if MQTTClient._mqtt_pid < 65535: MQTTClient._mqtt_pid = MQTTClient._mqtt_pid + 1
        else: MQTTClient._mqtt_pid = 1
        return MQTTClient._mqtt_pid

    def __init__(self, client_id, server, port=1883, user=None, password=None, keepalive=0,
                 ssl=0, ssl_params={}, socket_timeout=30, message_timeout=10):
        self.client_id = client_id
        self.sock = None
        self.server = server
        self.port = port
        self.ssl = ssl

        # do not uncomment !!!!!
        #t = self.ssl_params

        self.newpid = self.pid_gen()
        #if not getattr(self, 'cb', None): self.cb = None
        #if not getattr(self, 'cbstat', None): self.cbstat = lambda p, s: None
        self.user = user
        self.pswd = password
        self.keepalive = keepalive
        self.lw_topic = None
        self.lw_msg = None
        self.lw_qos = 0
        self.lw_retain = 0
        self.rcv_pids = {}
        #self.last_ping = time.ticks_ms()
        #self.last_cpacket = time.ticks_ms()
        self.socket_timeout = socket_timeout
        self.message_timeout = message_timeout

    def int_to_string(self, val, count, encode):
        ret = []
        for _ in range(0, count): ret.append(0)
        #if(encode.lower() == 'big'):
        #    c = count - 1
        #    divide = val / 256
        #    remain = val % 256
        #    while divide > 0 and c > 0:
        #        ret[c] = int(remain)
        #        val = (val - remain) / 256
        #        divide = val / 256
        #        remain = val % 256
        #        c = c - 1            
        #elif(encode.lower() == 'little'):
        #    c = 0
        #    divide = val / 256
        #    remain = val % 256
        #    while divide > 0 and c < count - 1:
        #        ret[c] = int(remain)
        #        val = (val - remain) / 256
        #        divide = val / 256
        #        remain = val % 256
        #        c = c + 1
        #else: raise ValueError(encode)
        #ret[c] = int(remain)
        #return self.array_to_bytes(ret)
        c = count - 1
        divide = val / 256
        remain = val % 256
        while divide > 0 and c > 0:
            ret[c] = int(remain)
            val = (val - remain) / 256
            divide = val / 256
            remain = val % 256
            c = c - 1
        ret[c] = int(remain)
        return self.array_to_bytes(ret)


    def array_to_bytes(self, array):
        if('encode' in dir('')): return bytes(array) #python >= 2.7
        else: return ''.join(map(chr, array)) #python <= 1.5.2

    def _read(self, n):
        if n < 0: raise Exception('M2')
        msg = ''
        cur_timeout = 0
        while len(msg) < n:
            try:
                rbytes = ''.join(map(chr, self.sock.read(n - len(msg))))
            except OSError:
                type, e, traceback = sys.exc_info()
                if e.args[0] == 11: rbytes = ''
                else: raise
            except AttributeError: raise Exception('M8')
            if rbytes == '':
                cur_timeout = cur_timeout + 100
                if(cur_timeout < self.socket_timeout * 1000):
                    time.sleep_ms(100)
                    continue
                else: raise Exception('M1')
            if rbytes == '': raise Exception('M1')
            else: msg = msg + rbytes
        return msg

    def _write(self, bytes_wr, length=-1):
        try:
            wr_len = length
            if(wr_len) < 0: wr_len = len(bytes_wr)
            out = self.sock.write(bytes_wr, wr_len)
        except AttributeError: raise Exception('M8')

        if length < 0:
            if out != len(bytes_wr): raise Exception('M3')
        else:
            if out != length: raise Exception('M3')
        return out

    def _send_str(self, s):
        #assert len(s) < 65536
        self._write(self.int_to_string(len(s), 2, 'big'))
        self._write(s)

    def _recv_len(self):
        n = 0
        sh = 0
        while 1:
            b = ord(self._read(1)[0])
            n = n | ((b & 0x7f) << sh)
            if not b & 0x80: return n
            sh = sh + 7

    def _varlen_encode(self, value, buf, offset=0):
        #assert value < 268435456
        while value > 0x7f:
            buf[offset] = (value & 0x7f) | 0x80
            value = value >> 7
            offset = offset + 1
        buf[offset] = value
        return offset + 1

    #def set_callback(self, f):
    #    self.cb = f
    #def set_callback_status(self, f):
    #    self.cbstat = f
    #def set_last_will(self, topic, msg, retain=0, qos=0):
    #    #assert 0 <= qos <= 2
    #    #assert topic
    #    self.lw_topic = topic
    #    self.lw_msg = msg
    #    self.lw_qos = qos
    #    self.lw_retain = retain

    def connect(self, clean_session=1):
        self.disconnect()
        ai = socket.getaddrinfo(self.server, self.port)[0]
        self.sock_raw = socket.socket(ai[0], ai[1], ai[2])
        try:
            self.sock_raw.connect(ai[-1])
        except OSError:
            type, e, traceback = sys.exc_info()
            raise RuntimeError(e)
        if self.ssl:
            import tls
            # do not uncomment !!!!!
            #self.sock = ssl.wrap_socket(self.sock_raw, key=self.ssl_params['key'], cert=self.ssl_params['cert'],
            #            server_side=self.ssl_params['server_side'], server_hostname=self.ssl_params['server_hostname'],
            #            do_handshake=self.ssl_params['do_handshake'])

            #self.sock = ssl.wrap_socket(self.sock_raw, server_hostname=self.server)
            ctx = tls.SSLContext(tls.PROTOCOL_TLS_CLIENT)
            ctx.verify_mode = tls.CERT_NONE
            self.sock = ctx.wrap_socket(self.sock_raw, server_hostname=self.server)

            self.sock_raw.setblocking(1)
        else: self.sock = self.sock_raw

        premsg = [0x10, 0, 0, 0, 0, 0]
        msg = [0, 4, 77, 81, 84, 84, 4, 0, 0, 0]
        sz = 10 + 2 + len(self.client_id)
        msg[7] = clean_session << 1
        if clean_session == 1:
            self.rcv_pids.clear()
        if self.user is not None:
            sz = sz + 2 + len(self.user)
            msg[7] = msg[7] | (1 << 7)
            if self.pswd is not None:
                sz = sz + 2 + len(self.pswd)
                msg[7] = msg[7] | (1 << 6)
        if self.keepalive:
            #assert self.keepalive < 65536
            msg[8] = msg[8] | (self.keepalive >> 8)
            msg[9] = msg[9] | (self.keepalive & 0x00FF)
        if self.lw_topic:
            sz = sz + 2 + len(self.lw_topic) + 2 + len(self.lw_msg)
            msg[7] = msg[7] | (0x4 | (self.lw_qos & 0x1) << 3 | (self.lw_qos & 0x2) << 3)
            msg[7] = msg[7] | (self.lw_retain << 5)
        plen = self._varlen_encode(sz, premsg, 1)
        self._write(self.array_to_bytes(premsg), plen)
        self._write(self.array_to_bytes(msg))
        self._send_str(self.client_id)
        if self.lw_topic:
            self._send_str(self.lw_topic)
            self._send_str(self.lw_msg)
        if self.user is not None:
            self._send_str(self.user)
            if self.pswd is not None:
                self._send_str(self.pswd)
        resp = list(self._read(4))
        if not (ord(resp[0]) == 0x20 and ord(resp[1]) == 0x02): raise Exception('M29')
        if ord(resp[3]) != 0: raise Exception('M' + str(20 + ord(resp[3])))
        #self.last_cpacket = time.ticks_ms()
        return ord(resp[2]) & 1

    def disconnect(self):
        if not self.sock: return
        try: self._write(self.array_to_bytes([0xE0,0]))
        except: pass
        try: self.sock.close()
        except: pass
        self.sock = None

    #def ping(self):
    #    self._write(self.array_to_bytes([0xC0,0]))
    #    self.last_ping = time.ticks_ms()

    def publish(self, topic, msg, retain=0, qos=0, dup=0): # qos always 0 
        #assert qos in (0, 1)
        pkt = [0x30, 0, 0, 0, 0]
        pkt[0] = pkt[0] | (qos << 1 | retain | int(dup) << 3)
        sz = 2 + len(topic) + len(msg)
        if qos > 0: sz = sz + 2
        plen = self._varlen_encode(sz, pkt, 1)
        self._write(self.array_to_bytes(pkt), plen)
        self._send_str(topic)
        #if qos > 0:
        #    pid = self.pid_gen() #next(self.newpid)
        #    self._write(self.int_to_string(pid, 2, 'big'))
        self._write(msg)
        #if qos > 0:
        #    self.rcv_pids[pid] = time.ticks_add(time.ticks_ms(), self.message_timeout * 1000)
        #    return pid

    #def subscribe(self, topic, qos=0):
    #    #assert qos in (0, 1)
    #    #assert self.cb is not None, 'Subscribe callback is not set'
    #    pkt = [0x82, 0, 0, 0, 0, 0, 0]
    #    pid = self.pid_gen() #next(self.newpid)
    #    sz = 2 + 2 + len(topic) + 1
    #    plen = self._varlen_encode(sz, pkt, 1)
    #    pids = self.int_to_string(pid, 2, 'big')
    #    pkt[plen] = pids[0]
    #    pkt[plen + 1] = pids[1]
    #    self._write(self.array_to_bytes(pkt), plen + 2)
    #    self._send_str(topic)
    #    #self._write(self.int_to_string(qos, 1, 'little'))
    #    self._write(self.int_to_string(qos, 1, 'big'))
    #    self.rcv_pids[pid] = time.ticks_add(time.ticks_ms(), self.message_timeout * 1000)
    #    return pid

    #def _message_timeout(self):
    #    curr_tick = time.ticks_ms()
    #    for pid, timeout in self.rcv_pids.items():
    #        if time.ticks_diff(timeout, curr_tick) <= 0:
    #            self.rcv_pids.pop(pid)
    #            self.cbstat(pid, 0)
    
    #def check_msg(self):
    #    if self.sock:
    #        try:
    #            res = chr(self.sock.read(1))
    #            if res is None:
    #                self._message_timeout()
    #                return None
    #        except OSError:
    #            self._message_timeout()
    #            return None
    #    else: raise Exception('M28')
    #
    #    #res = list(res)
    #    if res == '': raise Exception('M1')
    #
    #    if res == '\320':
    #        if self._read(1)[0] != 0: raise Exception('M-1')
    #        self.last_cpacket = time.ticks_ms()
    #        return
    #
    #    op = ord(res[0])
    #    if op == 0x40: # PUBACK
    #        sz = list(self._read(1))
    #        if sz != [0x02]: raise Exception('M-1')
    #        rcv_pid = int.from_bytes(self._read(2), 'big')
    #        if rcv_pid in self.rcv_pids:
    #            self.last_cpacket = time.ticks_ms()
    #            self.rcv_pids.pop(rcv_pid)
    #            self.cbstat(rcv_pid, 1)
    #        else:
    #            self.cbstat(rcv_pid, 2)
    #
    #    if op == 0x90: #SUBACK
    #        resp = list(self._read(4))
    #        if ord(resp[0]) != 0x03: raise Exception('M40') #(40, resp)
    #        if ord(resp[3]) == 0x80: raise Exception('M44') # 
    #        if ord(resp[3]) not in (0, 1, 2): raise Exception('M40') # (40, resp)
    #        pid = ord(resp[2]) | (ord(resp[1]) << 8)
    #        if pid in self.rcv_pids:
    #            self.last_cpacket = time.ticks_ms()
    #            self.rcv_pids.pop(pid)
    #            self.cbstat(pid, 1)
    #        else:
    #            raise Exception('M5')
    #
    #    self._message_timeout()
    #
    #    if op & 0xf0 != 0x30: #PUBLISH
    #        return op
    #    sz = self._recv_len()
    #    topic_len = int.from_bytes(self._read(2), 'big')
    #    topic = self._read(topic_len)
    #    sz = sz - (topic_len + 2)
    #    if op & 6:
    #        pid = int.from_bytes(self._read(2), 'big')
    #        sz = sz - 2
    #    if sz: msg = self._read(sz)
    #    else: msg = ''
    #    retained = op & 0x01
    #    dup = op & 0x08
    #    self.cb(topic, msg, retained, dup)
    #    self.last_cpacket = time.ticks_ms()
    #    if op & 6 == 2:
    #        self._write(self.array_to_bytes([0x40, 0x02])) #PUBACK
    #        self._write(self.int_to_string(pid, 2, 'big'))
    #    elif op & 6 == 4: raise NotImplementedError()
    #    elif op & 6 == 6: raise Exception('M-1')
    
    #def wait_msg(self):
    #    st_old = self.socket_timeout
    #    self.socket_timeout = None
    #    out = self.check_msg()
    #    self.socket_timeout = st_old
    #    return out
