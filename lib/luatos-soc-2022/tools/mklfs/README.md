# File system bin generation tool mklfs

## File description

The source code is in the src directory
mklfs.exe is a compiled exe and can be used directly

## Basic usage

1. First, create a new disk directory and put the files that need to be added into it. Note that folders are not supported.
2. According to the size of the file system partition, execute the following command

```
mklfs.exe -size 288
```

3. After the execution is completed, `disk.fs` will be generated. The suffix does not matter, just rename it to `disk.bin` and it will be used.

## Combined with the description in example_flash, the file system can be flashed

1. The flash address of the file system is 0x384000
