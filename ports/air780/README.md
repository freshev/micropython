# Micropython for Hezhou 4G Cat.1 Air780e (EC618) module

Original module documentation at [OpenLuat](https://docs.openluat.com/air780e/)  
Development board [Schematic and PCB](https://oshwhub.com/luat/evb_air780x_v1-6)  
Translated module documentation at [Docs](docs/)

## Test board 
![Air780e test board](https://github.com/freshev/micropython/blob/master/ports/air780/docs/pinout.png)


## Build

### Ubuntu
1. Install xmake
   ```
   ./docs/get_xmake.sh
   ```
2. Install prerequisites
   ```bash
   sudo apt install p7zip-full
   ```
3. Clone this repo:
   ```bash
   git clone https://github.com/freshev/micropython
   ```
4. Make
   ```bash
   cd micropython/ports/air780
   configure # --root
   ./make # --root
   ```
5. Firmware `micropython_Air780e_VERSION.binpkg` file is in `micropython/ports/air780/version/` folder.

6. Notes:  
  Version number configured via `configure`  
  Use simple `xmake` to build current version.  
  If build fails with `MP_QSTR_xxx` reason, remove `./build` directory and try again.  


### Windows
1. Clone this repo:
   ```bash
   git clone https://github.com/freshev/micropython
   ```
2. Make
   ```bash
   cd micropython/ports/air780
   configure.bat
   make.bat
   ```
3. Firmware `micropython_Air780e_VERSION` file is in `micropython/ports/air780/version/` folder.

4. Notes:  
  Version number configured via `configure.bat`  
  Use simple `make` to build current version.  
  If build fails with `MP_QSTR_xxx` reason, remove `./build` directory and try again.  

## Burn (Windows only)

1. Get `Luatools` at [LuatOS](https://wiki.luatos.org/pages/tools.html)

2. Run Luatools_v3.exe in newly created folder.

3. Press `Download Firmware` button at upper right corner.

4. Press `Select the file` button and select `.binpkg` file from `micropython/ports/air780/version/` folder.

5. Press `download` button at `Luatools`.

6. Set module to `download state` by pressing module `BOOT` button.  
   Press and release module `RESET` button or `power OFF` and `power ON` the module.  
   Release module `BOOT` button.  
   The firmware download at `Luatools` should start.

or use `ectool.py`

1. Get `ectool` by `pip3 install -U ectool`.

2. Run `ectool burn -f version/Air780_micropython_v1.0.binpkg`.


## FOTA (firmware over the air)

1. Make changes in source files (*.c or modules/*.py).

2. Change version and FOTA URL via `configure`.

3. Run `xmake -b ota`.

4. Upgrade pack `Air780_OLDVERSION_to_NEWVERSION` file is in `micropython/ports/air780/fota/` folder.

5. Deploy upgrade pack to internet.

6. Under python REPL run `machine.OTA('NEWVERSION')`.


## Connect

Use [pyserial](https://github.com/pyserial) or any other terminal (Ubuntu).

```bash
miniterm.py /dev/ttyUSB1 115200 --raw
```

Or use [putty](https://www.putty.org/) (Windows).



## Upload scripts

Use modified [ampy](https://github.com/freshev/Universal_AMPY).

```bash
cli.exe --port /dev/ttyUSB1 put frozentest.py 
```

or use `VS Code` with `rt-thread-micropython` plugin with modified [ampy](https://github.com/freshev/Universal_AMPY).

## Run scripts

```python
>>> help()
>>> import frozentest
>>> import blink
blink.blink(1)
```

## Functionality

- [x] GPIO: `machine.Pin`
- [x] ADC: `machine.ADC`
- [x] PWM: `machine.PWM`
- [x] UART: `machine.UART` 
- [x] SPI: `machine.SPI`
- [x] I2C: `machine.I2C`
- [x] RTC: `machine.RTC` 
- [x] 4G networking (IMEI, ICCID, SMS, ...): `cellular`
- [x] GPS: `gps`
- [x] time: `time`
- [x] File system (littlefs)
- [x] DNS: `cellular`, `socket`, `tls`
- [x] Power: `machine`, `watchdog`, `FOTA`
- [x] DHT: `DHT` sensor module. Use external resistor 5-10 kOhm for DHT11 sensor PULLUP
- [x] CC1101: `CC1101` RF module

## Examples

See [examples](examples) folder.

## API

Featured:

1. [`cellular`](#cellular): SMS, calls, connectivity
2. [`socket`](#socket): sockets over 4G
3. [`tls`](#tls): SSL/TLS over sockets
4. [`machine`](#machine): hardware and power control
5. [`I2C`](#I2C): I2C hardware implementation
6. [`SPI`](#SPI): SPI hardware implementation
7. [`gps`](#gps): everything related to GPS and assisted positioning
8. [`CC1101`](#CC1101): CC1101 module support over SPI
9. [`dht`](#dht): DHT module for DHT11, DHT12, DHT21, DHT22, AM2301 temperature & humidity sensors
10. [`umqtt`](#umqtt): umqtt support via frozen module
11. [`urequests`](#urequests): urequests support via frozen module

### `cellular`

Provides cellular functionality.
As usual, the original API does not give access to radio-level and low-level functionality such as controlling the registration on the cellular network: these are performed in the background automatically.
The purpose of this module is to have an access to high-level networking (SMS, network) as well as to read the status of various components of cellular networking.

#### Constants

* `NETWORK_FREQ_BAND_1`, `NETWORK_FREQ_BAND_3`, `NETWORK_FREQ_BAND_5`, `NETWORK_FREQ_BAND_8`, `NETWORK_FREQ_BAND_34`, `NETWORK_FREQ_BAND_38`, `NETWORK_FREQ_BAND_39`, `NETWORK_FREQ_BAND_40`, `NETWORK_FREQ_BAND_41`, `NETWORK_FREQ_BANDS_ALL`: frequencies;
* `OPERATOR_STATUS_UNKNOWN`, `OPERATOR_STATUS_AVAILABLE`, `OPERATOR_STATUS_CURRENT`, `OPERATOR_STATUS_DISABLED`: operator statuses;
* `ENOSIM`, `EREGD`, `ESMSSEND`, `ESMSDROP`, `EACTIVATION` : extended codes for `OSError`s raised by the module;

#### Classes

* `SMS(phone_number: str, message: str[, pn_type: int, index: int, purpose: int])`: handles SMS messages;
  * `.phone_number` (str): phone number (sender or destination);
  * `.message` (str): message contents;
  * `.purpose` (int): integer with purpose/status bits;
  * `.is_inbox` (bool): indicates incoming message;
  * `.is_read` (bool): indicates message was previously read;
  * `.is_unread` (bool): indicates unread message;
  * `.is_unsent` (bool): indicates unsent message;
  * `.send(timeout: int)`: sends a message;

#### Methods

* `get_imei()` (str): the International Mobile Equipment Identity (IMEI) number;
* `get_ipv4()` (str): get local IPV4 address;
* `get_ipv6()` (str): get local IPV6 address;
* `get_iccid()` (str): the Integrated Circuit Card ID (ICCID) number of the inserted SIM card;
* `get_imsi()` (str): the International Mobile Subscriber Identity (IMSI) number of the inserted SIM card;
* `get_network_status()` (int): cellular network status encoded in an integer. **TODO**: Provide bit-wise specs;
* `poll_network_exception()`: polls the network exception and raises it, if any;
* `is_sim_present()` (bool): checks whether a SIM card is present;
* `is_network_registered()` (bool): checks whether registered on the cellular network;
* `is_roaming()` (bool): checks whether registered on the roaming network;
* `get_signal_quality()` (int, int): the signal quality (0-31), RXLEVEL and signal_to_noise_ratio(SNR);
* `flight_mode([flag: bool])` (bool): the flight mode status. Turns in on or off if the argument is specified;
* `set_bands(bands: int = NETWORK_FREQ_BANDS_ALL)`: sets frequency bands;
* `scan()` (list): lists available operators: returns `(op_id: str, op_name: str, op_status: int)` for each;
* `stations()` (list): a list of nearby stations: `(mcc, mnc, lac, cell_id, bsic, rx_full, rx_sub, arfcn)`: all ints;
* `reset()`: resets network settings to defaults. 
* `gprs([apn: {str, bool}[, user: str, pass: str[, timeout: int]]])` (bool): activate (3 or 4 arguments), deactivate (`gprs(False)`) or obtain the status of network interface (on/off) if no arguments supplied; Stub code for A9G compatibility.
* `ussd(code: str[, timeout: int])` (int, str): USSD request. Stub code for A9G compatibility.
* `on_status_event(callback: Callable)`: sets a callback `function(status: int)` for network status change;
* `on_sms(callback: Callable)`: sets a callback `function(sms_or_status)` on SMS sent or received;;
* `sms_delete_by_index()`: deletes SMS from SIM `read` storage by index;
* `sms_delete_all_read()`: deletes all `read` SMS from SIM storage;
* `sms_read_all()`: read all `unread` SMSs from the `unread` storage at the SIM card;
* `sms_list_read()` (list): returns all `read` SMS from the SIM card;
* `sms_list()` (list): returns all SMS from the SIM card;
* `get_storage_size()` (tuple): SIM SMS storage info;

### `socket` ###

*Alias: `usocket`*

TCP/IP stack over 4G based on lwIP.
See [micropython docs](https://docs.micropython.org/en/latest/library/socket.html) for details.

### `tls` ###

TCP/IP stack over 4G based on Mbed TLS 3.6.3.
See [micropython docs](https://docs.micropython.org/en/latest/library/ssl.html) for details.

### `gps` ###

Provides the GPS functionality.
This is only available in the A9G module where GPS is a separate chip connected via UART2.

* `on()`: turns the GPS on;
* `off()`: turns the GPS off;
* `get_description()`: GPS module description;
* `get_serial()`: GPS module part number;
* `get_hardware_version()`: GPS module hardware version;
* `get_firmware_version()`: GPS module firmware version;
* `get_location()` (longitude: float, latitude: float): retrieves the current GPS location;
* `get_last_location()` (longitude: float, latitude: float): retrieves the last known GPS location without polling the GPS module;
* `get_satellites()` (tracked: int, visible: int): the numbers of satellites in operation;
* `time()` (int): the number of seconds since the epoch (2000). Use `time.localtime` for converting it into date/time values (this conversion may result in `OverflowError` until the GPS module starts reading meaningful satellite data);
* `nmea_data()` (tuple): all NMEA data parsed: `(rmc, (gsa[0], ...), gga, gll, gst, (gsv[0], ...), vtg, zda)`:
  - RMC: `(time: int, valid: bool, latitude, longitude, speed, course, variation: float)`;
  - GSA: `(mode: int, fix_type: int, satellite_prn: bytearray, pdop, hdop, vdop: float)`;
  - GGA: `(time_of_day: int, latitude, longitude: float, fix_quality, satellites_tracked: int, hdop, altitude: float, altitude_units: int, height: float, height_units: int, dgps_age: float)`;
  - GLL: `(latitude, longitude: float, time_of_day, status, mode: int)`;
  - GST: `(time_of_day: int, rms_deviation, ...: float)`;
  - GSV: `(total_messages, message_nr, total_satellites: int, satellite_info[4]: (nr, elevation, azimuth, snr: int))`;
  - VTG: `(true_track_degrees, magnetic_track_degrees, speed_knots, speed_kph: float, faa_mode: int)`;
  - ZDAL `(time, hour_offset, minute_offset: int)`.

  Latitudes and longitudes are in degrees `x100`.
  Time is given in seconds since the epoch or since `00:00` today.
  Status flags `mode`, `status` are ASCII indexes.
  For more info (units, etc) please consult the [minmea](https://github.com/kosma/minmea) project.

### `machine`

Provides power-related functions: power, watchdogs.
* `reset()`: hard-resets the module;
* `reset_cause()`: cause of machine last reset;
* `idle()`: handle any micropython events;
* `OTA(new_version, query='')`: tries to get new formware version `new_version` from internet. Exact URL configured via `configure->FOTA`.

#### Constants

* `HARD_RESET`, `PWRON_RESET`, `WDT_RESET`, `DEEPSLEEP_RESET`, `SOFT_RESET`: reset causes.


#### Classes

* `WDT(wd: int, timeout: int)`: arms the watchdog with a timeout in milliseconds.  
  `wd` parameter is one of:  
  0: use module watchdog;  
  `timeout` parameter can be:  
  > 0: sets watchdog timeout to `timeout` milliseconds
  = 0: turns off `wd` watchdog
  < 0: returns `wd` watchdog instance (i.e. to feed WD)
  * `.feed()`: reset watchdog timer;

#### Methods

* `reset()`: hard-resets the module;
* `idle()`: tunes the clock rate down and turns off peripherials;
* `sleep()`: turnes on light sleep mode;
* `deep_sleep()`: turnes on deep sleep mode;
* `get_input_voltage()` (float, float): the input voltage (mV) and the battery level (percents);
* `reset_cause()` (int): the reset cause, one of `*_RESET` constants.
* `wdt_test()`: Just wait forever with no watchdog feed().


* `FOTA (firmware over the air)`: tries to get new formware version from internet. Exact URL set in `configure`.


### `I2C` ###

I2C interface
See [micropython docs](https://docs.micropython.org/en/latest/library/machine.I2C.html) for details.

### `SPI` ###

SPI interface
See [micropython docs](https://docs.micropython.org/en/latest/library/machine.SPI.html) for details.

### `CC1101` ###
CC1101 433MHz module  
Originally written by Michael [www.elechouse.com](www.elechouse.com)  
Version: November 12, 2010 for Arduino platform.

#### Methods
* `cc1101(id=0, cs=0)`: initialize CC1101 module.
  * `id`: SPI ID. Can be 0 or 1;
  * `cs`: SPI CS. Can be 0;
  * `baudrate`: SPI frequency. Less or equal to 5000000. Default 4000000;
  * `dma_delay`: delay in us after writing and before reading SPI interface. Default 4us;
  * `debug`: print debug messages to MPY;
  * `debug_hst`: print debug messages to A9 diagnostic port;
  * `other parameters`: see [micropython docs](https://docs.micropython.org/en/latest/library/machine.SPI.html) for details;
  * Returns CC1101 object.

* `deinit()`: deinit CC1101 module.
  * Returns None.

* `read(addr)`: internal function. Use `spi_read_reg(...)` instead.  
  Reads CC1101 register with address addr.
  * Returns register value (one byte).

* `read_burst(addr, num)`: internal function. Use `spi_read_burst_reg(...)` instead.  
  Reads num CC1101 registers from address addr.
  * Returns register values (bytearray).

* `write(addr, val)`: internal function. Use `spi_write_reg(...)` instead.  
  Writes CC1101 register at address addr with value val.
  * Returns byte written.

* `write_burst(addr, vals)`: internal function. Use `spi_write_burst_reg(...)` instead.  
  Writes num CC1101 registers from address addr with values from vals bytearray.
  * Returns None.

* `strobe(addr)`: access CC1101 register with address `addr`  
  * Returns access result (one byte).

* `command(addr)`: alias for `strobe(...)`  
  Access CC1101 register with address `addr`.
  * Returns access result (one byte).

* `flush()`: flush SPI FIFO buffers.
  * Returns None.

* `spi_read_status(addr)`: reads CC1101 register status with address `addr`.
  * Returns status (one byte).

* `spi_read_reg(addr)`: reads CC1101 register value with address `addr`.
  * Returns register value(one byte).

* `spi_read_burst_reg(addr, num)`: reads `num` CC1101 register values from address `addr`.
  * Returns register values (bytearray).

* `spi_write_reg(addr, val)`: writes CC1101 register at address `addr` with value `val`.
  * Returns register value(one byte).

* `spi_write_burst_reg(addr, vals)`: writes CC1101 registers from address `addr` with values from `vals` bytearray.
  * Returns None.

* `set_cc_mode(mode)`: sets CC1101 mode from predefined settings.  
  Mode can be 0 or 1. See [CC1101 documentation](docs/CC1101/CC1101.pdf)
  * Returns None.

* `set_mhz(freq)`: sets CC1101 frequency. Frequency is a float value with ranges from [CC1101 documentation](docs/CC1101/CC1101.pdf).  
  Internally calculates and sets FREQ2, FREQ1 and FREQ0 registers.
  * Returns None.

* `get_mhz()`: gets CC1101 frequency.  
  Internally gets values from FREQ2, FREQ1 and FREQ0 registers and calculates CC1101 current frequency.
  * Returns frequency (float).

* `set_channel(channel)`: sets CC1101 frequency channel number (byte).
  * Returns None.

* `get_channel()`: gets CC1101 frequency channel number.
  * Returns frequency channel number (byte).

* `set_modulation(modulation)`: sets CC1101 modulation.  
  Modulation parameter is one of:  
  0: 2-FSK (Binary Frequency Shift Keying)  
  1: GFSK (Gauss Frequency Shift Keying)  
  2: ASK (Amplitude Shift Keying)  
  3: 4-FSK (2bit Binary Frequency Shift Keying)  
  4: MSK (Minimum Shift Keying)  
  * Returns None.

* `set_pa(power)`: sets CC1101 output power.  
  Internally calculates power and set values for PATABLE registers.  
  Power parameter can be from -30.0 dBm to 10 dBm.
  * Returns None.

* `set_chsp(value)`: sets CC1101 channel spacing. Value is a float with ranges from [CC1101 documentation](docs/CC1101/CC1101.pdf).  
  Internally calculates and sets MDMCFG1 and MDMCFG0 registers.
  * Returns None.

* `set_rx_bw(value)`: sets CC1101 channel bandwidth. Value is a float with ranges from [CC1101 documentation](docs/CC1101/CC1101.pdf).  
  Internally calculates and sets MDMCFG4 register.
  * Returns None.

* `set_d_rate(value)`: sets CC1101 channel data rate. Value is a float with ranges from [CC1101 documentation](docs/CC1101/CC1101.pdf).  
  Internally calculates and sets MDMCFG3 and MDMCFG4 registers.
  * Returns None.

* `set_deviation(value)`: sets CC1101 channel deviation. Value is a float with ranges from [CC1101 documentation](docs/CC1101/CC1101.pdf).  
  Internally calculates and sets DEVIATN register.
  * Returns None.

* `set_tx()`: enables TX mode. Turn CC1101 first to IDLE and than to TX state. Transmit data from TX buffer over the air.
  * Returns None.

* `set_rx()`: enables RX. Turn CC1101 first to IDLE and than to RX state.
  * Returns None.

* `set_tx_freq(frequency)`: enables TX. Turns CC1101 first to IDLE state, changes the frequency and than turns to TX state.
  * Returns None.

* `set_rx_freq(frequency)`: enables RX. Turns CC1101 first to IDLE state, changes the frequency and than turns to RX state.
  * Returns None.

* `get_rssi()`: gets receive signal strength.
  * Returns value in dBm (float).

* `get_lqi()`: gets link quality. Higher value indicates a better link quality.
  * Returns link quality indication (byte).

* `set_sres()`: reset CC1101 module.
  * Returns None.

* `set_sfs_tx_on()`: enables and calibrates frequency synthesizer
  * Returns None.

* `set_sx_off()`: turns off crystal oscillator.
  * Returns None.

* `set_scal()`: calibrates frequency synthesizer and turn it off.  
  Timeout 100ms after this command.
  * Returns None.

* `set_srx()`: enables RX. Performs calibration first if coming from IDLE and MCSM0.FS_AUTOCAL=1
  * Returns None.

* `set_stx()`: enables TX (from IDLE state). Performs calibration first if MCSM0.FS_AUTOCAL=1.
  * Returns None.

* `set_sidle()`: exit RX / TX, turn off frequency synthesizer and exit Wake-On-Radio mode if applicable.
  * Returns None.

* `flush_rx()`: flushes the RX FIFO buffer.  
  Only issue SFRX in IDLE or RXFIFO_OVERFLOW states.
  * Returns None.

* `flush_tx()`: flushes the TX FIFO buffer.  
  Only issue SFTX in IDLE or TXFIFO_UNDERFLOW states.
  * Returns None.

* `go_sleep()`: Sets IDLE state, followed by PowerDown state.
  * Returns None.

* `send_data(wbuffer, timeout)`: Sends data from `wbuffer` to CC1101 TX buffer and turns to TX state.  
  Waits for GDO0 strobe (data send finished). Flushes TX buffer after send
  * Returns None.

* `check_receive_flag()`: Check for incoming data.  
   If not in RX state, force RX state.
   * Returns  
   0: If GDO0 is 0  
   1: If GDO0 is 1 - wait for incoming packet receive finish and returns.

* `receive_data()`: receive data from CC1101 RX FIFO buffer.  
  After receive flushes RX buffer, sets IDLE state, followed by RX state
  * Returns data received (bytes)

* `check_crc()`: checks CRC bit in LQI register
  * Returns CRC flag

* `set_clb(byte b, byte s, byte e)`: sets calibration data.  
  Parameter b is from 1 to 4.
  * Returns None.

* `get_cc1101()`: checks CC1101 availability.
  * Returns:  
  0: module unavailable  
  1: module active

* `get_mode()`: gets current state.
  * Returns  
  0: IDLE state  
  1: TX state  
  2: RX state

* `set_sync_word(sh, sl)`: sets sync word.  
  Parameters:  
  sh: high byte of sync word  
  sl: low byte of sync word
  * Returns None.

* `set_addr(addr)`: sets CC1101 address `addr`.
  * Returns None.

* `set_white_data(data)`: sets white data. See p.15.1 at [CC1101 documentation](docs/CC1101/CC1101.pdf).
  * Returns None.

* `set_pkt_format(data)`: sets packet format. See p.15.2 at [CC1101 documentation](docs/CC1101/CC1101.pdf).
  * Returns None.

* `set_crc(flag)`: sets CRC check flag. See p.15 at [CC1101 documentation](docs/CC1101/CC1101.pdf).
  * Returns None.

* `set_length_config(lconf)`: sets length config. See p.15 at [CC1101 documentation](docs/CC1101/CC1101.pdf).
  * Returns None.

* `set_packet_length(length)`: sets packet lenght. See p.15 at [CC1101 documentation](docs/CC1101/CC1101.pdf).
  * Returns None.

* `set_dc_filter_off(flag)`: turns off (or on) digital channel filter bandwidth. See p.13 at [CC1101 documentation](docs/CC1101/CC1101.pdf).
  * Returns None.

* `set_manchester(flag)`: turns on (or off) Manchester encoding. See p.16 at [CC1101 documentation](docs/CC1101/CC1101.pdf).
  * Returns None.

* `set_sync_mode(mode)`: sets CC1101 sync mode. See p.17.1 at [CC1101 documentation](docs/CC1101/CC1101.pdf).
  * Returns None.

* `set_fec(flag)`: turns on (or off) `Forward Error Correction`. See p.18.1 at [CC1101 documentation](docs/CC1101/CC1101.pdf).
  * Returns None.

* `set_pre(count)`: sets the minimum number of preamble bytes to be transmitted (MDMCFG1).  
  Parameter `count` from 0(2 preamble bytes) to 7(24 preample bytes)
  * Returns None.

* `set_pqt(th)`: sets preamble quality estimator threshold. See PKTCTRL1 at [CC1101 documentation](docs/CC1101/CC1101.pdf).
  * Returns None.

* `set_crc_af(flag)`: enables (or disables) automatic flush of RX FIFO when CRC in not OK. See PKTCTRL1 at [CC1101 documentation](docs/CC1101/CC1101.pdf).
  * Returns None.

* `set_append_status(flag)`: appends two status bytes to payload. See PKTCTRL1 at [CC1101 documentation](docs/CC1101/CC1101.pdf).
  * Returns None.

* `set_adr_chk(mode)`: controls address check configuration of received packages. See PKTCTRL1 at [CC1101 documentation](docs/CC1101/CC1101.pdf).
  * Returns None.

* `check_rx_fifo()`: checks RX FIFO buffer for data.
  * Returns  
  0: RX buffer has no data  
  1: RX buffer has data

* `crc8(data, polynomial)`: returns crc8 on `data` with `polynomial`.
  * Returns crc8 byte

* `ba2hex(ba)`: returns hex string for `ba` bytearray parameter (340 bytes maximum).
  * Returns hex string

### `dht` ###
DHT series temperature and humidity sensor module.  
Originally written by Adafruit Industries [https://www.adafruit.com](https://www.adafruit.com)

#### Attention
DHT sensors like DHT21, DHT22, AM2301 need external pullup resistor (i.e. 10k ohm) for working properly.

#### Methods
* `dht(pin=20, type=dht.DHT11)`: initialize DHT module.
  * `pin`: Pin number;
  * `type`: Sensor type. One of DHT11, DHT12, DHT21, DHT22 or AM2301(the same as DHT21);
  * Returns DHT object.

* `get_type()`: sensor type, defined in constructor.
  * Returns sensor type as integer: 11 - for DHT11, 12 - for DHT12, etc.

* `read_temperature(S=True, force=True)`: read sensor temperature.
  * `S`: Scale.  
     - True = Fahrenheit  
     - False = Celcius;
  * `force`: force read data from sensor;
  * Returns temperature as float or `nan` if reading failed.

* `read_humidity(force=True)`: read sensor humidity.
  * `force`: force read data from sensor;
  * Returns humidity as float or `nan` if reading failed.

* `deinit()`: deinit sensor. Free pin for other application.


### umqtt ###
Rewritten from umqtt.simple to support python 1.5.2 syntax.  
In order to reduce module size `umqtt` has only `connect`, `close` and `publish` methods.  
To use other methods, uncomment needed methods in `modules/umqtt.py` and rebuild port.

### urequests ###
Helper class to make http/https requests (GET, PUT, HEAD, etc)

## Notes ##

* The size of micropython heap is roughly 512 Kb. 206K(+32K) can be realistically allocated right after hard reset.
* Firmware removes *.txt files in SOC file system by SMS 'rmconfig' (configurable).
* Firmware removes *.py files in SOC file system by SMS 'rmcode' (configurable).
* Firmware removes *.py and *.txt files in SOC file system by SMS 'rmall' (configurable).
* Firmware resets the module by SMS 'reset' (configurable).
* You can configure module with auto respawned (after delete) `main.py` script from `examples` folder.
* My Air780EG (GPS) module often does not boot correctly due to insufficient power from USB.
* To run FotaToolkit under Linux the bash script [FotaToolkit](../../lib/luatos-soc-2022/tools/dtools/FotaToolkit) and [deltagen](../../lib/luatos-soc-2022/tools/deltagen) were written.
* You can deploy BINPKG and FOTA pack files to another folder in compressed or uncompressed form (configurable)
* When REPL over USB feature used, writing special characters to UART over USB (or soft reboot in MP) conflicts with Luatools (module hangs). Close Luatools window or use REPL over UART1/UART2.

## Known bugs ##

Module freezes when use UART with Luatools running.  
If build fails with `MP_QSTR_xxx` reason, remove `./build` directory and try again.  
Feel free to debug and contribute.
