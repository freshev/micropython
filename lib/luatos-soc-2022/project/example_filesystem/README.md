# File system API description

## Overview

* The main body of the file system is littlefs, with a total space of 288k and about 100k available.
* There are multiple sets of APIs, each with its own characteristics, but they should not be mixed.

## Osa File API

Located in `osasys.h`, there are only a few functions, similar to the posix API. No directory operations are involved.

```c
OSAFILE OsaFopen(const char *fileName, const char* mode);
INT32 OsaFtell(OSAFILE fp);
INT32 OsaFclose(OSAFILE fp);
UINT32 OsaFread(void *buf, UINT32 size, UINT32 count, OSAFILE fp);
UINT32 OsaFwrite(void *buf, UINT32 size, UINT32 count, OSAFILE fp);
INT32 OsaFseek(OSAFILE fp, INT32 offset, UINT8 seekType);
UINT32 OsaFremove(const char *fileName);
```

General calling logic:
```c
#define BUFF_SIZE (512)
char buff[BUFF_SIZE];
UINT32 rlen = 0;
OSAFILE fp = OsaFopen("fs_test", "r");
if (fp) { // If successful, must not be NULL
//Read and write data
while ((rlen = OsaFread(buff, BUFF_SIZE, 1, fp)) > 0) {
// Process the read data
    }
    OsaFclose(fp);
}
```

## LiffteFs File API

On the original lifftefs, it is encapsulated into `lfs_port.h`

Notice:

* Do not call `LFS_init` `LFS_deinit`, the bottom layer will handle it by itself
* APIs all start with `LFS_`, which are synchronous blocking and thread-safe functions. They basically correspond to the API of native lifttefs one-to-one

General calling logic:
```c
#define BUFF_SIZE (512)
char buff[BUFF_SIZE];
UINT32 rlen = 0;
int ret = 0;
lfs_file_t *file = malloc(sizeof(lfs_file_t));
if (file == NULL) {
//Memory allocation error
return -1;
}
ret = LFS_fileOpen(file, "fs_test", LFS_O_RDONLY);
if (ret == LFS_ERR_OK) { // If successful, must not be NULL
//Read and write data
while ((rlen = LFS_fileRead(file, buff, BUFF_SIZE)) > 0) {
// Process the read data
    }
    LFS_fileClose(file);
}
free(file); // Pay attention to releasing memory
```
