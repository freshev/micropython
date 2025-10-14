Examples for micropython on Air780e
===================================

[example_01_blink.py](example_01_blink.py)

Blinks the built-in blue LED on the pudding board connected to GPIO 27.

Keywords: **gpio, machine, blink, hello world**

Modules: `machine`, `time`

[example_05_watchdog.py](example_05_watchdog.py)

Arms the hardware watchdog and sleeps until hard reset.

Keywords: **watchdog, hardware, reset**

Modules: `machine`, `time`

[example_10_FOTA.py](example_05_watchdog.py)

Firmware over the air upgrade

Keywords: **FOTA, firmware upgrade**

Modules: `machine`


[example_21_dht.py](example_21_dht.py)

Displays temperature using DHT temperature sensor.

Keywords: **DHT**

Modules: `dht`

[example_22_cc1101.py](example_22_cc1101.py)

RF scanner based on CC1101 RF module.

Keywords: **CC1101, Scanner**

Modules: `cc1101`

[example_30_network.py](example_30_network.py)

Tests connection to the internet by running a raw HTTP request.

Keywords: **internet, TCP, HTTP, GPRS, connection**

Modules: `cellular`, `socket`

[example_31_ssl.py](example_31_ssl.py)

Test an encrypted SSL HTTP request.

Keywords: **HTTPS, SSL, encryption**

Modules: `cellular`, `socket`, `ssl`

[example_32_http.py](example_32_http.py)

Performs a simple URL request using `urequests` from [micropython-lib](https://github.com/micropython/micropython-lib).

Keywords: **URL, requests, HTTP, HTTPS**

Modules: `cellular`, `urequests` (external), `upip`

[example_40_network_events.py](example_40_network_events.py)

Demonstrates how to track GSM network events.

Keywords: **GSM, connect, flight mode**

Modules: `cellular`

[example_41_sms.py](example_41_sms.py)

Demonstrates how to send and receive SMS.

Keywords: **SMS**

Modules: `cellular`

[example_42_event_driven_gprs.py](example_42_event_driven_gprs.py)

Demonstrates how to connect and download in the background using events.

Keywords: **GPRS, background, connection**

Modules: `cellular`, `socket`

[example_43_ussd.py](example_43_ussd.py)

Demonstrates how to preform a simple USSD request.

Keywords: **USSD**

Modules: `cellular`

[example_50_mqtt.py](example_50_mqtt.py)

Reports location to a test MQTT server via cellular connection using `umqtt` from [micropython-lib](https://github.com/micropython/micropython-lib).

Keywords: **MQTT, GPS, track**

Modules: `cellular`, `umqtt` (external), `gps`, `upip`


[example_99_sms_server.py](example_99_sms_server.py)

SMS Sender Server software.

server commands should be in json format:
{"command": "ping"} -> {"response" : "success"} or {"response" : "failed"}
{"command": "send", "phone" : "+7XXXXXXXXXX", "message": "test"} -> {"response" : "success"}, {"response" : "failed"}, {"response" : "toolong"} or {"response" : "balance"}
{"command": "receive" } -> {"response": "success", "list":[("from": "+7XXXXXXXXXX", "message": "test response", "ts":(2024, 12, 31, 23, 59, 59, 3)),("from": "+7...", "message": "...", "ts":(2024, 12, 31, 23, 59, 59, 3))]}


