# LuatOS demo code

## IMPORTANT NOTE

The demo of the library usually needs to be matched with the latest firmware. If you find a problem with the demo, please first confirm whether it is the latest firmware.

The latest firmware download address: https://gitee.com/openLuat/LuatOS/releases

## applicability of demo

* If there are subfolders, such as Air101, it means that the demo may only be suitable for the corresponding hardware. But Air101/Air103/W806 belong to the same type and are basically universal.
* Those without sub-files are usually general-purpose demos, which have nothing to do with the specific hardware. However, the firmware used may not have the corresponding library, and it will prompt xxx not found or nil xxx

## Demo list

* Please refer to the [wlan](wlan/) directory for demos related to the esp32c3 distribution network.

|File name|Function|Dependent libraries|Supported Moduless|Remarks|
|------|----|-------|-----------|----|
|[adc](https://gitee.com/openLuat/LuatOS/tree/master/demo/adc/)|Analog-to-digital conversion|adc|All||
|[camera](https://gitee.com/openLuat/LuatOS/tree/master/demo/camera/)|Camera|camera|air105||
|[coremark](https://gitee.com/openLuat/LuatOS/tree/master/demo/coremark/)|Benchmark|coremark|All |production firmware does not have this library and can be compiled by yourself or in the cloud|
|[crypto](https://gitee.com/openLuat/LuatOS/tree/master/demo/crypto/)|Encryption and decryption|crypto|all||
|[dht12](https://gitee.com/openLuat/LuatOS/tree/master/demo/dht12/)|Temperature and humidity sensor|i2c|All||
|[eink](https://gitee.com/openLuat/LuatOS/tree/master/demo/eink/)|Electronic Ink Screen|eink|All||
|[fatfs](https://gitee.com/openLuat/LuatOS/tree/master/demo/fatfs/)|Mount sd card|fatfs,sdio|all|Some Moduless support sdio mounting, and the rest support spi mounting |
|[fdb](https://gitee.com/openLuat/LuatOS/tree/master/demo/fdb/)|persistent kv storage|fdb|all||
|[fs](https://gitee.com/openLuat/LuatOS/tree/master/demo/fs/)|File system|io|All||
|[gpio](https://gitee.com/openLuat/LuatOS/tree/master/demo/gpio/)|General Input and Output|gpio|All||
|[gpio_irq](https://gitee.com/openLuat/LuatOS/tree/master/demo/gpio_irq/)|io interrupt|gpio|all||
|[gtfont](https://gitee.com/openLuat/LuatOS/tree/master/demo/gtfont/)|Qualcomm font|gtfont|all|requires additional Qualcomm font chip, plug-in in SPI|
|[hello_world](https://gitee.com/openLuat/LuatOS/tree/master/demo/hello_world/)|Simplest example|None|All||
|[i2c](https://gitee.com/openLuat/LuatOS/tree/master/demo/i2c/)|IIC bus|i2c|all|Demonstrate basic i2c operations|
|[io_queue](https://gitee.com/openLuat/LuatOS/tree/master/demo/io_queue/)|IO sequence|ioqueue|air105|High-precision IO sequence|
|[ir](https://gitee.com/openLuat/LuatOS/tree/master/demo/ir/)|Infrared|ir|air105|Currently only supports reception|
|[json](https://gitee.com/openLuat/LuatOS/tree/master/demo/json/)|JSON codec|json|all||
|[keyboard](https://gitee.com/openLuat/LuatOS/tree/master/demo/keyboard/)|Keyboard matrix|keyboard|air105|Hardware direct drive|
|[lcd](https://gitee.com/openLuat/LuatOS/tree/master/demo/lcd/)|SPI screen driver|lcd,spi|all||
|[lcd_custom](https://gitee.com/openLuat/LuatOS/tree/master/demo/lcd_custom/)|Custom LCD screen driver|lcd,spi|All|Custom LCD driver|
|[lcd_mlx90640](https://gitee.com/openLuat/LuatOS/tree/master/demo/lcd_mlx90640/)|Infrared temperature measurement|mlx90640|All|Unfinished|
|[libcoap](https://gitee.com/openLuat/LuatOS/tree/master/demo/libcoap/)|coap codec|licoap|all|only codec, not communication|
|[libgnss](https://gitee.com/openLuat/LuatOS/tree/master/demo/libgnss/)|GNSS parsing|libgnss|all|communication with GNSS Modules via UART|
|[lvgl](https://gitee.com/openLuat/LuatOS/tree/master/demo/lvgl/)|LVGL examples|lvgl,spi|all|There are a large number of LVGL examples in this directory, and examples of different Moduless are also Can refer to |
|[meminfo](https://gitee.com/openLuat/LuatOS/tree/master/demo/meminfo/)|Memory Status|rtos|All||
|[multimedia](https://gitee.com/openLuat/LuatOS/tree/master/demo/multimedia/)|Multimedia|decoder|air105|Audio decoding example|
|[network](https://gitee.com/openLuat/LuatOS/tree/master/demo/network/)|Network library|network|air105|Works with w5500 to achieve Ethernet access|
|[nimble](https://gitee.com/openLuat/LuatOS/tree/master/demo/nimble/)|Bluetooth library|nimble|air101/air103|Only supports simple transceiver and high power consumption|
|[ota](https://gitee.com/openLuat/LuatOS/tree/master/demo/ota/)|Firmware update|uart|If you have your own network, please use the libfota library, refer to fota's demo||
|[pm](https://gitee.com/openLuat/LuatOS/tree/master/demo/pm/)|Power Consumption Control|pm|All||
|[pwm](https://gitee.com/openLuat/LuatOS/tree/master/demo/pwm/)|Controllable Square Wave|pwm|All||
|[rtc](https://gitee.com/openLuat/LuatOS/tree/master/demo/rtc/)|Internal clock|rtc|All||
|[sfud](https://gitee.com/openLuat/LuatOS/tree/master/demo/sfud/)|Universal FLASH reading and writing|sfud,spi|all||
|[sht20](https://gitee.com/openLuat/LuatOS/tree/master/demo/sht20/)|Temperature and humidity sensor|i2c|All||
|[sht30](https://gitee.com/openLuat/LuatOS/tree/master/demo/sht30/)|Temperature and humidity sensor|i2c|All||
|[socket](https://gitee.com/openLuat/LuatOS/tree/master/demo/socket/)|Network Socket|socket|air105/air780e||
|[spi](https://gitee.com/openLuat/LuatOS/tree/master/demo/spi/)|SPI library demo|spi|all||
|[statem](https://gitee.com/openLuat/LuatOS/tree/master/demo/statem/)|io state machine|statem|all|air105 recommends using ioqueue|
|[sys_timerStart](https://gitee.com/openLuat/LuatOS/tree/master/demo/sys_timerStart/)|Demo timed running|sys|All||
|[u8g2](https://gitee.com/openLuat/LuatOS/tree/master/demo/u8g2/)|Monochrome OLED screen driver|u8g2|All||
|[uart](https://gitee.com/openLuat/LuatOS/tree/master/demo/uart/)|UART Demo|uart|All||
|[usb_hid](https://gitee.com/openLuat/LuatOS/tree/master/demo/usb_hid/)|USB自定义HID|usbapp|air105||
|[usb_tf](https://gitee.com/openLuat/LuatOS/tree/master/demo/usb_tf/)|USB read and write TF card|usbapp|air105|Speed   500~700kbyte/s|
|[usb_uart](https://gitee.com/openLuat/LuatOS/tree/master/demo/usb_uart/)|USB virtual serial port|usbapp|air105||
|[video_play](https://gitee.com/openLuat/LuatOS/tree/master/demo/video_play/)|Video play|uart,sdio|all|Currently only supports naked rgb565ble video stream|
|[wdt](https://gitee.com/openLuat/LuatOS/tree/master/demo/wdt/)|harddog|wdt|all||
|[ws2812](https://gitee.com/openLuat/LuatOS/tree/master/demo/ws2812/)|Driver WS2812B|gpio,pwm,spi|all||
|[wlan](https://gitee.com/openLuat/LuatOS/tree/master/demo/wlan/)|wifi related|wlan|ESP32 series supports wifi, Air780E series only supports wifi scanning||

