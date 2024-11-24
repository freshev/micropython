# Air780EPVH positioning data test program

## Conditions of use

Hardware conditions: Air780EVPH

## Firmware requirements

Air780EP or Air780EPV firmware compiled after 2024.6.1

## Reminder, this demo requires a lot of traffic

If it continues to be enabled, the daily traffic needs to be more than **100M**, so be sure to pay attention!!

This demo does not optimize traffic at all and will report **all** GNSS data for analysis!!!

## demo description

1. Turning on testGnss is a positioning function demonstration, including turning on the GNSS function, obtaining GNSS data, and injecting assisted positioning information (AGPS)
2. Open testMqtt to report positioning information to the MQTT server. The corresponding demonstration webpage is https://iot.openluat.com/iot/device-gnss
3. Open testGpio to demonstrate the GPIO function. After successful positioning, switch the GPIO output.
4. Open testTcp to demonstrate uploading data to the TCP server. The corresponding web page is https://gps.nutz.cn WeChat applet iRTU Object Search
