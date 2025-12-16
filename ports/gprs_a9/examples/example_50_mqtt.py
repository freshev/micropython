# Micropython a9g example
# Source: https://github.com/freshev/micropython
# Author: pulkin
# Modified: freshev
# Demonstrates how to use mqtt for publishing GPS location

# Get online
import cellular
cellular.gprs("internet", "", "")

import umqtt
name = "a9g-micropython-board"
server = "test.mosquitto.org"
topic = "a9g-micropython-board-topic"
client = umqtt.MQTTClient(name, server)
try:
    client.connect()
    client.publish(topic, "version unknown")
    client.disconnect()
    print("Done")
except: raise Exception('MQTT exc')

cellular.gprs(False)
