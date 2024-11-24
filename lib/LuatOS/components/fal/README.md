# FAL: Flash abstraction layer

## 1. Introduction to FAL

FAL (Flash Abstraction Layer) Flash abstraction layer is an abstraction layer that manages and operates Flash and Flash-based partitions. It unifies the API for Flash and partition operations on the upper layer (the framework diagram is shown below), and has the following characteristics:

- Supports statically configurable partition tables and can be associated with multiple Flash devices;
- Partition table supports **auto-mount**. Avoid the problem of partition tables being defined multiple times in multiple firmware projects;
- The code is streamlined, **no dependence** on the operating system, and can run on bare metal platforms, such as bootloaders that have certain resource requirements;
- Unified operation interface. Ensures the reusability of the underlying Flash driver for components that have a certain dependence on Flash, such as file systems, OTA, and NVM (for example: [EasyFlash](https://github.com/armink-rtt-pkgs/EasyFlash));
- Comes with test commands based on Finsh/MSH, which can operate (read, write, and erase) Flash or partitions through Shell in byte addressing mode, making it convenient for developers to debug and test;

![FAL framework](docs/figures/fal_framework.png)

### 1.1. Open FAL

To use fal package, you need to select it in RT-Thread's package manager. The specific path is as follows:

```
RT-Thread online packages
system packages --->
--- fal: Flash Abstraction Layer implement. Manage flash device and partition.
[*]   Enable debug log output
[*]   FAL partition table config has defined on 'fal_cfg.h'
(onchip) The flash device which saving partition table
(65536) The patition table end address relative to flash device offset.
[ ]   FAL uses SFUD drivers
(norflash0) The name of the device used by FAL (NEW)
version (latest)  --->
```

The configuration instructions for each function are as follows:

- Enable debug log output (enabled by default);
- Whether the partition table is defined in `fal_cfg.h` (enabled by default). If this option is turned off, fal will automatically go to the specified location of Flash to retrieve and load the partition table. Please see the following two options for specific configuration details;
- Flash device that stores partition table;
- The offset at which the partition table's **end address** is located on the Flash device. fal will retrieve the partition table starting from this address and read directly to the top of Flash. If you are not sure about the specific location of the partition table, you can also configure it as the end address of Flash. Fal will retrieve the entire Flash, and the retrieval time may increase.
- Enable FAL porting files for SFUD (off by default);
- The name of the FLASH device passed in when calling the `rt_sfud_flash_probe` function should be entered (it can also be obtained by viewing the name of the Block Device through the list_device command). This name corresponds to the Flash name in the partition table. Only by setting the device name correctly can the read and write operations on FLASH be completed.

Then let RT-Thread's package manager update automatically, or use the `pkgs --update` command to update the package into the BSP.

### 1.2, FAL directory

| Name | Description |
| ------- | ---------- |
| inc | header file directory |
| src | source code directory |
| samples | Routine directory |

### 1.3、FAL API

The FAL-related API is shown in the figure, [Click here to view detailed explanation of API parameters] (docs/fal_api.md).

![FAL API](docs/figures/fal-api.png)

### 1.4. License

fal package follows the LGPLv2.1 license, see the `LICENSE` file for details.

### 1.5. Dependence

No dependencies on RT-Thread and can also be used on bare metal.

> The test command function requires RT-Thread Finsh/MSH

## 2. Use FAL

The basic steps for using FAL are as follows:

1. Open FAL: Open the fal package from Env and download it to the project.
2. FAL transplantation: define flash device, define flash device table, and define flash partition table. The following mainly explains step 2.
3. Call fal_init() to initialize the library: after the transplantation is completed, it can be called at the application layer, such as in the main function.

![fal port](docs/figures/fal-port.png)

### 2.1. Define flash device

Before defining the Flash device table, you need to define the Flash device first. It can be on-chip flash or off-chip SFUD-based spi flash:

- To define the on-chip flash device, please refer to [`fal_flash_sfud_port.c`](https://github.com/RT-Thread-packages/fal/blob/master/samples/porting/fal_flash_sfud_port.c).
- To define the off-chip spi flash device, please refer to [`fal_flash_stm32f2_port.c`](https://github.com/RT-Thread-packages/fal/blob/master/samples/porting/fal_flash_stm32f2_port.c).

To define specific Flash device objects, users need to implement the `init`, `read`, `write`, and `erase` operating functions respectively according to their own Flash conditions:

- `static int init(void)`: **optional** initialization operation.
- `static int read(long offset, uint8_t *buf, size_t size)`: read operation.

| Parameters | Description |
| ------ | ------------------------- |
| offset | Flash offset address of read data |
| buf | Buffer to store data to be read |
| size | The size of the data to be read |
| return | Returns the actual read data size |

- `static int write(long offset, const uint8_t *buf, size_t size)`: Write operation.

| Parameters | Description |
| ------ | ------------------------- |
| offset | Flash offset address of written data |
| buf | Buffer to store data to be written |
| size | The size of the data to be written |
| return | Returns the actual written data size |

- `static int erase(long offset, size_t size)`: Erase operation.

| Parameters | Description |
| ------ | ------------------------- |
| offset | Flash offset address of erased area |
| size | The size of the erased area |
| return | Returns the actual erased area size |

Users need to implement these operation functions respectively according to their own Flash conditions. The specific Flash device object is defined at the bottom of the file. The following example defines stm32f2 on-chip flash: stm32f2_onchip_flash

```c
const struct fal_flash_dev stm32f2_onchip_flash =
{
.name       = "stm32_onchip",
.addr       = 0x08000000,
.len        = 1024*1024,
.blk_size   = 128*1024,
.ops        = {init, read, write, erase},
.write_gran = 8
};
```

- `"stm32_onchip"` : The name of the Flash device.
- `0x08000000`: The starting address of Flash operation.
- `1024*1024`: Total size of Flash (1MB).
- `128*1024`: Flash block/sector size (because the block sizes of STM32F2 are not uniform, the erase granularity is the size of the largest block: 128K).
- `{init, read, write, erase}`: Flash operation function. If there is no init initialization process, the first operation function position can be left blank.
- `8`: Set the write granularity, unit bit, 0 means not effective (default value is 0), this member is a new member whose fal version is greater than 0.4.0. Each flash write granularity is different and can be set through this member. Here are some common flash write granularities:
- nor flash: 1 bit
- stm32f4:  8 bit
- stm32f1:  32 bit
- stm32l4:  64 bit

### 2.2. Define flash device table

The Flash device table is defined in the `fal_cfg.h` header file. Before defining the partition table, you need to create a new `fal_cfg.h` file**. Please put this file in the port folder of the corresponding BSP or project directory, and Add the header file path to the project. fal_cfg.h can be completed by referring to [samples/porting/fal_cfg.h](https://github.com/RT-Thread-packages/fal/blob/master/samples/porting/fal_cfg.h).

Device table example:

```c
/* ===================== Flash device Configuration ========================= */
extern const struct fal_flash_dev stm32f2_onchip_flash;
extern struct fal_flash_dev nor_flash0;

/* flash device table */
#define FAL_FLASH_DEV_TABLE                                          \
{                                                                    \
&stm32f2_onchip_flash,                                           \
&nor_flash0,                                                     \
}
```

In the Flash device table, there are two Flash objects, one is the on-chip Flash of STM32F2, and the other is the off-chip Nor Flash.

### 2.3. Define flash partition table

The partition table is also defined in the `fal_cfg.h` header file. Flash partitions are based on Flash devices, and each Flash device can have N partitions. The set of these partitions is the partition table. Before configuring the partition table, make sure that the **Flash device** and **device table** have been defined. fal_cfg.h can be completed by referring to [samples/porting/fal_cfg.h](https://github.com/RT-Thread-packages/fal/blob/master/samples/porting/fal_cfg.h).

Partition table example:

```c
#define NOR_FLASH_DEV_NAME             "norflash0"
/* ====================== Partition Configuration ========================== */
#ifdef FAL_PART_HAS_TABLE_CFG
/* partition table */
#define FAL_PART_TABLE                                                               \
{                                                                                    \
{FAL_PART_MAGIC_WORD,        "bl",     "stm32_onchip",         0,   64*1024, 0}, \
{FAL_PART_MAGIC_WORD,       "app",     "stm32_onchip",   64*1024,  704*1024, 0}, \
{FAL_PART_MAGIC_WORD, "easyflash", NOR_FLASH_DEV_NAME,         0, 1024*1024, 0}, \
{FAL_PART_MAGIC_WORD,  "download", NOR_FLASH_DEV_NAME, 1024*1024, 1024*1024, 0}, \
}
#endif /* FAL_PART_HAS_TABLE_CFG */
```

The detailed description of the above partition table is as follows:

| Partition name | Flash device name | Offset address | Size | Description |
| ----------- | -------------- | --------- | ----- | ------------------ |
| "bl" | "stm32_onchip" | 0 | 64KB | Bootloader |
| "app" | "stm32_onchip" | 64*1024 | 704KB | Application |
| "easyflash" | "norflash0" | 0 | 1MB | EasyFlash parameter storage |
| "download" | "norflash0" | 1024*1024 | 1MB | OTA download area |

The partition parameters that users need to modify include: partition name, associated Flash device name, offset address (relative to the internal Flash device), and size. The following points need to be noted:

- The partition name is guaranteed to **not be repeated**;
- The associated Flash device **must have been defined in the Flash device table** and the **names are consistent**, otherwise an error that the Flash device cannot be found will occur;
- The starting address and size of the partition **cannot exceed the address range of the Flash device**, otherwise it will cause a package initialization error;

> Note: When defining each partition, in addition to filling in the parameter attributes Introductiond above, you need to add the `FAL_PART_MAGIC_WORD` attribute in front and `0` at the end (currently used to retain functions)

## 3. Finsh/MSH test command

fal provides a wealth of test commands. The project only needs to enable the Finsh/MSH function on RT-Thread. These commands will be very useful when developing and debugging Flash-based applications. It can accurately write or read the original Flash data at the specified location, quickly verify the integrity of the Flash driver, and even perform performance testing on the Flash.

The specific functions are as follows: Enter fal to see the complete command list

```
msh />fal
Usage:
fal probe [dev_name|part_name]   - probe flash device or partition by given name
fal read addr size               - read 'size' bytes starting at 'addr'
fal write addr data1 ... dataN   - write some bytes 'data' starting at 'addr'
fal erase addr size              - erase 'size' bytes starting at 'addr'
fal bench <blk_size>             - benchmark test with per block size

msh />
```

### 3.1. Specify the Flash device or Flash partition to be operated

When using the fal command for the first time, typing `fal probe` directly will display the partition table information. You can specify that the object to be operated is a partition in the partition table or a Flash device.

After the partition or Flash is successfully selected, some of its properties will be displayed. The approximate effect is as follows:

```
msh />fal probe
No flash device or partition was probed.
Usage: fal probe [dev_name|part_name]   - probe flash device or partition by given name.
[I/FAL] ==================== FAL partition table ====================
[I/FAL] | name      | flash_dev    |   offset   |    length  |
[I/FAL] -------------------------------------------------------------
[I/FAL] | bl        | stm32_onchip | 0x00000000 | 0x00010000 |
[I/FAL] | app       | stm32_onchip | 0x00010000 | 0x000b0000 |
[I/FAL] | ef        | norflash0    | 0x00000000 | 0x00100000 |
[I/FAL] | download  | norflash0    | 0x00100000 | 0x00100000 |
[I/FAL] =============================================================
msh />
msh />fal probe download
Probed a flash partition | download | flash_dev: norflash0 | offset: 1048576 | len: 1048576 |.
msh />
```

### 3.2. Erase data

First enter `fal erase`, followed by the starting address and length of the data to be erased. The following command is: erase 4096 bytes of data starting from address 0 (relative to Flash or partition)

> Note: According to the characteristics of Flash, the erase action will be processed according to sector alignment. Therefore, if the erase operation address or length is not aligned with the Flash sector, the entire sector data associated with it will be erased.

```
msh />fal erase 0 4096
Erase data success. Start from 0x00000000, size is 4096.
msh />
```

### 3.3. Write data

First enter `fal write`, followed by N data to be written, separated by spaces. The following command is: starting from address 8, write the 5 bytes of data 1, 2, 3, 4, and 5.

```
msh />fal write 8 1 2 3 4 5
Write data success. Start from 0x00000008, size is 5.
Write data: 1 2 3 4 5 .
msh />
```

### 3.4. Reading data

First enter `fal read`, followed by the starting address and length of the data to be read. The following command is: read 64 bytes of data starting from address 0

```
msh />fal read 0 64
Read data success. Start from 0x00000000, size is 64. The data is:
Offset (h) 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F
[00000000] FF FF FF FF FF FF FF FF 01 02 03 04 05 FF FF FF
[00000010] FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
[00000020] FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
[00000030] FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF

msh />
```

### 3.5. Performance test

The performance test will test the erasing, writing and reading speed of Flash, and will also test the accuracy of writing and reading data to ensure the consistency of writing and reading data of the entire Flash or the entire partition.

Enter `fal bench` first, followed by the sector size of the Flash to be tested (please check the corresponding Flash manual, SPI Nor Flash is generally 4096). Since the performance test will cause data loss of the entire Flash or the entire partition, the command must be followed by `yes` at the end.

```
msh />fal bench 4096 yes
Erasing 1048576 bytes data, waiting...
Erase benchmark success, total time: 2.674S.
Writing 1048576 bytes data, waiting...
Write benchmark success, total time: 7.107S.
Reading 1048576 bytes data, waiting...
Read benchmark success, total time: 2.716S.
msh />
```

## 4. Common applications

- [fatfs file system routine based on FAL partition](https://github.com/RT-Thread/IoT_Board/tree/master/examples/15_component_fs_flash)
- [Application note of littlefs file system based on FAL partition](https://www.rt-thread.org/document/site/application-note/components/dfs/an0027-littlefs/)
- [EasyFlash transplantation instructions based on FAL partition](https://github.com/armink-rtt-pkgs/EasyFlash/tree/master/ports)

## 5. Frequently Asked Questions

**1. When using FAL, the `fal_cfg.h` header file cannot be found**

`fal_cfg.h` is the configuration file of the fal software package. The user needs to manually create it and define the relevant partition table information. Please place this file under the port folder of BSP or the port folder of the project directory (if not, create a new port folder), and add the header file path to the project. For details, see "`2.2, Defining Flash Devices" Table `" section.

## 6. Contact information

* Maintenance: [armink](https://github.com/armink)
* Home page: https://github.com/RT-Thread-packages/fal
