/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Paul Sokolovsky
 * Copyright (c) 2025 freshev
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

#include "iot_flash.h"
#include "iot_debug.h"
#include "iot_fs.h"
#include "hal_config.h"

/*
#define CONFIG_FS_SYS_FLASH_ADDRESS 0x60300000
#define CONFIG_FLASH_SIZE 0x800000
#define CONFIG_FS_SYS_FLASH_OFFSET 0x300000
#define CONFIG_FS_SYS_FLASH_SIZE 0x260000
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
    
    // iot_debug_print("flash read at 0x%06x, len: %d", offset, len);

    // We know that allocation will be 4-byte aligned for sure
    uint32_t ret_len = 0;
    E_AMOPENAT_MEMD_ERR res = iot_flash_read(offset, len, &ret_len, buf);
    if (res == OPENAT_MEMD_ERR_NO) {
        if (alloc_buf) {
            return mp_obj_new_bytes(buf, ret_len);
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

    // iot_debug_print("flash write at 0x%06x, len: %d", offset, bufinfo.len);
    uint32_t ret_len = 0;
    E_AMOPENAT_MEMD_ERR res = iot_flash_write(offset, bufinfo.len, &ret_len, bufinfo.buf);    
    if (res == OPENAT_MEMD_ERR_NO) {
        return mp_const_none;
    }
    mp_raise_OSError(MP_EIO);
}
static MP_DEFINE_CONST_FUN_OBJ_2(modair_flash_write_obj, modair_flash_write);

static mp_obj_t modair_flash_erase(mp_obj_t offset_in, mp_obj_t len_in) {
    mp_int_t offset = mp_obj_get_int(offset_in);
    mp_int_t len = mp_obj_get_int(len_in);

    // iot_debug_print("flash erase at 0x%06x, len: %d", offset, len);
    E_AMOPENAT_MEMD_ERR res = iot_flash_erase(offset, len);
    if (res == OPENAT_MEMD_ERR_NO) {
        return mp_const_none;
    }
    mp_raise_OSError(MP_EIO);
}
static MP_DEFINE_CONST_FUN_OBJ_2(modair_flash_erase_obj, modair_flash_erase);

static mp_obj_t modair_flash_size(void) {
    return mp_obj_new_int_from_uint(CONFIG_FLASH_SIZE); // 4MB
}
static MP_DEFINE_CONST_FUN_OBJ_0(modair_flash_size_obj, modair_flash_size);


static mp_obj_t modair_flash_user_start(void) {
    return MP_OBJ_NEW_SMALL_INT((uint32_t)CONFIG_FS_SYS_FLASH_OFFSET);
}
static MP_DEFINE_CONST_FUN_OBJ_0(modair_flash_user_start_obj, modair_flash_user_start);


static mp_obj_t modair_flash_user_end(void) {
    return MP_OBJ_NEW_SMALL_INT((uint32_t)CONFIG_FS_SYS_FLASH_OFFSET + CONFIG_FS_SYS_FLASH_SIZE);
}
static MP_DEFINE_CONST_FUN_OBJ_0(modair_flash_user_end_obj, modair_flash_user_end);

static mp_obj_t modair_freemem() {
    uint32_t total;
    uint32_t used;
    iot_os_mem_used(&total, &used);
    return MP_OBJ_NEW_SMALL_INT((uint32_t)(total - used));
}
static MP_DEFINE_CONST_FUN_OBJ_0(modair_freemem_obj, modair_freemem);

static mp_obj_t modair_meminfo() {
    uint32_t total;
    uint32_t used;
    iot_os_mem_used(&total, &used);
    mp_printf(&mp_plat_print, "Total: %d, Used: %d\n", total, used);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(modair_meminfo_obj, modair_meminfo);

static mp_obj_t modair_malloc(mp_obj_t size_in) {
    return MP_OBJ_NEW_SMALL_INT((mp_uint_t)iot_os_malloc(mp_obj_get_int(size_in))); // ???
}
static MP_DEFINE_CONST_FUN_OBJ_1(modair_malloc_obj, modair_malloc);

static mp_obj_t modair_free(mp_obj_t addr_in) {
    iot_os_free((void *)mp_obj_get_int(addr_in));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(modair_free_obj, modair_free);

// --------------------------------------------------------
//                  Flash file system API
// --------------------------------------------------------
static T_AMOPENAT_USER_FSMOUNT fs_param = {0};
mp_obj_t modair_flash_mount(mp_obj_t arg1, mp_obj_t arg2) {
    (void) arg1;
    (void) arg2;

    // from coolwatcher: 
    // FBDEV: create phys_start/0x300000 phys_size/0x260000 eb_size/0x8000 pb_size/0x200 read_only/0    - system file system
    // FBDEV: create phys_start/0x560000 phys_size/0x280000 eb_size/0x10000 pb_size/0x200 read_only/0   - modem file system 
    // FBDEV: create phys_start/0x7e0000 phys_size/0x20000 eb_size/0x1000 pb_size/0x100 read_only/0     - factory file system

    // #define CONFIG_APPIMG_FLASH_ADDRESS 0x60180000
    // #define CONFIG_APPIMG_FLASH_SIZE 0x180000 // reserved NOR flash size for loadable app image

    // #define CONFIG_FS_SYS_FLASH_OFFSET 0x300000 // SFFS mount, device block size/500 block count/4799 cache count 4
    // #define CONFIG_FS_SYS_FLASH_SIZE 0x260000  // reserved NOR flash size for system file system
    // #define CONFIG_FS_MODEM_FLASH_ADDRESS 0x60560000
    // #define CONFIG_FS_MODEM_FLASH_SIZE 0x280000 // reserved NOR flash size for modem image file system

    // #define CONFIG_FS_FACTORY_FLASH_ADDRESS 0x607e0000  // SFFS mount, device block size/244 block count/495 cache count 4
    // #define CONFIG_FS_FACTORY_FLASH_SIZE 0x20000 // reserved NOR flash size for factory file system

    /*

    // FS_SYS filesystem already mounted by SDK init and can not be unmount.

    int ret;
	fs_param.exFlash = E_AMOPENAT_FLASH_INTERNAL;
	fs_param.offset = CONFIG_FS_SYS_FLASH_OFFSET;
	fs_param.size = CONFIG_FS_SYS_FLASH_SIZE;
	fs_param.path = "/"; // should be arg2
	fs_param.clkDiv = 2;

	ret = iot_fs_unmount(&fs_param);
	iot_debug_print("umount \"%s\" ret: %d", fs_param.path, ret);

	ret = iot_fs_mount(&fs_param);
	if (!ret) {
		ret = iot_fs_format(&fs_param);
		if (!ret) {
			iot_debug_print("format \"%s\" failed", fs_param.path);
		} else {
			ret = iot_fs_mount(&fs_param);
			if (!ret) {
				iot_debug_print("mount \"%s\" failed", fs_param.path);
			} else iot_debug_print("mount \"%s\" success", fs_param.path);
		}
	} else iot_debug_print("mount \"%s\" success", fs_param.path);
    if (!ret) mp_raise_OSError(MP_EBUSY);
    */
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_2(modair_flash_mount_obj, modair_flash_mount);
MP_DEFINE_CONST_STATICMETHOD_OBJ(modair_flash_mount_static_class_obj, &modair_flash_mount_obj);

mp_obj_t modair_flash_umount() {
    int ret = iot_fs_unmount(&fs_param);
    if (!ret) mp_raise_OSError(MP_EBUSY);
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_0(modair_flash_umount_obj, modair_flash_umount);
MP_DEFINE_CONST_STATICMETHOD_OBJ(modair_flash_umount_static_class_obj, &modair_flash_umount_obj);

int32_t iot_fs_exists(char* path_in) {

    int ret = -1;
    char *path = iot_os_malloc(strlen(path_in) + 1);
    char *file = iot_os_malloc(strlen(path_in) + 1);
    if(path != NULL && file != NULL) {
        strcpy(path, path_in);
        char * dir = strrchr(path, '/');
        if(dir != NULL) {
            strcpy(file, dir + 1);
            *dir = '\0';
        } else {
            strcpy(path, "/");
            strcpy(file, path_in);
        }
        // iot_debug_print("[exists] search '%s' in: %s", file, path);

        AMOPENAT_FS_FIND_DATA *fs_entry = iot_os_malloc(sizeof(AMOPENAT_FS_FIND_DATA));
        if(fs_entry != NULL) {
            memset(fs_entry, 0, sizeof(AMOPENAT_FS_FIND_DATA));
            
            int iFd;
            int res = iFd = iot_fs_find_first(path, fs_entry);
            // iot_debug_print("[exists] find_first: %s, found=%d, st_mode=%d", fs_entry->st_name, res, fs_entry->st_mode);
            if(res >= 0 && strcmp((char*)fs_entry->st_name, file) == 0) {
                // iot_debug_print("[exists] entry found");
            	ret = fs_entry->st_mode;
            	res = -1;
            }
            
            while(res >= 0) {
                res = iot_fs_find_next(iFd, fs_entry);
                // iot_debug_print("[exists] find_next: %s, found=%d, st_mode=%d", fs_entry->st_name, res, fs_entry->st_mode);
            	if(res >= 0 && strcmp((char*)fs_entry->st_name, file) == 0) {
            	    // iot_debug_print("[exists] entry found");
            	    ret = fs_entry->st_mode;
                    res = -1;
            	}
            } 
            if(iFd >= 0) iot_fs_find_close(iFd);
            iot_os_free(fs_entry);
            fs_entry = NULL;
        }
    }
    if(path != NULL) { iot_os_free(path); path = NULL; }
    if(file != NULL) { iot_os_free(file); file = NULL; }
    return ret;
}

uint8_t iot_fs_dexist(char* path) {
    int st_mode = iot_fs_exists((char*)path);
    if (st_mode != -1 && ((st_mode & E_FS_ATTR_ARCHIVE) != E_FS_ATTR_ARCHIVE)) return 1;
    return 0;
}

uint8_t iot_fs_fexist(char* path) {
    int st_mode = iot_fs_exists((char*)path);
    if (st_mode != -1 && ((st_mode & E_FS_ATTR_ARCHIVE) == E_FS_ATTR_ARCHIVE)) return 1;
    return 0;
}

typedef struct _mp_air_lfs_ilistdir_it_t {
    mp_obj_base_t base;
    mp_fun_1_t iternext;
    int32_t iFd;
    char *dir;
} mp_air_lfs_ilistdir_it_t;


static mp_obj_t modair_ilistdir_it_iternext(mp_obj_t self_in) {
    mp_air_lfs_ilistdir_it_t *self = MP_OBJ_TO_PTR(self_in);

    AMOPENAT_FS_FIND_DATA *fs_entry = iot_os_malloc(sizeof(AMOPENAT_FS_FIND_DATA));
    if(fs_entry != NULL) {
        memset(fs_entry, 0, sizeof(AMOPENAT_FS_FIND_DATA));
        
        int res = -1;
        if(self->iFd == -2) {
            res = self->iFd = iot_fs_find_first((char*)self->dir, fs_entry);
            // iot_debug_print("find_first: %s", fs_entry->st_name);
        } else {
            res = iot_fs_find_next(self->iFd, fs_entry);
            // iot_debug_print("find_next: %s", fs_entry->st_name);
        }
        
        if(res >= 0) {
            mp_obj_tuple_t *t = MP_OBJ_TO_PTR(mp_obj_new_tuple(3, NULL));
            t->items[0] = mp_obj_new_str((char*)fs_entry->st_name, strlen((char*)fs_entry->st_name)),
            t->items[1] = MP_OBJ_NEW_SMALL_INT((fs_entry->st_mode & E_FS_ATTR_ARCHIVE) == E_FS_ATTR_ARCHIVE ? MP_S_IFREG : MP_S_IFDIR),
            t->items[2] = MP_OBJ_NEW_SMALL_INT(0), // no inode number
            iot_os_free(fs_entry); 
            fs_entry = NULL; 
            return MP_OBJ_FROM_PTR(t);
        } else {
            iot_fs_find_close(self->iFd);
            self->iternext = NULL;        
        }
        iot_os_free(fs_entry);
        fs_entry = NULL;
    }
    return MP_OBJ_STOP_ITERATION;
}

mp_obj_t modair_flash_ilistdir(mp_obj_t path_in) {
    const char* path = mp_obj_str_get_str(path_in);
    if (strlen(path) == 0) path = mp_obj_str_get_str(MP_OBJ_NEW_QSTR(MP_QSTR_));

    mp_air_lfs_ilistdir_it_t *iter = mp_obj_malloc(mp_air_lfs_ilistdir_it_t, &mp_type_polymorph_iter);
    iter->iternext = modair_ilistdir_it_iternext;
    iter->dir = (char*)path;
    iter->iFd = -2;
    return MP_OBJ_FROM_PTR(iter);
}

MP_DEFINE_CONST_FUN_OBJ_1(modair_flash_ilistdir_obj, modair_flash_ilistdir);
MP_DEFINE_CONST_STATICMETHOD_OBJ(modair_flash_ilistdir_static_class_obj, &modair_flash_ilistdir_obj);


mp_obj_t modair_flash_mkdir(mp_obj_t path_in) {
    const char* path = mp_obj_str_get_str(path_in);
    if (!iot_fs_dexist((char*)path)) {
        int res = iot_fs_make_dir((char*)path, E_FS_ATTR_DIR);
        if (res < 0) mp_raise_OSError(MP_EIO);
    } else mp_raise_OSError(MP_EEXIST);
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(modair_flash_mkdir_obj, modair_flash_mkdir);
MP_DEFINE_CONST_STATICMETHOD_OBJ(modair_flash_mkdir_static_class_obj, &modair_flash_mkdir_obj);


mp_obj_t modair_flash_remove(mp_obj_t path_in) {
    const char* path = mp_obj_str_get_str(path_in);
    if (iot_fs_fexist((char*)path)) {
        int res = iot_fs_delete_file((char*)path);
        if (res < 0) mp_raise_OSError(MP_EIO);
    } else mp_raise_OSError(MP_ENOENT);
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(modair_flash_remove_obj, modair_flash_remove);
MP_DEFINE_CONST_STATICMETHOD_OBJ(modair_flash_remove_static_class_obj, &modair_flash_remove_obj);

mp_obj_t modair_flash_rename(mp_obj_t old_path_in, mp_obj_t new_path_in) {
    const char* path_old = mp_obj_str_get_str(old_path_in);
    const char* path_new = mp_obj_str_get_str(new_path_in);
    //ensure_exists(path_old);
    if (iot_fs_fexist((char*)path_old)) {
        int res = OPENAT_rename_file((char*)path_old, (char*)path_new);
        if (res < 0) mp_raise_OSError(MP_EIO);
    } else mp_raise_OSError(MP_ENOENT);
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_2(modair_flash_rename_obj, modair_flash_rename);
MP_DEFINE_CONST_STATICMETHOD_OBJ(modair_flash_rename_static_class_obj, &modair_flash_rename_obj);


mp_obj_t modair_flash_rmdir(mp_obj_t path_in) {
    const char* path = mp_obj_str_get_str(path_in);
    if (iot_fs_dexist((char*)path)) {
        int res = iot_fs_remove_dir((char*)path);
        if (res < 0) mp_raise_OSError(MP_EIO);
    } else mp_raise_OSError(MP_ENOENT);
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(modair_flash_rmdir_obj, modair_flash_rmdir);
MP_DEFINE_CONST_STATICMETHOD_OBJ(modair_flash_rmdir_static_class_obj, &modair_flash_rmdir_obj);

mp_obj_t modair_flash_stat(mp_obj_t path_in) {
    const char* path = mp_obj_str_get_str(path_in);
    int32_t size = 0;
    mp_int_t st_mode = 0;

    //iot_debug_print("dexist %s, %d", path, iot_fs_dexist(path));
    //iot_debug_print("fexist %s, %d", path, iot_fs_fexist(path)); // reboots if iot_fs_dexist(path) == true 

    if (iot_fs_dexist((char*)path)) {
        st_mode = MP_S_IFDIR;
    }
    else if (iot_fs_fexist((char*)path)) {
        st_mode = MP_S_IFREG;
        size = iot_fs_file_size((char*)path);
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
    T_AMOPENAT_FILE_INFO fs_info = {0};
    int res = iot_fs_get_fs_info((char*)path, &fs_info);
    if (res != 0) mp_raise_OSError(MP_EIO);

    mp_obj_tuple_t *t = MP_OBJ_TO_PTR(mp_obj_new_tuple(10, NULL));
    t->items[0] = MP_OBJ_NEW_SMALL_INT(1); // bsize
    t->items[1] = MP_OBJ_NEW_SMALL_INT(1); // frsize
    t->items[2] = MP_OBJ_NEW_SMALL_INT(fs_info.totalSize);// blocks
    t->items[3] = MP_OBJ_NEW_SMALL_INT(fs_info.totalSize - fs_info.usedSize);// bfree
    t->items[4] = MP_OBJ_NEW_SMALL_INT(fs_info.totalSize - fs_info.usedSize);// bavail
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
    int32_t handle;
} air_file_obj_t;

const mp_obj_type_t modair_fileio;
const mp_obj_type_t modair_textio;

static void modair_file_print(const mp_print_t * print, mp_obj_t self_in, mp_print_kind_t kind) {
    (void)self_in;
    (void)kind;
    mp_printf(print, "<io.%s>", mp_obj_get_type_str(self_in));
}

// FS_O_RDONLY     = 0x00,    // Read only.
// FS_O_WRONLY	   = 0x01,    // Write only. 
// FS_O_RDWR       = 0x02,    // Read and Write.
// FS_O_ACCMODE    = 0x03,    // Access. 
// FS_O_CREAT      = 0x0100,  // If the file exists, this flag has no effect except as noted under FS_O_EXCL below. Otherwise, the file shall be created.
// FS_O_EXCL       = 0x00200, // If FS_O_CREAT and FS_O_EXCL are set, the function shall fail if the file exists.
// FS_O_TRUNC      = 0x01000, // If the file exists, and is a regular file, and the file is successfully opened FS_O_WRONLY or FS_O_RDWR, its length shall be truncated to 0.
// FS_O_APPEND     = 0x02000, // If set, the file offset shall be set to the end of the file prior to each write.
// FS_O_SHARE      = 0x04000, // Shared

static mp_obj_t modair_file_open_internal(mp_obj_t path_in, mp_obj_t mode_in) {
    const char *path = mp_obj_str_get_str(path_in);
    const mp_obj_type_t *type = &modair_textio;
    const char *mode = mp_obj_str_get_str(mode_in);
    const char *mode_str = mode;
    uint32_t imode = 0;    
    uint8_t to_create = 0;
    for (; *mode_str; ++mode_str) {
        switch (*mode_str) {
            case 'r':imode |= FS_O_RDONLY; break;
            case 'w':imode |= FS_O_WRONLY; to_create = 1; break;
            case 'x':break;
            case 'a':imode |= FS_O_APPEND; break;
            case '+':imode |= FS_O_RDWR; break;
            case 'b':type = &modair_fileio;break;
            case 't':type = &modair_textio;break;
        }
    }

    // iot_debug_print("open %s, imode = %d", path, imode);
    air_file_obj_t *o = m_new_obj(air_file_obj_t);
    o->base.type = type;

    int32_t handle = iot_fs_open_file((char*)path, imode);
    if (handle < 0) {
        if(to_create == 1) {
           handle = iot_fs_create_file((char*)path);
           if (handle < 0) {
               mp_raise_OSError(MP_ENOENT);
           } 
        } else {
           mp_raise_OSError(MP_ENOENT);
        }
    }
    o->handle = handle;
    // iot_debug_print("file open: %d, %s, %s", o->handle, path, mode);
    return MP_OBJ_FROM_PTR(o);
}

static mp_uint_t modair_file_read(mp_obj_t self_in, void *buf, mp_uint_t size, int *errcode) {
    air_file_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if(self->handle >= 0) {
        int res = iot_fs_read_file(self->handle, buf, size);
        if (res < 0) {
            *errcode = -MP_EIO;
            return MP_STREAM_ERROR;
        }
        return (mp_uint_t)res;
    } else {
        *errcode = MP_EBADF;
        return MP_STREAM_ERROR;
    }
    return 0;
}

static mp_uint_t modair_file_write(mp_obj_t self_in, const void *buf, mp_uint_t size, int *errcode) {
    air_file_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if(self->handle >= 0) {
        int res = iot_fs_write_file(self->handle, (uint8_t *)buf, size);
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
        if(self->handle >= 0) {
            struct mp_stream_seek_t *s = (struct mp_stream_seek_t *)(uintptr_t)arg;
            int res = iot_fs_seek_file(self->handle, s->offset, s->whence);
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
        if(self->handle >= 0) {
            int res = iot_fs_flush_file(self->handle);
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
        if(self->handle >= 0) {
            int res = iot_fs_close_file(self->handle);
            if (res < 0) {
                *errcode = -MP_EIO;
                return MP_STREAM_ERROR;
            }
        }
        self->handle = -1;
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
    return modair_file_open_internal(args[ARG_file].u_obj, args[ARG_mode].u_obj);
}

MP_DEFINE_CONST_FUN_OBJ_KW(modair_file_open_obj, 1, modair_file_open);
MP_DEFINE_CONST_STATICMETHOD_OBJ(modair_flash_open_static_class_obj, &modair_file_open_obj);

// --------------------------------------------------------
//                Micropython flash bindings
// --------------------------------------------------------

static const mp_rom_map_elem_t modair_flash_locals_dict_table[] = {
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
