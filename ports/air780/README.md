# Micropython for Hezhou 4G Cat.1 Air780e (EC618) module

Original module documentation at [OpenLuat](https://docs.openluat.com/air780e/)  
Translated module documentation at [Docs](docs/)

## Test board 
![Air780e test board](https://wiki.luatos.org/_images/pinout2.png)


## Build

### Ubuntu
1. Install xmake
   ```
   ./docs/get_xmake.sh
   ```
2. Clone this repo:
   ```bash
   git clone https://github.com/freshev/micropython
   ```
3. Make
   ```bash
   cd micropython/ports/air780
   configure # --root
   ./make # --root
   ```
4. Firmware `Air780_micropython_VERSION.binpkg` file is in `micropython/ports/air780/version/` folder.

5. Notes:  
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
3. Firmware `Air780_micropython_VERSION` file is in `micropython/ports/air780/version/` folder.

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
- [x] PWM: `machine.PWM` (NOT TESTED - have no test module)
- [x] UART: `machine.UART` 
- [ ] SPI: `machine.SPI` (NOT TESTED - have no test module)
- [ ] I2C: `machine.I2C` (NOT TESTED - have no test module)
- [ ] RTC: `machine.RTC` 
- [x] 4G networking (IMEI, ICCID, SMS, ...): `cellular`
- [x] GPS: `gps`
- [x] time: `time`
- [x] File system (littlefs)
- [x] DNS: `cellular`, `socket`, `ssl`
- [x] Power: `machine`, `watchdog`, `FOTA`
- [ ] CC1101: `CC1101`
- [ ] DHT: `DHT`

Missed software modules actively developed now (At least at 2024).  
I would be very grateful for a test board to test `NOT TESTED` functionality.  
Make a [pull request](https://github.com/freshev/micropython/pulls) to contact.

## Examples

See [examples](examples) folder.

## API

Featured:

1. [`cellular`](#cellular): SMS, calls, connectivity
2. [`socket`](#socket): sockets over 4G
3. [`ssl`](#ssl): SSL over sockets
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

### `ssl` ###

*Alias: `ussl`*

TCP/IP stack over 4G based on Mbedtls.
See [micropython docs](https://docs.micropython.org/en/latest/library/ssl.html) for details.


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
* My Air780EG module often does not boot correctly due to insufficient power from USB.
