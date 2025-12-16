# Micropython a9g example
# Source: https://github.com/freshev/micropython
# Author: pulkin
# Modified: freshev
# Demonstrates how to send and receive SMS
import cellular
import time

global flag
flag = 1

def sms_handler(evt):
    global flag
    if evt == cellular.SMS_SENT:
        print("SMS sent")

    else:
        print("SMS received, attempting to read ...")
        cellular.sms_read_all()
        ls = cellular.sms_list_read()
        print(ls[-1])
        flag = 0

cellular.on_sms(sms_handler)
cellular.SMS("8800", "test").send()

print("Doing something important ...")
while flag:
    time.sleep(1)

print("Done!")

