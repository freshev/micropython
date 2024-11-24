# Upgrade package format and generation instructions (applicable to EC618)

This document describes the upgrade package format and generation method of various firmware of `EC618` series Moduless

## the term

* Original binpkg - the original firmware of the Modules, must be `binpkg suffix`
* Original differential package - use the FOTA tool of Yixin to generate a differential package through 2 binpkg files, with the suffix `.par`
* Hezhou standard AT firmware - In addition to QAT, firmware that supports Hezhou standard AT commands
* QAT Firmware - Firmware that only supports QAT instructions
* CSDK firmware - `non-LuatOS` firmware compiled from this code base (luatos-soc-2022)
* LuatOS firmware - `LuatOS` firmware compiled from this code base (luatos-soc-2022)
* Differential upgrade - refers to the underlying CP/AP firmware upgrade. If it is `LuatOS` firmware, it will include script area data
* Whole package upgrade - used for specific CSDK firmware, including complete CP/AP data, does not depend on old versions, and does not need to generate differential packages

## Upgrade package format list

There are actually 4 specific differential packet formats, namely:

1. Original differential package - applicable to `QAT firmware` / `CSDK firmware (not full package upgrade)`
2. Hezhou AT differential package - suitable for `Hezhou standard AT firmware`
3. CSDK whole package upgrade package - applicable to `CSDK firmware (whole package upgrade)`
4. LuatOS differential upgrade package - suitable for `LuatOS firmware`

## Generation method - original differential package

Suitable for `QAT firmware` / `CSDK firmware (not full package upgrade)`

1. Prepare 2 binpkg files, assuming `old.binpkg` and `new.binpkg`
2. Use the FOTA tool to generate a `.par` file, assuming it is `diff.par`
3. Do not add any before and after data, and deliver it to the device as it is.

## Generation method - Hezhou AT differential package

Suitable for `Universe standard AT firmware`, such as AT/LPAT/LSAT/AUAT

1. Use the `original differential package` generation method, first generate the `diff.par` file
2. Get the file size of `diff.par`, assuming it is `diff_size`
3. Generate file header data, the pseudo code in python form is as follows
4. Attach the above data to the header of the `diff.par` file to generate a differential package in the AT format.

```python
head = struct.pack(">bIbI", 0x7E, 0x01, 0x7D, diff_size)
```

## Generation method - CSDK whole package upgrade package

Suitable for `CSDK firmware (whole package upgrade)`, here only describes the general process, please refer to the code in dtools for specific details

1. Prepare the binpkg file, assuming it is `new.binpkg`, there is no need for the old version of the binpkg file
2. Use `fcelf` to unzip binpkg and get `cp.bin`, `ap.bin`, `imagedata.json`
3. Use `soc_tools.exe` to compress `cp.bin` into `cp.zip`, which is actually in `lzma` format and the fragment size is 256k
4. Use `soc_tools.exe` to compress `ap.bin` into `ap.zip`, which is actually in `lzma` format and the fragment size is 256k
5. Merge `cp.zip` and `ap.zip` into `total.zip`
6. Prepare a 0-byte empty file, assuming it is `dummy.bin`
7. Execute `soc_tools.exe` and synthesize `total.zip` and `dummy.bin` into the final upgrade package

For specific implementation code, please refer to the code in the `tools\dtools` directory.

## Generation method - LuatOS differential upgrade package

Suitable for `LuatOS firmware`, mainly:

1. In different versions, the bottom layer may be the same, that is, the binpkg file, but the data in the script area is different.
2. Only the script area is inconsistent, and an upgrade package needs to be generated.
3. SOC files are generally in 7z format or zip format. It is recommended to use 7za tool or py7za library to open them, both of which can be automatically recognized.

Generation steps

1. First unzip the old and new soc files to get binpkg
2. Compare the old and new binpkg files. If they are not the same, generate the `original difference package` and get `diff.par`
3. If binpkg is the same, create an empty `diff.par` with a size of 0
4. Execute `soc_tools.exe` and compress `script.bin` into `script.zip`, which is actually in `lzma` format
5. Execute `soc_tools.exe` and synthesize `diff.par` and `script.zip` into the final upgrade package

For specific implementation code, please refer to the code in the `tools\dtools` directory.

## How to know what upgrade package to generate

For the upgrade package corresponding to EC618, the main difficulty lies in whether the CSDK is a differential upgrade or a full package upgrade.

Here are a few ideas for reference only

1. Solution A. Corresponding to the `CSDK` firmware, when compiling the firmware, an additional upgrade package file containing info.json is packaged, which is described through the `fota` table
2. Option B, when creating an IoT project, add fields and ask customers to select specific types.

Attached is a fragment of the fota table in info.json. The `core_type` identifies the difference `diff` or the whole package `full`. The example comes from the ec718 library.

```json
"photo" : {
"magic_num" : "eac37218",
"block_len" : "40000",
"core_type" : "diff",
"ap_type" : "diff",
"cp_type" : "diff",
"full_addr" : "002BD000",
"fota_len" : "69000"
    }
```
