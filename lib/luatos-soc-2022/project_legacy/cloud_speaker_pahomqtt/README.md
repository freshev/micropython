# illustrate

* This demo is a cloud speaker demonstration demo

## Function description
1: After the device starts, it first broadcasts: "Powering on"
2: After the device successfully connects to the mqtt server and subscribes to the topic, it will broadcast: "Server connection successful"
3: Use another mqtt clinet to send a fixed format message to the topic subscribed by the device, and the device will broadcast the Alipay receipt xxxx yuan

## Topics subscribed by the device
/sub/topic/money/{imei} {imei} is the imei of the Modules
For example, the imei of the Modules is: 866714xxxx16190

Then the topic subscribed by the device is /sub/topic/money/866714xxxx16190

## Server to which the device is connected
host：lbsmqtt.airm2m.com
port：1884

## Message delivery format
{"money":"number"}
Where number is a number not greater than 99999999.99

like:
Send {"money":"12345678.9"} to the topic "/sub/topic/money/866714xxxx16190"
The device will broadcast "Alipay received 12,345,678.9 yuan"
