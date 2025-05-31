from machine import UART, Pin
#import filesys
import utime

class NEVAMT:
    _CMD_INIT = [0x2F, 0x3F]
    _CMD_POST_INIT = [0x21]
    _EOL = [0x0D, 0x0A]

    # self.read485([0x2F, 0x3F, 0x21, 0x0D, 0x0A])
    # self.read485([0x6, 0x30, 0x35, 0x31, 0x0D, 0x0A])
    # self.read485([0x01, 0x50, 0x31, 0x02, 0x28, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x29, 0x03, 0x61])

    _CMD_PREFIX = [0x01, 0x52, 0x31, 0x02]
    _ETX = [0x03]
    _CMD_CLOSE = [0x1, 0x42, 0x30, 0x03]

    _GET_READ = ([0x30, 0x46, 0x30, 0x38, 0x38, 0x30, 0x46, 0x46, 0x28, 0x29])  #показания (тотал + тарифы)
    _GET_HW = ([0x36, 0x30, 0x30, 0x31, 0x30, 0x34, 0x46, 0x46, 0x28, 0x29])  #прошивка (версия)
    _GET_ID = ([0x36, 0x30, 0x30, 0x31, 0x30, 0x31, 0x46, 0x46, 0x28, 0x29])  # ID
    _GET_SN = ([0x36, 0x30, 0x30, 0x31, 0x30, 0x30, 0x46, 0x46, 0x28, 0x29])  # серийник
    _GET_V_A = ([0x32, 0x30, 0x30, 0x37, 0x30, 0x30, 0x46, 0x46, 0x28, 0x29])  # напряжение A
    _GET_V_B = ([0x33, 0x34, 0x30, 0x37, 0x30, 0x30, 0x46, 0x46, 0x28, 0x29])  # напряжение B
    _GET_V_C = ([0x34, 0x38, 0x30, 0x37, 0x30, 0x30, 0x46, 0x46, 0x28, 0x29])  # напряжение C
    _GET_PW = ([0x31, 0x30, 0x30, 0x37, 0x30, 0x30, 0x46, 0x46, 0x28, 0x29])  # мощность
    _GET_CUR_A = ([0x31, 0x46, 0x30, 0x37, 0x30, 0x30, 0x46, 0x46, 0x28, 0x29])  # ток A
    _GET_CUR_B = ([0x33, 0x33, 0x30, 0x37, 0x30, 0x30, 0x46, 0x46, 0x28, 0x29])  # ток B
    _GET_CUR_C = ([0x34, 0x37, 0x30, 0x37, 0x30, 0x30, 0x46, 0x46, 0x28, 0x29])  # ток C


    def __init__(self, address = '', rts_pin = 16, uart = 1, timeout=5, debug=0):
        self.rs485 = UART(uart, 9600, bits=7, parity=0, stop=1, timeout=100)   # add UART lib
        print(self.rs485)
        self.rts = Pin(rts_pin, Pin.OUT, 0) # add Pin lib
        bitaddress=[]
        if address != '':
            for ch in address:
                 bitaddress.append(ord(ch))
            self.address=list(bytearray(bitaddress))
        else:
            self.address = []
        self.Z = ''
        self.timeout = timeout
        self.debug = debug
        self.counter_data = {}
        self.init()

        utime.sleep(3) # sometines CE301 failed to read after init, so need a timeout

    def init(self):
        init = self.read485([0x2F, 0x3F, 0x21, 0x0D, 0x0A])
        self.read485([0x6, 0x30, 0x35, 0x31, 0x0D, 0x0A])
        self.read485([0x01, 0x50, 0x31, 0x02, 0x28, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x29, 0x03, 0x61])

        if init:
            if init.find('NEVA') >=0:
                self.logline("it's REAL NEVA: " + init)
                f_name = (init[5:]).split('.')[0]
                self.counter_data['TYPE'] = f_name[:6]
                self.counter_data['SUBTYPE'] = f_name
            elif init.find('CE') >=0:
                self.read485([0x1, 0x42, 0x30, 0x03, 0x75])
                self.logline("Not NEVA, CE")
                utime.sleep(3)



    def log(self, s):
        if self.debug:
          #filesys.log(s, end='')
          print(s, end='')
    def logline(self, s):
        if self.debug:
          #filesys.log(s)
          print(s)

    def close(self):
        cmd = self._CMD_CLOSE + self.get_bcc(self._CMD_CLOSE)
        _cmd = ''
        for bit in cmd:
            _cmd = _cmd + chr(int(bit))
        utime.sleep_ms(100)
        self.logline("_cmd: "+ _cmd)
        self.rts.value(1)
        self.rs485.write(_cmd)
        utime.sleep_ms(5)
        self.rts.value(0)
        self.rs485.close()
        return

    def getValues(self):
        return self.counter_data

    def read485(self, cmd):
        _cmd = ''
        for bit in cmd:
            _cmd = _cmd + chr(int(bit))

        utime.sleep_ms(500)
        _data = ''
        _buffer = ''
        self.logline("_cmd: "+ _cmd)
        self.rts.value(1)
        self.rs485.write(_cmd)
        utime.sleep_ms(5) # 5
        self.rts.value(0)
        for _ in range(10):
            if self.rs485.any() > 0:
                _data = self.rs485.read()

                if ('decode' in dir(_data)):
                    _data_decoded = _data.decode()
                else:
                    _data_decoded = _data
                self.logline(' -> ')
                self.logline(_data)
                self.parser(_data_decoded)
                if _data:
                    _buffer =  _buffer + _data_decoded
                return _buffer
            else:
                self.log('.')
                utime.sleep_ms(100)
        else:
            self.logline('Counter not responding!')


    def get_bcc(self, message):
        bcc = 0x00
        for i in range(1, len(message)):
            bcc = bcc ^ message[i]
        return [hex(bcc)]




    def cmd(self, cmd):
        _cmd = self._CMD_PREFIX + cmd + self._ETX
        bcc = self.get_bcc(_cmd)
        _cmd = _cmd + bcc
        res = self.read485(_cmd)
        return res


    def parser(self, _data):

        print("_data", _data)
        if ('decode' in dir(_data)):
            _data_decoded = _data.decode()
        else:
            _data_decoded = _data
        if _data == None:
            return 'NONE'

#         if _data_decoded.find('NEVA') >=0:
#             self.counter_data['TYPE'] = (_data_decoded[5:]).split('.')[0]

        if _data_decoded.find('0F0880FF') >=0:
            _idx = _data_decoded.find('0F0880FF') + len('0F0880FF')
            _total = (_data_decoded[_idx:]).split('(')[1].split(',')[0]
            _day = (_data_decoded[_idx + len(_total):]).split(',')[1].split(',')[0]
            _night = (_data_decoded[_idx + len(_total) + len(_day):]).split(',')[1].split(',')[0]
            self.counter_data['READ'] = _total + ';' + _day + ';' + _night

        if _data_decoded.find('600104FF') >=0:
            _idx = _data_decoded.find('600104FF') + len('600104FF')
            self.counter_data['HW'] = (_data_decoded[_idx:]).split('(')[1].split(')')[0]

        if _data_decoded.find('600101FF') >=0:
            _idx = _data_decoded.find('600101FF') + len('600101FF')
            self.counter_data['ID'] = (_data_decoded[_idx:]).split('(')[1].split(')')[0]

        if _data_decoded.find('600100FF') >=0:
            _idx = _data_decoded.find('600100FF') + len('600100FF')
            self.counter_data['SN'] = (_data_decoded[_idx:]).split('(')[1].split(')')[0]

        if _data_decoded.find('200700FF') >=0:
            _idx = _data_decoded.find('200700FF') + len('200700FF')
            _a = (_data_decoded[_idx:]).split('(')[1].split(')')[0]
            self.counter_data['V'] = _a + ';'
        if _data_decoded.find('340700FF') >=0:
            _idx = _data_decoded.find('340700FF') + len('340700FF')
            _b = (_data_decoded[_idx:]).split('(')[1].split(')')[0]
            self.counter_data['V'] = self.counter_data['V'] + _b + ';'
        if _data_decoded.find('480700FF') >=0:
            _idx = _data_decoded.find('480700FF') + len('480700FF')
            _c = (_data_decoded[_idx:]).split('(')[1].split(')')[0]
            self.counter_data['V'] = self.counter_data['V'] + _c + ';'

        if _data_decoded.find('100700FF') >=0:
            _idx = _data_decoded.find('100700FF') + len('100700FF')
            self.counter_data['PW'] = (_data_decoded[_idx:]).split('(')[1].split(')')[0]

        if _data_decoded.find('1F0700FF') >=0:
            _idx = _data_decoded.find('1F0700FF') + len('1F0700FF')
            _a = (_data_decoded[_idx:]).split('(')[1].split(')')[0]
            self.counter_data['CUR'] = _a + ';'

        if _data_decoded.find('330700FF') >=0:
            _idx = _data_decoded.find('330700FF') + len('330700FF')
            _b = (_data_decoded[_idx:]).split('(')[1].split(')')[0]
            self.counter_data['CUR'] = self.counter_data['CUR'] + _b + ';'
        if _data_decoded.find('470700FF') >=0:
            _idx = _data_decoded.find('470700FF') + len('470700FF')
            _c = (_data_decoded[_idx:]).split('(')[1].split(')')[0]
            self.counter_data['CUR'] = self.counter_data['CUR'] + _c + ';'


    def getAll(self):
        if self.counter_data != {}:
            self.cmd(self._GET_READ)
            self.cmd(self._GET_HW)
            self.cmd(self._GET_ID)
            self.cmd(self._GET_SN)
            self.cmd(self._GET_V_A)
            self.cmd(self._GET_V_B)
            self.cmd(self._GET_V_C)
            self.cmd(self._GET_PW)
            self.cmd(self._GET_CUR_A)
            self.cmd(self._GET_CUR_B)
            self.cmd(self._GET_CUR_C)
        self.logline(self.counter_data)
        self.close()

#c = NEVAMT(debug=1)
#print(c.counter_data)

# import machine
# import time
# p = machine.Pin(16, machine.Pin.OUT, 0)
# u = machine.UART(1, 115200)
# p.on()
# u.write("ABCDEF")
# time.sleep_ms(5)
# p.off()

