import os
import sys
import filesys
import json
import machine
import time
import cellular

def gprs_on(led=None, debug=0):
    res = 0    
    stat = cellular.get_network_status(); filesys.log('NWS: %s' % hex(stat))
    #if debug: filesys.log('NWS: %s, Registered = %i, InProress = %i, Attached = %i, Active = %i' % (hex(stat), ((stat & 0x01) >> 0), ((stat & 0x04) >> 2), ((stat & 0x08) >> 3), ((stat & 0x10) >> 4)))
    flag = 0
    while(cellular.get_network_status() == 0):
        if led != None: led.value(not led.value())
        filesys.log('*', '')
        time.sleep_ms(250)
        flag = 1
    if flag == 1: filesys.log('')
    if debug: filesys.log('GPRS ', '')
    if(not cellular.gprs()):
        cellular.gprs('internet', '', '')
        if(cellular.gprs()): 
            res=1; 
            if debug: filesys.log('ok')
        else: 
            if debug: filesys.log('nok')
    else: 
        res = 1
        if debug: filesys.log('ok')
    stat = cellular.get_network_status(); filesys.log('NWS: %s' % hex(stat))
    #if debug: filesys.log('NWS: %s, Registered = %i, InProress = %i, Attached = %i, Active = %i' % (hex(stat), ((stat & 0x01) >> 0), ((stat & 0x04) >> 2), ((stat & 0x08) >> 3), ((stat & 0x10) >> 4)))
    return res

def gprs_off(debug=0):
    cellular.gprs(0)
    if debug: filesys.log('GPRS off')
    filesys.log('GPRS off')
    stat = cellular.get_network_status(); filesys.log('NWS: %s' % hex(stat))

def watchdog(led = None):    
    if led != None: led.value(1)
    machine.WDT(0,-1).feed()
    time.sleep_ms(20)
    if led != None: led.value(0)

class Settings:

    def __init__(self, debug = 0):
        self.debug = debug
        self.file = 'settings.txt'

    def log(self, s):
        if self.debug: filesys.log(s, end = '')
    def logline(self, s):
        if self.debug: filesys.log(s)

    def rf(self, file):
        sett = self.read()
        if int(sett['OTA']) == 0: self.write({'OTA': '1'})
        try: os.remove(file + '.pyo')
        except: pass
        try: os.remove(file + '.py')
        except: pass
        machine.reset()

    def check_key(self, var, key): # check key existance
        return (('has_key' in dir(var)) and var.has_key(key)) or (('has_key' not in dir(var)) and key in var)
    def check_key_nv(self, var, key, notvalue): # check key existance and value != notvalue
        return ((('has_key' in dir(var)) and var.has_key(key)) or (('has_key' not in dir(var)) and key in var)) and var[key] != notvalue

    def read(self):
        is_settings_file = 0
        for i in os.listdir():
            if i == self.file:
                is_settings_file = 1
                break

        if not is_settings_file:
            filesys.log('Gen BC')
            j = json.loads('{"snum":"default","sname":"default","ssub":"default","OTA":"1","DEBUG":"1","BC":""}')
            j['BC'] = self.generate()
            filesys.log('New %s' % self.file)
            f = filesys.open(self.file, 'w')
            f.write(json.dumps(j))
            f.close()

        f = filesys.open(self.file, 'r')
        r = f.read()
        f.close()
        j = json.loads(r)        
        try: 
            checkstr = 'Counter: '+ j['sname'] + '_' + j['snum'] + ', sub:' + j['ssub'] + ', OTA:' + j['OTA'] + ', DEBUG:' + j['DEBUG'] + ', BC:' + j['BC']
            if self.debug: self.logline(checkstr)
        except:
            try: os.remove(self.file)
            except: pass
            return self.read()
        return j

    def write(self, data):
        if self.debug: self.log('Update ')
        try:
            f = filesys.open(self.file, 'r')
            r = f.read()
            j = json.loads(r)
            f.close()

            for key in data.keys():
                if self.check_key(j, key): j[key] = data[key]
            f = filesys.open(self.file, 'w')
            d = json.dumps(j)
            f.write(d)
            f.close()            
            if self.debug: self.logline('ok %s' % data)
        except:
            type, ex, tb = sys.exc_info()
            if self.debug: self.logline('nok (%s)' % str(ex))

    def random(self, maximum = 16):
        a0 = machine.ADC(0)
        a1 = machine.ADC(1)
        _a0 = a0.read_u16() + os.urandom(1)[0]
        time.sleep_ms(100)
        _a1 = a1.read_u16() + os.urandom(1)[0]
        return ((_a0 + _a1) * _a0 * _a1 - _a1) % maximum

    def generate(self, length=16):
        a0 = machine.ADC(0)
        a1 = machine.ADC(1)
        source = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890'
        gen = ''
        for _ in range(length):
            _a0 = a0.read_u16() + os.urandom(1)[0]
            time.sleep_ms(100)
            _a1 = a1.read_u16() + os.urandom(1)[0]
            time.sleep_ms(100)
            x = ((_a0 + _a1) * _a0 * _a1 - _a1) % len(source)
            gen = gen + source[x]
        return gen

