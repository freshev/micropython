import machine
import time
import select
import json
import gc
import cellular

# ----------------------------------------------
# Attention: configure with "REPL port" = UART1
# ----------------------------------------------

uart_port = 0x20 # Virtual UART (USB) = 0x20
uart_debug_port = 1  # HW UART1 = 1
uart_speed = 115200
debug_uart_speed = 115200
RESET = True
DEBUG = True
data = False
phone = ""

# server commands should be in json format:
# {"command": "ping"} -> {"response" : "success"} or {"response" : "failed"}
# {"command": "send", "phone" : "+7XXXXXXXXXX", "message": "test"} -> {"response" : "success"}, {"response" : "failed"}, {"response" : "toolong"} or {"response" : "balance"}
# {"command": "receive"} -> {"response": "success", "list":[("from": "+7XXXXXXXXXX", "message": "test response", "ts":(2024, 12, 31, 23, 59, 59, 3)),("from": "+7...", "message": "...", "ts":(2024, 12, 31, 23, 59, 59, 3))]}

def debug(s):
    if DEBUG:
        t = time.localtime()
        print('{0:02d}'.format(t[2]) + '-' + '{0:02d}'.format(t[1]) + '-' + '{0:04d}'.format(t[0]) + ' ' +
              '{0:02d}'.format(t[3]) + ':' + '{0:02d}'.format(t[4]) + ':' + '{0:02d}'.format(t[5]) + ' ' + s + '\r')

def callback(sms):
    global data, phone
    if sms.phone_number == phone and (sms.message == 'Done' or sms.message == 'Status report: 0'):
        data = True
    print("%s -> %s" % (sms.phone_number, sms.message))

cellular.on_sms(callback)
class server:
    def response(self, result):
        res = 'failed'
        if result == 1: res = 'success'
        if result == 2: res = 'partially'
        if result == 3: res = 'toolong'
        if result == 4: res = 'balance'
        resp = {'response': res}
        return resp

    def run(self):
        global data, phone
        debug('Server started')

        uart = machine.UART(uart_port)
        uart.init(baudrate=uart_speed, bits=8, parity=None, stop=1) #rxbuf MUST be set ? #, rxbuf=1024

        poller = select.poll()
        poller.register(uart, select.POLLIN)

        try:
            while(True):
                poll = poller.poll(0)
                if(len(poll) > 0):
                    if isinstance(poll[0][0], type(uart)):
                        inbytes = uart.read()
                        processed = 0
                        if len(inbytes) > 0:
                          try:
                              com = json.loads(inbytes)
                              print('Command:', com)
                              if 'command' in com:
                                  if com['command'] == 'reset':
                                      resp = self.response(1)
                                      print(json.dumps(resp))
                                      uart.write(json.dumps(resp))
                                      time.sleep(1)
                                      machine.reset()
                                      processed = 1
                                  if com['command'] == 'ping':
                                      resp = self.response(1)
                                      print(json.dumps(resp))
                                      uart.write(json.dumps(resp))
                                      processed = 1
                                  if com['command'] == 'send' and 'phone' in com and 'message' in com:
                                      if len(com['message']) <= 70:
                                          cellular.sms_read_all()
                                          cellular.sms_delete_all_read()
                                          data = False
                                          phone = com['phone']
                                          sms = cellular.SMS(com['phone'], com['message'])
                                          resp = self.response(0)
                                          res = sms.send()
                                          if res == 1:
                                              # wait for feedback SMS with 'Done' from far end
                                              resp = self.response(1)
                                              counter = 0
                                              while counter < 200: # 20 seconds
                                                  #print(data)
                                                  if data == True: break
                                                  time.sleep_ms(100)
                                                  counter = counter + 1
                                              if data == True:
                                                  list = cellular.sms_list()
                                                  retlist = []
                                                  for mess in list:
                                                      if mess.phone_number == phone and (mess.message == 'Done' or mess.message == 'Status report: 0'): # message from far end
                                                          retlist.append({ 'phone': mess.phone_number, 'message': mess.message, 'dcs': mess.dcs})
                                                  resp['list'] = retlist
                                                  uart.write(json.dumps(resp))
                                              else:
                                                  resp = self.response(2)  # partially
                                                  uart.write(json.dumps(resp))
                                          else:
                                              if(res == 2):
                                                  resp = self.response(4) # check balance
                                                  uart.write(json.dumps(resp))
                                              else:
                                                  resp = self.response(0)
                                                  uart.write(json.dumps(resp))
                                      else:
                                          resp = self.response(3)  # message too long
                                          uart.write(json.dumps(resp))
                                      print(json.dumps(resp))
                                      processed = 1
                                  if com['command'] == 'receive':
                                      resp = self.response(1)
                                      list = cellular.sms_list_read()
                                      retlist = []
                                      for mess in list: retlist.append({ 'phone': mess.phone_number, 'message': mess.message, 'dcs': mess.dcs})
                                      resp['list'] = retlist
                                      print(json.dumps(resp))
                                      uart.write(json.dumps(resp))
                                      processed = 1
                          except: pass
                        if processed == 1:
                            debug('"' + com['command'] + '" processed, mem: ' + str(gc.mem_free()))
                        else:
                            debug('Error in command: %s' %  str(inbytes))
                            uart.write(json.dumps(self.response(0)))
                time.sleep_ms(1)

        except KeyboardInterrupt:
            debug('Ctrl-C pressed. Exiting')
            raise KeyboardInterrupt
        except Exception:
            debug('Something went wrong')
            pass
        finally:
            poller.unregister(uart)


machine.WDT(0, 120000)
uart_debug = machine.UART(uart_debug_port)
uart_debug.init(baudrate=debug_uart_speed, bits=8, parity=None, stop=1)

while(True):
    tserver = server()
    tserver.run()
    if RESET: machine.reset()
    gc.collect()
    debug('Free mem: ' + str(gc.mem_free()) + '')
