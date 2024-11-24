#include "luat_base.h"
#include "sqlite3.h"
#include "luat_fs.h"
#include "luat_mem.h"
#include "luat_rtos.h"
#include "luat_crypto.h"


#define LUAT_LOG_TAG "sqlite3"
#include "luat_log.h"

#ifndef LUAT_SQLITE3_DEBUG
#define LUAT_SQLITE3_DEBUG 0
#endif

#if LUAT_SQLITE3_DEBUG == 0 
#undef LLOGD
#undef LLOGDUMP
#define LLOGD(...)
#define LLOGDUMP(...)
#endif

static int svfs_Open(sqlite3_vfs* ctx, sqlite3_filename zName, sqlite3_file*, int flags, int *pOutFlags);
static int svfs_Delete(sqlite3_vfs*, const char *zName, int syncDir);
static int svfs_Access(sqlite3_vfs* ctx, const char *zName, int flags, int *pResOut);
static int svfs_FullPathname(sqlite3_vfs* ctx, const char *zName, int nOut, char *zOut);
static int svfs_Randomness(sqlite3_vfs* ctx, int nByte, char *zOut);
static int svfs_Sleep(sqlite3_vfs* ctx, int microseconds);
static int svfs_CurrentTime(sqlite3_vfs* ctx, double*);
static int svfs_GetLastError(sqlite3_vfs* ctx, int, char *);

// vfs io
static int svfs_io_Close(sqlite3_file*);
static int svfs_io_Read(sqlite3_file*, void*, int iAmt, sqlite3_int64 iOfst);
static int svfs_io_Write(sqlite3_file*, const void*, int iAmt, sqlite3_int64 iOfst);
static int svfs_io_Truncate(sqlite3_file*, sqlite3_int64 size);
static int svfs_io_Sync(sqlite3_file*, int flags);
static int svfs_io_FileSize(sqlite3_file*, sqlite3_int64 *pSize);
static int svfs_io_Lock(sqlite3_file*, int);
static int svfs_io_Unlock(sqlite3_file*, int);
static int svfs_io_CheckReservedLock(sqlite3_file*, int *pResOut);
static int svfs_io_FileControl(sqlite3_file*, int op, void *pArg);
static int svfs_io_SectorSize(sqlite3_file*);
static int svfs_io_DeviceCharacteristics(sqlite3_file*);



static const sqlite3_io_methods svfs_io = {
    .iVersion = 1,
    .xClose = svfs_io_Close,
    .xRead = svfs_io_Read,
    .xWrite = svfs_io_Write,
    .xTruncate = svfs_io_Truncate,
    .xSync = svfs_io_Sync,
    .xFileSize = svfs_io_FileSize,
    .xLock = svfs_io_Lock,
    .xUnlock = svfs_io_Unlock,
    .xCheckReservedLock = svfs_io_CheckReservedLock,
    .xFileControl = svfs_io_FileControl,
    .xSectorSize = svfs_io_SectorSize,
    .xDeviceCharacteristics = svfs_io_DeviceCharacteristics
};

typedef struct sfile
{
    sqlite3_file sf;
    const char* path;
    int flags;
}sfile_t;


static sqlite3_vfs svfs = {
    .iVersion = 1,
    .mxPathname = 63,
    .szOsFile = sizeof(sfile_t),
    .zName = "luatos",
    .xOpen = svfs_Open,
    .xDelete = svfs_Delete,
    .xAccess = svfs_Access,
    .xFullPathname = svfs_FullPathname,
    .xRandomness = svfs_Randomness,
    .xSleep = svfs_Sleep,
    .xCurrentTime = svfs_CurrentTime,
    .xGetLastError = svfs_GetLastError
};

int sqlite3_os_init(void) {
    sqlite3_vfs_register(&svfs, 1);
    return 0;
};

int sqlite3_os_end(void) {
    sqlite3_vfs_unregister(&svfs);
    return 0;
}

#if 0
static void * luat_sqlite3_Malloc(int);         /* Memory allocation function */
static  void luat_sqlite3_Free(void*);          /* Free a prior allocation */
static  void *luat_sqlite3_Realloc(void*,int);  /* Resize an allocation */
static  int luat_sqlite3_Size(void*);           /* Return the size of an allocation */
static  int luat_sqlite3_Roundup(int);          /* Round up request size to allocation size */
static  int luat_sqlite3_Init(void*);           /* Initialize the memory allocator */
static  void luat_sqlite3_xShutdown(void*);      /* Deinitialize the memory allocator */

static const sqlite3_mem_methods mems = {
  .xMalloc = luat_sqlite3_Malloc,
  .xFree = luat_sqlite3_Free,
  .xRealloc = luat_sqlite3_Realloc,
  .xSize = luat_sqlite3_Size,
  .xRoundup = luat_sqlite3_Roundup,
  .xInit = luat_sqlite3_Init,
  .xShutdown = luat_sqlite3_xShutdown,
  NULL
};
#endif

int luat_sqlite3_init(void) {
    // sqlite3_config(SQLITE_CONFIG_MALLOC, &mems);
    return sqlite3_initialize();
}

static FILE* sopen(const char* zName, int flags) {
    // size_t len = luat_fs_fsize(zName);
    LLOGD("Open file %s %d %d", zName, flags, len);
    FILE* fd = NULL;
    if (!luat_fs_fexist(zName)) {
        fd = luat_fs_fopen(zName, "wb");
        if (fd) {
            luat_fs_fclose(fd);
        }
    }
    fd = luat_fs_fopen(zName, "rb+");
    if (fd == NULL) {
        LLOGW("File opening failed %s", zName);
    }
    LLOGD("File handle %s %p", zName, fd);
    return fd;
}

static int svfs_Open(sqlite3_vfs* ctx, sqlite3_filename zName, sqlite3_file* f, int flags, int *pOutFlags) {
    FILE* fd = sopen(zName, flags);
    if (fd == NULL) {
        return SQLITE_NOTFOUND;
    }
    luat_fs_fclose(fd);
    sfile_t* t = (sfile_t*)f;
    t->sf.pMethods = &svfs_io;
    t->path = zName;
    t->flags = flags;
    return SQLITE_OK;
}
static int svfs_Delete(sqlite3_vfs* ctx, const char *zName, int syncDir) {
    LLOGD("Delete file %s", zName);
    luat_fs_remove(zName);
    return SQLITE_OK;
}
static int svfs_Access(sqlite3_vfs* ctx, const char *zName, int flags, int *pResOut) {
    // LLOGD("Access file %s %d", zName, flags);
    return SQLITE_OK;
}
static int svfs_FullPathname(sqlite3_vfs* ctx, const char *zName, int nOut, char *zOut) {
    memcpy(zOut, zName, strlen(zName));
    zOut[strlen(zName)] = 0x00;
    return SQLITE_OK;
}
static int svfs_Randomness(sqlite3_vfs* ctx, int nByte, char *zOut) {
    // LLOGD("Random number generation length %d", nByte);
    luat_crypto_trng(zOut, nByte);
    return SQLITE_OK;
}
static int svfs_Sleep(sqlite3_vfs* ctx, int microseconds) {
    LLOGD("Sleep microseconds %d", microseconds);
    return SQLITE_OK;
}
static int svfs_CurrentTime(sqlite3_vfs* ctx, double* t) {
    *t = 0.0;
    return SQLITE_OK;
}
static int svfs_GetLastError(sqlite3_vfs* ctx, int err, char * msg) {
    LLOGD("Get the last error %d", err);
    return SQLITE_OK;
}

// vfs io
static int svfs_io_Close(sqlite3_file* f) {
    sfile_t* t = (sfile_t*)f;
    if (!t)
        return -1;
    LLOGD("File closed %s", t->path);
    return 0;
}

static int svfs_io_Read(sqlite3_file* f, void* data, int iAmt, sqlite3_int64 iOfst) {
    int ret = 0;
    sfile_t* t = (sfile_t*)f;
    char* ptr = data;
    if (ptr == NULL) {
        LLOGD("io read target address is NULL");
        return SQLITE_IOERR;
    }
    memset(ptr, 0, iAmt);
    FILE* fd = sopen(t->path, t->flags);
    if (fd == NULL) {
        LLOGD("Reading target file does not exist %s", t->path);
        return SQLITE_NOTFOUND;
    }
    LLOGD("File read %s length %d offset %d", t->path, iAmt, iOfst);
    ret = luat_fs_fseek(fd, iOfst, SEEK_SET);
    if (ret < 0) {
        luat_fs_fclose(fd);
        LLOGE("Failed to read location seek %s %d", t->path, ret);
        return SQLITE_IOERR_READ;
    }
    ret = luat_fs_fread(data, iAmt, 1, fd);
    if (ret < 0) {
        luat_fs_fclose(fd);
        LLOGE("Failed to read data %s %d", t->path, ret);
        return SQLITE_IOERR_READ;
    }
    if (ret != iAmt) {
        luat_fs_fclose(fd);
        LLOGD("The read length is insufficient %s %d %d", t->path, ret, iAmt);
        return SQLITE_IOERR_SHORT_READ;
    }
    LLOGD("Read completed %p length %d offset %d", t->path, iAmt, iOfst);
    LLOGDUMP(ptr, iAmt > 48 ? 48 : iAmt);
    luat_fs_fclose(fd);
    return SQLITE_OK;
}

static int svfs_io_Write(sqlite3_file* f, const void* data, int iAmt, sqlite3_int64 iOfst) {
    int ret = 0;
    sfile_t* t = (sfile_t*)f;
    const char* ptr = data;
    if (ptr == NULL) {
        LLOGD("io write target address is NULL");
        return SQLITE_IOERR;
    }
    FILE* fd = sopen(t->path, t->flags);
    if (fd == NULL) {
        LLOGD("io write target file does not exist %s", t->path);
        return SQLITE_NOTFOUND;
    }
    LLOGD("File writing pointer %p length %d offset %d", data, iAmt, iOfst);
    ret = luat_fs_fseek(fd, iOfst, SEEK_SET);
    if (ret < 0) {
        luat_fs_fclose(fd);
        LLOGE("File write setting offset failed %d", ret);
        return SQLITE_IOERR_WRITE;
    }
    ret = luat_fs_fwrite(data, iAmt, 1, fd);
    if (ret < 0) {
        luat_fs_fclose(fd);
        LLOGE("File writing failed %s %d", t->path, ret);
        return SQLITE_IOERR_WRITE;
    }
    if (ret != iAmt) {
        luat_fs_fclose(fd);
        LLOGE("Insufficient file write length result %d length %d", ret, iAmt);
        return SQLITE_IOERR_WRITE;
    }
    LLOGD("File writing completed %s length %d offset %d", t->path, iAmt, iOfst);
    LLOGDUMP(ptr, iAmt > 48 ? 48 : iAmt);
    luat_fs_fclose(fd);
    return SQLITE_OK;
}

static int svfs_io_Truncate(sqlite3_file* f, sqlite3_int64 size) {
    sfile_t* t = (sfile_t*)f;
    int ret = 0;
    ret = luat_fs_truncate(t->path, size);
    if (ret == 0)
        return SQLITE_OK;
    return SQLITE_IOERR_TRUNCATE;
}

static int svfs_io_Sync(sqlite3_file* f, int flags) {
    return 0;
}

static int svfs_io_FileSize(sqlite3_file* f, sqlite3_int64 *pSize) {
    sfile_t* t = (sfile_t*)f;
    *pSize = luat_fs_fsize(t->path);
    // LLOGD("Read file size %s %d", t->path, *pSize);
    return 0;
}

static int svfs_io_Lock(sqlite3_file* f, int t) {
    return 0;
}

static int svfs_io_Unlock(sqlite3_file* f, int t) {
    return 0;
}

static int svfs_io_CheckReservedLock(sqlite3_file* f, int *pResOut) {
    return 0;
}

static int svfs_io_FileControl(sqlite3_file* f, int op, void *pArg) {
    return 0;
}

static int svfs_io_SectorSize(sqlite3_file* f) {
    return 16;
}

static int svfs_io_DeviceCharacteristics(sqlite3_file* f) {
    return SQLITE_IOCAP_ATOMIC;
}

// memory function

#if 0
static void * luat_sqlite3_Malloc(int len) {
    if (len == 0) {
        return NULL;
    }
    char* ptr = luat_heap_malloc(len + sizeof(int));
    if (ptr == NULL) {
        return ptr;
    }
    memcpy(ptr, &len, sizeof(int));
    return ptr + sizeof(int);
}

static void luat_sqlite3_Free(void* ptr) {
    if (ptr == NULL)
        return;
    char* tmp = ptr;
    tmp -= sizeof(int);
    luat_heap_free(tmp);
}
static void *luat_sqlite3_Realloc(void* ptr,int nsize) {
    char* tmp = ptr;
    tmp -= sizeof(int);
    tmp = luat_heap_realloc(tmp, nsize);
    if (tmp == NULL) {
        return NULL;
    }
    memcpy(tmp, &nsize, sizeof(int));
    tmp += sizeof(int);
    return tmp;
}

static int luat_sqlite3_Size(void* ptr) {
    if (ptr == NULL) {
        return 0;
    }
    char* tmp = ptr;
    tmp -= sizeof(int);
    int size = 0;
    memcpy(&size, tmp, sizeof(int));
    return size;
}
static int luat_sqlite3_Roundup(int len) {
    if (len == 0) {
        return 0;
    }
    len = (len + 7) / 8 ;
    len = len * 8;
    return len;
}

static  int luat_sqlite3_Init(void* args) {
    return 0;
}

static  void luat_sqlite3_xShutdown(void* args) {
    return;
}
#endif
