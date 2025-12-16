# Third-party library

Stored are optional components

## Directory introduction

|Directory|Purpose|Dependencies|
|---|----|----|
|cJSON|json parsing and synthesis| None|
|httpclient|http client|If you use https, rely on mbedtls|
|mbedtls|Encryption decoding library|If you use fota, you need to rely on mbedtls|
|mqtt|mqtt client|If using mqtts,mbedtls|
|sntp|Time synchronization client|None|
|unity|unit testing|none|
|fal|flash abstraction layer|none|
|flashdb|Flash database|depends on fal|
