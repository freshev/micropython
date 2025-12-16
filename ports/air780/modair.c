/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Paul Sokolovsky
 * Copyright (c) 2024 freshev
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdio.h>

#include "py/gc.h"
#include "py/runtime.h"
#include "py/persistentcode.h"
#include "py/mperrno.h"
#include "py/mphal.h"
#include "py/stream.h"
#include "extmod/vfs.h"

#include "luat_flash.h"
#include "luat_debug.h"
#include "luat_fs.h"
#include "luat_mem.h"
#include "mem_map.h"

/*
The overall layout of Flash is as follows, corresponding to non-LuatOS applications:

AP flash layout, total 4MB
flash raw address: 0x00000000---0x00400000
flash xip address(from ap view): 0x00800000---0x00c00000


0x00000000 |---------------------------------|
                    | header1 8KB |
0x00002000 |---------------------------------|
                    | header2 8KB |
0x00004000 |---------------------------------|
                    | bl part1 32KB |
0x0000c000 |---------------------------------|
                    | bl part2 96KB |
0x00024000 |---------------------------------|
                    | app img 2.5MB + 384k |------OTA write
0x00304000 |---------------------------------|
                    | fota 512KB |-----OTA download write
0x00384000 |---------------------------------|
                    | lfs 288KB |-----FS write
0x003cc000 |---------------------------------|
                    | kv 64KB |-----kv write
0x003dc000 |---------------------------------|
                    | rel_ap(factory) 16KB |-----factory write
0x003e0000 |---------------------------------|
                    | rel_ap 16KB |-----factory write
0x003e4000 |---------------------------------|
                    | hib backup 96KB |-----hib write
0x003fc000 |---------------------------------|
                    | plat config 16KB |-----similar as FS
0x00400000 |---------------------------------|


**Notice**:
1. If you want to read data directly, you can read it directly through the above address +0x00800000, without going through luat_flash.h
2. If you need to write or erase data, you need to call luat_flash.h

As you can see from the picture above, there are only two areas that can be moved:
1. KV database area, if you do not use luat_kv at all, you can use it directly
2. AP space, if you determine that you do not need to use all the space, you can crop it, but you need to change mem_map.h


#define AP_VIEW_CPFLASH_XIP_ADDR        (0x08800000)
#define AP_FLASH_XIP_ADDR               (0x00800000)

//bl addr and size
#define BOOTLOADER_FLASH_LOAD_ADDR      (0x00804000)
#ifdef __USER_CODE__
#define BOOTLOADER_FLASH_LOAD_SIZE      (0x1e000)//120kB
#else
#define BOOTLOADER_FLASH_LOAD_SIZE      (0x20000)//128kB
#endif
//ap image addr and size
#define AP_FLASH_LOAD_ADDR              (0x00824000)
#ifdef __USER_CODE__
#define AP_FLASH_LOAD_SIZE              (0x2E0000)//2.5MB + 384KB
#else
#define AP_FLASH_LOAD_SIZE              (0x280000)//2.5MB
#endif

// 0x002a4000 -----0x00304000 RESERVRD 384KB

//fota addr and size
#define FLASH_FOTA_REGION_START         (0x304000)
#define FLASH_FOTA_REGION_LEN           (0x80000)//512KB
#define FLASH_FOTA_REGION_END           (0x384000)

#ifdef __USER_CODE__

//fs addr and size
#define FLASH_FS_REGION_START           (0x384000)
#define FLASH_FS_REGION_END             (0x3cc000)
#define FLASH_FS_REGION_SIZE            (FLASH_FS_REGION_END-FLASH_FS_REGION_START) // 352KB
#define FLASH_FDB_REGION_START          (0x3cc000)  //FDB 64KB
#define FLASH_FDB_REGION_END            (0x3dc000)
//softsim addr and size
#define SOFTSIM_FLASH_PHYSICAL_BASEADDR (0xfcc000)
#define SOFTSIM_FLASH_MAX_SIZE          (0x00000)//0KB
#else
#define FLASH_FS_REGION_START           (0x384000)
#define FLASH_FS_REGION_END             (0x3cc000)
#define FLASH_FS_REGION_SIZE            (FLASH_FS_REGION_END-FLASH_FS_REGION_START) // 288KB

//softsim addr and size
#define SOFTSIM_FLASH_PHYSICAL_BASEADDR (0x3cc000)
#define SOFTSIM_FLASH_MAX_SIZE          (0x10000)//64KB
#endif

//ap reliable addr and size
#define NVRAM_FACTORY_PHYSICAL_BASE     (0x3dc000)
#define NVRAM_FACTORY_PHYSICAL_SIZE     (0x4000)//16KB
#define NVRAM_PHYSICAL_BASE             (0x3e0000)
#define NVRAM_PHYSICAL_SIZE             (0x4000)//16KB


//hib bakcup addr and size
#define FLASH_HIB_BACKUP_EXIST          (1)
#define FLASH_MEM_BACKUP_ADDR           (AP_FLASH_XIP_ADDR+0x3e4000)
#define FLASH_MEM_BACKUP_NONXIP_ADDR    (FLASH_MEM_BACKUP_ADDR-AP_FLASH_XIP_ADDR)
#define FLASH_MEM_BLOCK_SIZE            (0x6000)
#define FLASH_MEM_BLOCK_CNT             (0x4)
#define FLASH_MEM_BACKUP_SIZE           (0x18000)//96KB

//plat config addr and size
#define FLASH_MEM_PLAT_INFO_ADDR        (AP_FLASH_XIP_ADDR+0x3fc000)
#define FLASH_MEM_PLAT_INFO_SIZE        (0x1000)//4KB
#define FLASH_MEM_PLAT_INFO_NONXIP_ADDR (FLASH_MEM_PLAT_INFO_ADDR - AP_FLASH_XIP_ADDR)

#define FLASH_MEM_RESET_INFO_ADDR        (AP_FLASH_XIP_ADDR+0x3fd000)
#define FLASH_MEM_RESET_INFO_SIZE        (0x1000)//4KB
#define FLASH_MEM_RESET_INFO_NONXIP_ADDR (FLASH_MEM_RESET_INFO_ADDR - AP_FLASH_XIP_ADDR)


#define CP_FLASH_XIP_ADDR               (0x00800000)

//cp img
#define CP_FLASH_LOAD_ADDR              (0x00800000)
#define CP_FLASH_LOAD_SIZE              (0x80000)//512KB

//for ramdump
#define CP_FLASH_RESV_ADDR              (0x00880000)
//#define CP_FLASH_RESV_PHYSICAL_ADDR     (0x80000)
#define CP_FLASH_RESV_SIZE              (0x4d000)//308KB
//#define CP_FLASH_RESV_NUM_SECTOR        (77)

#define FLASH_EXCEP_DUMP_ADDR           (0x80000)
#define FLASH_EXCEP_DUMP_SECTOR_NUM     (77)
#define FLASH_EXCEP_KEY_INFO_ADDR       (0x3fe000)  //(AP_FLASH_XIP_ADDR+0x3fe000)
#define FLASH_EXCEP_KEY_INFO_LEN        (0x1000)//4KB



#define CP_FLASH_IP2_ADDR               (0x008cd000)
#define CP_FLASH_IP2_SIZE               (0x1000)//4KB

//cp reliable addr and size, cp nvm write by ap
#define CP_NVRAM_FACTORY_PHYSICAL_BASE  (0xce000)
#define CP_NVRAM_FACTORY_PHYSICAL_SIZE  (0x19000)//100KB
#define CP_NVRAM_PHYSICAL_BASE          (0xe7000)
#define CP_NVRAM_PHYSICAL_SIZE          (0x19000)//100KB


#define AP_IMG_MERGE_ADDR               (0x00024000)
#define CP_IMG_MERGE_ADDR               (0x00000000)
#define BL_IMG_MERGE_ADDR               (0x00004000)

#define BLS_SEC_HAED_ADDR               (0x0)
#define BLS_FLASH_LOAD_SIZE             (0x2000)
#define SYS_SEC_HAED_ADDR               (0x2000)
#define SYS_FLASH_LOAD_SIZE             (0x2000)


// external storage device 
//e.g. external flash data addr and size
#define EF_FLASH_XIP_ADDR               (0x80000000)
#define EF_DATA_LOAD_ADDR               (0x00000000)
#define EF_DATA_LOAD_SIZE               (0x100000)//1MB

//e.g. external SD card data addr and size
#define SD_CARD_XIP_ADDR                (0x40000000)
#define SD_DATA_LOAD_ADDR               (0x00000000)
#define SD_DATA_LOAD_SIZE               (0x100000)//1MB
*/

static mp_obj_t modair_flash_read(mp_obj_t offset_in, mp_obj_t len_or_buf_in) {
    mp_int_t offset = mp_obj_get_int(offset_in);

    mp_int_t len;
    byte *buf;
    bool alloc_buf = mp_obj_is_int(len_or_buf_in);

    if (alloc_buf) {
        len = mp_obj_get_int(len_or_buf_in);
        buf = m_new(byte, len);
    } else {
        mp_buffer_info_t bufinfo;
        mp_get_buffer_raise(len_or_buf_in, &bufinfo, MP_BUFFER_WRITE);
        len = bufinfo.len;
        buf = bufinfo.buf;
    }
    
    LUAT_DEBUG_PRINT("flash read at 0x%06x, len: %d", offset, len);

    // We know that allocation will be 4-byte aligned for sure
    int res = luat_flash_read((char *)buf, offset, len);
    if (res > 0) {
        if (alloc_buf) {
            return mp_obj_new_bytes(buf, len);
        }
        return mp_const_none;
    }
    if (alloc_buf) {
        m_del(byte, buf, len);
    }
    mp_raise_OSError(MP_EIO);
}
static MP_DEFINE_CONST_FUN_OBJ_2(modair_flash_read_obj, modair_flash_read);

static mp_obj_t modair_flash_write(mp_obj_t offset_in, const mp_obj_t buf_in) {
    mp_int_t offset = mp_obj_get_int(offset_in);
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(buf_in, &bufinfo, MP_BUFFER_READ);
    if (bufinfo.len & 0x3) {
        mp_raise_ValueError(MP_ERROR_TEXT("len must be multiple of 4"));
    }

    // LUAT_DEBUG_PRINT("flash write at 0x%06x, len: %d", offset, bufinfo.len);
    int  res = luat_flash_write(bufinfo.buf, offset, bufinfo.len);
    if (res > 0) {
        return mp_const_none;
    }
    mp_raise_OSError(MP_EIO);
}
static MP_DEFINE_CONST_FUN_OBJ_2(modair_flash_write_obj, modair_flash_write);

static mp_obj_t modair_flash_erase(mp_obj_t offset_in, mp_obj_t len_in) {
    mp_int_t offset = mp_obj_get_int(offset_in);
    mp_int_t len = mp_obj_get_int(len_in);

    // LUAT_DEBUG_PRINT("flash erase at 0x%06x, len: %d", offset, len);
    int res = luat_flash_erase(offset, len); // offset should be < NVRAM_FACTORY_PHYSICAL_BASE = 0x3dc000
    if (res == 0) {
        return mp_const_none;
    }
    mp_raise_OSError(MP_EIO);
}
static MP_DEFINE_CONST_FUN_OBJ_2(modair_flash_erase_obj, modair_flash_erase);

static mp_obj_t modair_flash_size(void) {
    return mp_obj_new_int_from_uint(MSMB_START_ADDR); // 4MB
}
static MP_DEFINE_CONST_FUN_OBJ_0(modair_flash_size_obj, modair_flash_size);


static mp_obj_t modair_flash_user_start(void) {
    return MP_OBJ_NEW_SMALL_INT((uint32_t)FLASH_FS_REGION_START);
}
static MP_DEFINE_CONST_FUN_OBJ_0(modair_flash_user_start_obj, modair_flash_user_start);


static mp_obj_t modair_flash_user_end(void) {
    return MP_OBJ_NEW_SMALL_INT((uint32_t)FLASH_FS_REGION_END);
}
static MP_DEFINE_CONST_FUN_OBJ_0(modair_flash_user_end_obj, modair_flash_user_end);

static mp_obj_t modair_freemem() {
    size_t total;
    size_t used;
    size_t max_used;
    luat_meminfo_sys(&total, &used, &max_used);
    return MP_OBJ_NEW_SMALL_INT((uint32_t)(total - used));
}
static MP_DEFINE_CONST_FUN_OBJ_0(modair_freemem_obj, modair_freemem);

static mp_obj_t modair_meminfo() {
    size_t total;
    size_t used;
    size_t max_used;
    luat_meminfo_sys(&total, &used, &max_used);
    mp_printf(&mp_plat_print, "Total: %d, Used: %d, Max used: %d\n", total, used, max_used);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(modair_meminfo_obj, modair_meminfo);

static mp_obj_t modair_malloc(mp_obj_t size_in) {
    return MP_OBJ_NEW_SMALL_INT((mp_uint_t)luat_heap_malloc(mp_obj_get_int(size_in))); // ???
}
static MP_DEFINE_CONST_FUN_OBJ_1(modair_malloc_obj, modair_malloc);

static mp_obj_t modair_free(mp_obj_t addr_in) {
    luat_heap_free((void *)mp_obj_get_int(addr_in));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(modair_free_obj, modair_free);

// --------------------------------------------------------
//                  Flash file system API
// --------------------------------------------------------

mp_obj_t modair_flash_mount(mp_obj_t arg1, mp_obj_t arg2) {
    // LFS_init LFS_deinit LFS_format functions are prohibited
    (void) arg1;
    (void) arg2;
    int res = luat_fs_ready();
    if (res == 0) mp_raise_OSError(MP_EBUSY);
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_2(modair_flash_mount_obj, modair_flash_mount);
MP_DEFINE_CONST_STATICMETHOD_OBJ(modair_flash_mount_static_class_obj, &modair_flash_mount_obj);

mp_obj_t modair_flash_umount() {
    // LFS_init LFS_deinit LFS_format functions are prohibited
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_0(modair_flash_umount_obj, modair_flash_umount);
MP_DEFINE_CONST_STATICMETHOD_OBJ(modair_flash_umount_static_class_obj, &modair_flash_umount_obj);

typedef struct _mp_air_lfs_ilistdir_it_t {
    mp_obj_base_t base;
    mp_fun_1_t iternext;
    int16_t fs_index;
    char *dir;
} mp_air_lfs_ilistdir_it_t;


static mp_obj_t modair_ilistdir_it_iternext(mp_obj_t self_in) {
    mp_air_lfs_ilistdir_it_t *self = MP_OBJ_TO_PTR(self_in);
    luat_fs_dirent_t *fs_entry = luat_heap_malloc(sizeof(luat_fs_dirent_t));
    if(fs_entry != NULL) {
        memset(fs_entry, 0, sizeof(luat_fs_dirent_t));
        
        int res = luat_fs_lsdir(self->dir, fs_entry, self->fs_index ,1); // skip fs_index files, read 1 fs_entry
        // LUAT_DEBUG_PRINT("lsdir->next %s: %s, index = %d, result = %d", self->dir, fs_entry->d_name, self->fs_index, res);
        
        if(res == 1) {
            mp_obj_tuple_t *t = MP_OBJ_TO_PTR(mp_obj_new_tuple(3, NULL));
            t->items[0] = mp_obj_new_str(fs_entry->d_name, strlen(fs_entry->d_name)),
            t->items[1] = MP_OBJ_NEW_SMALL_INT(fs_entry->d_type == 0 ? MP_S_IFREG : MP_S_IFDIR),
            t->items[2] = MP_OBJ_NEW_SMALL_INT(0), // no inode number
            luat_heap_free(fs_entry); 
            fs_entry = NULL; 
            self->fs_index++;
            return MP_OBJ_FROM_PTR(t);
        }
        luat_heap_free(fs_entry);
        fs_entry = NULL;
    }
    return MP_OBJ_STOP_ITERATION;
}

mp_obj_t modair_flash_ilistdir(mp_obj_t path_in) {
    const char* path = mp_obj_str_get_str(path_in);
    if (strlen(path) == 0) path = mp_obj_str_get_str(MP_OBJ_NEW_QSTR(MP_QSTR_));
    else if (!luat_fs_dexist(path)) mp_raise_OSError(MP_ENOTDIR);

    mp_air_lfs_ilistdir_it_t *iter = mp_obj_malloc(mp_air_lfs_ilistdir_it_t, &mp_type_polymorph_iter);
    iter->iternext = modair_ilistdir_it_iternext;
    iter->dir = (char*)path;
    iter->fs_index = 2; // skip "." and ".." folders
    // LUAT_DEBUG_PRINT("lsdir %s:", iter->dir);
    return MP_OBJ_FROM_PTR(iter);
}

MP_DEFINE_CONST_FUN_OBJ_1(modair_flash_ilistdir_obj, modair_flash_ilistdir);
MP_DEFINE_CONST_STATICMETHOD_OBJ(modair_flash_ilistdir_static_class_obj, &modair_flash_ilistdir_obj);


mp_obj_t modair_flash_mkdir(mp_obj_t path_in) {
    const char* path = mp_obj_str_get_str(path_in);
    if (!luat_fs_dexist(path)) {
        int res = luat_fs_mkdir(path);
        if (res != 0) mp_raise_OSError(MP_EIO);
    } else mp_raise_OSError(MP_EEXIST);
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(modair_flash_mkdir_obj, modair_flash_mkdir);
MP_DEFINE_CONST_STATICMETHOD_OBJ(modair_flash_mkdir_static_class_obj, &modair_flash_mkdir_obj);


mp_obj_t modair_flash_remove(mp_obj_t path_in) {
    const char* path = mp_obj_str_get_str(path_in);
    if (luat_fs_fexist(path)) {
        int res = luat_fs_remove(path);
        if (res != 0) mp_raise_OSError(MP_EIO);
    } else mp_raise_OSError(MP_ENOENT);
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(modair_flash_remove_obj, modair_flash_remove);
MP_DEFINE_CONST_STATICMETHOD_OBJ(modair_flash_remove_static_class_obj, &modair_flash_remove_obj);


mp_obj_t modair_flash_rename(mp_obj_t old_path_in, mp_obj_t new_path_in) {
    const char* path_old = mp_obj_str_get_str(old_path_in);
    const char* path_new = mp_obj_str_get_str(new_path_in);
    //ensure_exists(path_old);
    if (luat_fs_fexist(path_old)) {
        int res = luat_fs_rename(path_old, path_new);
        if (res != 0) mp_raise_OSError(MP_EIO);
    } else mp_raise_OSError(MP_ENOENT);
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_2(modair_flash_rename_obj, modair_flash_rename);
MP_DEFINE_CONST_STATICMETHOD_OBJ(modair_flash_rename_static_class_obj, &modair_flash_rename_obj);


mp_obj_t modair_flash_rmdir(mp_obj_t path_in) {
    const char* path = mp_obj_str_get_str(path_in);
    if (luat_fs_dexist(path)) {
        int res = luat_fs_rmdir(path);
        if (res != 0) mp_raise_OSError(MP_EIO);
    } else mp_raise_OSError(MP_ENOENT);
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(modair_flash_rmdir_obj, modair_flash_rmdir);
MP_DEFINE_CONST_STATICMETHOD_OBJ(modair_flash_rmdir_static_class_obj, &modair_flash_rmdir_obj);

mp_obj_t modair_flash_stat(mp_obj_t path_in) {
    const char* path = mp_obj_str_get_str(path_in);
    int32_t size = 0;
    mp_int_t st_mode = 0;
    int32_t fd = NULL;

    //LUAT_DEBUG_PRINT("dexist %s, %d", path, luat_fs_dexist(path));
    //LUAT_DEBUG_PRINT("fexist %s, %d", path, luat_fs_fexist(path)); // reboots if luat_fs_dexist(path) == true 

    if (luat_fs_dexist(path)) {
        st_mode = MP_S_IFDIR;
    }
    else if (luat_fs_fexist(path)) {
        st_mode = MP_S_IFREG;
        size = luat_fs_fsize(path);
    }
    else mp_raise_OSError(MP_ENOENT);

    mp_obj_tuple_t *t = MP_OBJ_TO_PTR(mp_obj_new_tuple(10, NULL));
    t->items[0] = MP_OBJ_NEW_SMALL_INT(st_mode); // st_mode
    for (int i = 1; i <= 9; ++i) {
        if (i == 6) t->items[i] = MP_OBJ_NEW_SMALL_INT(size);
        else t->items[i] = MP_OBJ_NEW_SMALL_INT(0); // dev, nlink, uid, gid, size, atime, mtime, ctime
    }
    return MP_OBJ_FROM_PTR(t);
}

MP_DEFINE_CONST_FUN_OBJ_1(modair_flash_stat_obj, modair_flash_stat);
MP_DEFINE_CONST_STATICMETHOD_OBJ(modair_flash_stat_static_class_obj, &modair_flash_stat_obj);

mp_obj_t modair_flash_statvfs(mp_obj_t path_in) {
    const char* path = mp_obj_str_get_str(path_in);
    luat_fs_info_t fs_info = {0};
    int res = luat_fs_info(path, &fs_info);
    if (res != 0) mp_raise_OSError(MP_EIO);

    mp_obj_tuple_t *t = MP_OBJ_TO_PTR(mp_obj_new_tuple(10, NULL));
    t->items[0] = MP_OBJ_NEW_SMALL_INT(fs_info.block_size); // bsize
    t->items[1] = MP_OBJ_NEW_SMALL_INT(0);                  // frsize
    t->items[2] = MP_OBJ_NEW_SMALL_INT(fs_info.total_block);// blocks
    t->items[3] = MP_OBJ_NEW_SMALL_INT(fs_info.total_block - fs_info.block_used);// bfree
    t->items[4] = MP_OBJ_NEW_SMALL_INT(fs_info.total_block - fs_info.block_used);// bavail
    t->items[5] = MP_OBJ_NEW_SMALL_INT(0); // files
    t->items[6] = MP_OBJ_NEW_SMALL_INT(0); // ffree
    t->items[7] = MP_OBJ_NEW_SMALL_INT(0); // favail
    t->items[8] = MP_OBJ_NEW_SMALL_INT(0); // flags
    t->items[9] = MP_OBJ_NEW_SMALL_INT(MICROPY_ALLOC_PATH_MAX);
    return MP_OBJ_FROM_PTR(t);
}

MP_DEFINE_CONST_FUN_OBJ_1(modair_flash_statvfs_obj, modair_flash_statvfs);
MP_DEFINE_CONST_STATICMETHOD_OBJ(modair_flash_statvfs_static_class_obj, &modair_flash_statvfs_obj);

// --------------------------------------------------------
//                  Flash file API
// --------------------------------------------------------
typedef struct _air_file_obj_t {
    mp_obj_base_t base;
    FILE *f;
} air_file_obj_t;

const mp_obj_type_t modair_fileio;
const mp_obj_type_t modair_textio;

static void modair_file_print(const mp_print_t * print, mp_obj_t self_in, mp_print_kind_t kind) {
    (void)self_in;
    (void)kind;
    mp_printf(print, "<io.%s>", mp_obj_get_type_str(self_in));
}

static mp_obj_t modair_file_open_internal(mp_obj_t path_in, mp_obj_t mode_in) {
    const char *path = mp_obj_str_get_str(path_in);
    const mp_obj_type_t *type = &modair_textio;
    const char *mode = mp_obj_str_get_str(mode_in);
    const char *mode_str = mode;
    for (; *mode_str; ++mode_str) {
        switch (*mode_str) {
            case 'r':break;
            case 'w':break;
            case 'x':break;
            case 'a':break;
            case '+':break;
            case 'b':type = &modair_fileio;break;
            case 't':type = &modair_textio;break;
        }
    }
    //air_file_obj_t *o = m_new_obj_with_finaliser(air_file_obj_t);
    air_file_obj_t *o = m_new_obj(air_file_obj_t);
    o->base.type = type;

    FILE *f = luat_fs_fopen(path, mode);
    if (f == NULL) {
        mp_raise_OSError(MP_ENOENT);
    }
    o->f = f;
    // LUAT_DEBUG_PRINT("file open: %p, %s, %s", o->f, path, mode);
    return MP_OBJ_FROM_PTR(o);
}

static mp_uint_t modair_file_read(mp_obj_t self_in, void *buf, mp_uint_t size, int *errcode) {
    air_file_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if(self->f != NULL) {
        int res = luat_fs_fread(buf, size, 1, self->f);
        if (res < 0) {
            *errcode = -MP_EIO;
            return MP_STREAM_ERROR;
        }
        return (mp_uint_t)res;
    } else {
        *errcode = MP_EBADF;
        return MP_STREAM_ERROR;
    }
}

static mp_uint_t modair_file_write(mp_obj_t self_in, const void *buf, mp_uint_t size, int *errcode) {
    air_file_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if(self->f != NULL) {
        int res = luat_fs_fwrite(buf, size, 1, self->f);
        if (res < 0) {
            *errcode = -MP_EIO;
            return MP_STREAM_ERROR;
        }
        if (res != (int)size) {
            *errcode = MP_ENOSPC;
            return MP_STREAM_ERROR;
        }
        return (mp_uint_t)res;
    } else {
        *errcode = MP_EBADF;
        return MP_STREAM_ERROR;
    }
}

static mp_uint_t modair_file_ioctl(mp_obj_t self_in, mp_uint_t request, uintptr_t arg, int *errcode) {
    air_file_obj_t *self = MP_OBJ_TO_PTR(self_in);

    if (request == MP_STREAM_SEEK) {
        if(self->f != NULL) {
            struct mp_stream_seek_t *s = (struct mp_stream_seek_t *)(uintptr_t)arg;
            int res = luat_fs_fseek(self->f, s->offset, s->whence);
            if (res < 0) {
                *errcode = -MP_EIO;
                return MP_STREAM_ERROR;
            }
            res = luat_fs_ftell(self->f);
            if (res < 0) {
                *errcode = -MP_EIO;
                return MP_STREAM_ERROR;
            }
            s->offset = res;
            return 0;
        } else {
            *errcode = MP_EBADF;
            return MP_STREAM_ERROR;
        }
    } else if (request == MP_STREAM_FLUSH) {
        if(self->f != NULL) {
            int res = luat_fs_fflush(self->f);
            if (res < 0) {
                *errcode = -MP_EIO;
                return MP_STREAM_ERROR;
            }
            return 0;
        } else {
            *errcode = MP_EBADF;
            return MP_STREAM_ERROR;
        }
    } else if (request == MP_STREAM_CLOSE) {
        if(self->f != NULL) {
            int res = luat_fs_fclose(self->f);
            if (res < 0) {
                *errcode = -MP_EIO;
                return MP_STREAM_ERROR;
            }
        }
        self->f = NULL;
        return 0;
    } else {
        *errcode = MP_EINVAL;
        return MP_STREAM_ERROR;
    }
}

static const mp_rom_map_elem_t modair_file_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&mp_stream_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_readinto), MP_ROM_PTR(&mp_stream_readinto_obj) },
    { MP_ROM_QSTR(MP_QSTR_readline), MP_ROM_PTR(&mp_stream_unbuffered_readline_obj) },
    { MP_ROM_QSTR(MP_QSTR_readlines), MP_ROM_PTR(&mp_stream_unbuffered_readlines_obj) },
    { MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&mp_stream_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_flush), MP_ROM_PTR(&mp_stream_flush_obj) },
    { MP_ROM_QSTR(MP_QSTR_close), MP_ROM_PTR(&mp_stream_close_obj) },
    { MP_ROM_QSTR(MP_QSTR_seek), MP_ROM_PTR(&mp_stream_seek_obj) },
    { MP_ROM_QSTR(MP_QSTR_tell), MP_ROM_PTR(&mp_stream_tell_obj) },
    { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&mp_stream_close_obj) },
    { MP_ROM_QSTR(MP_QSTR___enter__), MP_ROM_PTR(&mp_identity_obj) },
    { MP_ROM_QSTR(MP_QSTR___exit__), MP_ROM_PTR(&mp_stream___exit___obj) },
};
static MP_DEFINE_CONST_DICT(modair_file_locals_dict, modair_file_locals_dict_table);

static const mp_stream_p_t modair_fileio_stream_p = {
    .read = modair_file_read,
    .write = modair_file_write,
    .ioctl = modair_file_ioctl,
};

MP_DEFINE_CONST_OBJ_TYPE(
    modair_fileio,
    MP_QSTR_FileIO,
    MP_TYPE_FLAG_ITER_IS_STREAM,
    print, modair_file_print,
    protocol, &modair_fileio_stream_p,
    locals_dict, &modair_file_locals_dict
);

static const mp_stream_p_t modair_textio_stream_p = {
    .read = modair_file_read,
    .write = modair_file_write,
    .ioctl = modair_file_ioctl,
    .is_text = true,
};

MP_DEFINE_CONST_OBJ_TYPE(
    modair_textio,
    MP_QSTR_TextIOWrapper,
    MP_TYPE_FLAG_ITER_IS_STREAM,
    print, modair_file_print,
    protocol, &modair_textio_stream_p,
    locals_dict, &modair_file_locals_dict
);


mp_obj_t modair_file_open(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_file, ARG_mode, ARG_encoding };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_file, MP_ARG_OBJ | MP_ARG_REQUIRED, {.u_rom_obj = MP_ROM_PTR(MP_ROM_NONE)} },
        { MP_QSTR_mode, MP_ARG_OBJ, {.u_rom_obj = MP_ROM_QSTR(MP_QSTR_r)} },
        { MP_QSTR_buffering, MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_encoding, MP_ARG_OBJ, {.u_rom_obj = MP_ROM_PTR(MP_ROM_NONE)} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
    //const char* path = mp_obj_str_get_str(args[ARG_file].u_obj);
    //const char* mode = mp_obj_str_get_str(args[ARG_mode].u_obj);
    //const mp_obj_type_t* type = &mp_type_internal_flash_textio;
    //return internal_flash_file_open(file_name, type, args);    
    //LUAT_DEBUG_PRINT("file open: %s, %s", path, mode);
    return modair_file_open_internal(args[ARG_file].u_obj, args[ARG_mode].u_obj);
}

MP_DEFINE_CONST_FUN_OBJ_KW(modair_file_open_obj, 1, modair_file_open);
MP_DEFINE_CONST_STATICMETHOD_OBJ(modair_flash_open_static_class_obj, &modair_file_open_obj);

// --------------------------------------------------------
//                Micropython flash bindings
// --------------------------------------------------------

static const mp_rom_map_elem_t modair_flash_locals_dict_table[] = {
  //{ MP_ROM_QSTR(MP_QSTR_open), MP_ROM_PTR(&modair_flash_open_static_class_obj) },
    //{ MP_ROM_QSTR(MP_QSTR_open), MP_ROM_PTR(&modair_file_open_obj) },
    { MP_ROM_QSTR(MP_QSTR_open), MP_ROM_PTR(&modair_flash_open_static_class_obj) },

    { MP_ROM_QSTR(MP_QSTR_mount), MP_ROM_PTR(&modair_flash_mount_static_class_obj) },
    { MP_ROM_QSTR(MP_QSTR_umount), MP_ROM_PTR(&modair_flash_umount_static_class_obj) },
    { MP_ROM_QSTR(MP_QSTR_ilistdir), MP_ROM_PTR(&modair_flash_ilistdir_static_class_obj) },
    { MP_ROM_QSTR(MP_QSTR_mkdir), MP_ROM_PTR(&modair_flash_mkdir_static_class_obj) },
    { MP_ROM_QSTR(MP_QSTR_remove), MP_ROM_PTR(&modair_flash_remove_static_class_obj) },
    { MP_ROM_QSTR(MP_QSTR_rename), MP_ROM_PTR(&modair_flash_rename_static_class_obj) },
    { MP_ROM_QSTR(MP_QSTR_rmdir), MP_ROM_PTR(&modair_flash_rmdir_static_class_obj) },
    { MP_ROM_QSTR(MP_QSTR_stat), MP_ROM_PTR(&modair_flash_stat_static_class_obj) },
    { MP_ROM_QSTR(MP_QSTR_statvfs), MP_ROM_PTR(&modair_flash_statvfs_static_class_obj) },
};

static MP_DEFINE_CONST_DICT(modair_flash_locals_dict, modair_flash_locals_dict_table);


MP_DEFINE_CONST_OBJ_TYPE(
    modair_flash_type,
    MP_QSTR_flash,
    MP_TYPE_FLAG_NONE,
    locals_dict, &modair_flash_locals_dict
    );


static const mp_rom_map_elem_t modair_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_air) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_flash), (mp_obj_t)MP_ROM_PTR(&modair_flash_type) },
    { MP_ROM_QSTR(MP_QSTR_flash_read), MP_ROM_PTR(&modair_flash_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_flash_write), MP_ROM_PTR(&modair_flash_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_flash_erase), MP_ROM_PTR(&modair_flash_erase_obj) },
    { MP_ROM_QSTR(MP_QSTR_flash_size), MP_ROM_PTR(&modair_flash_size_obj) },
    { MP_ROM_QSTR(MP_QSTR_flash_user_start), MP_ROM_PTR(&modair_flash_user_start_obj) },
    { MP_ROM_QSTR(MP_QSTR_flash_user_end), MP_ROM_PTR(&modair_flash_user_end_obj) },
    { MP_ROM_QSTR(MP_QSTR_freemem), MP_ROM_PTR(&modair_freemem_obj) },
    { MP_ROM_QSTR(MP_QSTR_meminfo), MP_ROM_PTR(&modair_meminfo_obj) },
    { MP_ROM_QSTR(MP_QSTR_malloc), MP_ROM_PTR(&modair_malloc_obj) },
    { MP_ROM_QSTR(MP_QSTR_free), MP_ROM_PTR(&modair_free_obj) },
};

static MP_DEFINE_CONST_DICT(modair_module_globals, modair_module_globals_table);

const mp_obj_module_t modair_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&modair_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR_air, modair_module);
