#include "luat_base.h"
#include "luat_fskv.h"
#include "lfs.h"
#include "common_api.h"

#define LFS_BLOCK_DEVICE_READ_SIZE (256)
#define LFS_BLOCK_DEVICE_PROG_SIZE (256)
#define LFS_BLOCK_DEVICE_CACHE_SIZE (256)
#define LFS_BLOCK_DEVICE_ERASE_SIZE (4096) // one sector 4KB
//#define LFS_BLOCK_DEVICE_TOTOAL_SIZE (64 * 1024)
#define LFS_BLOCK_DEVICE_LOOK_AHEAD (16)


typedef struct luat_kvfs_lfs
{
    char read_buffer[LFS_BLOCK_DEVICE_READ_SIZE];
    char prog_buffer[LFS_BLOCK_DEVICE_PROG_SIZE];
    // char cache_buffer[LFS_BLOCK_DEVICE_CACHE_SIZE];
    char lookahead_buffer[LFS_BLOCK_DEVICE_LOOK_AHEAD];
    lfs_t lfs;
    uint32_t read_address;
    uint32_t write_address;
    uint32_t total_len;
    struct lfs_config conf;
}luat_kvfs_lfs_t;

static luat_kvfs_lfs_t* fskv_lfs;

// Read a block
static int block_device_read(const struct lfs_config *cfg, lfs_block_t block,
        lfs_off_t off, void *buffer, lfs_size_t size) {
    memcpy(buffer, fskv_lfs->read_address + block * LFS_BLOCK_DEVICE_ERASE_SIZE + off, size);
    return LFS_ERR_OK;
}

// Program a block
//
// The block must have previously been erased.
static int block_device_prog(const struct lfs_config *cfg, lfs_block_t block,
        lfs_off_t off, const void *buffer, lfs_size_t size) {

	int ret = luat_flash_write(buffer, fskv_lfs->write_address + block * LFS_BLOCK_DEVICE_ERASE_SIZE + off, size);
    // LLOGD("sfd write %d %d %d %d", block, off, size, ret);
    if (ret >= 0) {
        // LLOGD("block_device_prog return LFS_ERR_OK");
        return LFS_ERR_OK;
    }
    // LLOGD("block_device_prog return LFS_ERR_IO");
    return LFS_ERR_IO;
}

// Erase a block
//
// A block must be erased before being programmed. The
// state of an erased block is undefined.
static int block_device_erase(const struct lfs_config *cfg, lfs_block_t block) {
    luat_flash_erase(fskv_lfs->write_address + block * LFS_BLOCK_DEVICE_ERASE_SIZE, LFS_BLOCK_DEVICE_ERASE_SIZE);
    return 0;
}

// Sync the block device
static int block_device_sync(const struct lfs_config *cfg) {
    return 0;
}

static int luat_fskv_lfs_init(uint32_t read_start_address, uint32_t write_start_address, uint32_t total_len) {
    int ret = 0;
    if (fskv_lfs == NULL) {
        fskv_lfs = malloc(sizeof(luat_kvfs_lfs_t));
        if (fskv_lfs == NULL) {
            DBG("out of memory when malloc fskv_lfs");
            return -1;
        }
        memset(fskv_lfs, 0, sizeof(luat_kvfs_lfs_t));
        fskv_lfs->read_address = read_start_address;
        fskv_lfs->write_address = write_start_address;
        fskv_lfs->total_len = total_len;
        fskv_lfs->conf.read = block_device_read;
        fskv_lfs->conf.prog = block_device_prog;
        fskv_lfs->conf.erase = block_device_erase;
        fskv_lfs->conf.sync = block_device_sync;
        fskv_lfs->conf.attr_max = 0;
        fskv_lfs->conf.file_max = 4096;
        fskv_lfs->conf.block_count = (total_len) / LFS_BLOCK_DEVICE_ERASE_SIZE;
        fskv_lfs->conf.block_size = LFS_BLOCK_DEVICE_ERASE_SIZE;
        fskv_lfs->conf.block_cycles = 200;
        fskv_lfs->conf.name_max = 63;
        fskv_lfs->conf.read_size = LFS_BLOCK_DEVICE_CACHE_SIZE;
        fskv_lfs->conf.cache_size = LFS_BLOCK_DEVICE_CACHE_SIZE;
        fskv_lfs->conf.prog_size = LFS_BLOCK_DEVICE_PROG_SIZE;
        fskv_lfs->conf.cache_size = LFS_BLOCK_DEVICE_CACHE_SIZE;
        fskv_lfs->conf.lookahead_size = LFS_BLOCK_DEVICE_LOOK_AHEAD;
        fskv_lfs->conf.lookahead_buffer = fskv_lfs->lookahead_buffer;
        fskv_lfs->conf.prog_buffer = fskv_lfs->prog_buffer;
        fskv_lfs->conf.read_buffer = fskv_lfs->read_buffer;

        ret = lfs_mount(&fskv_lfs->lfs, &fskv_lfs->conf);
        if (ret != LFS_ERR_OK) {
//            DBG("fskv_lfs mount ret %d, exec auto-format", ret);
            ret = lfs_format(&fskv_lfs->lfs, &fskv_lfs->conf);
            if (ret != LFS_ERR_OK) {
                free(fskv_lfs);
                fskv_lfs = NULL;
                DBG("fskv_lfs auto-format ret %d", ret);
                return ret;
            }
            ret = lfs_mount(&fskv_lfs->lfs, &fskv_lfs->conf);
            if (ret != LFS_ERR_OK) {
            	free(fskv_lfs);
                fskv_lfs = NULL;
                DBG("fskv_lfs remount ret %d", ret);
                return ret;
            }
        }
        DBG("init ok");
    }
    return 0;
}


int luat_fskv_del(const char* key) {
    lfs_remove(&fskv_lfs->lfs, key);
    return 0;
}

int luat_fskv_set(const char* key, void* data, size_t len) {
    lfs_file_t fd = {0};
    int ret = 0;
    ret = lfs_file_open(&fskv_lfs->lfs, &fd, key, LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC);
    if (ret != LFS_ERR_OK) {
        return -1;
    }
    ret = lfs_file_write(&fskv_lfs->lfs, &fd, data, len);
    lfs_file_close(&fskv_lfs->lfs, &fd);
    return ret;
}

int luat_fskv_get(const char* key, void* data, size_t len) {
    lfs_file_t fd = {0};
    int ret = 0;
    ret = lfs_file_open(&fskv_lfs->lfs, &fd, key, LFS_O_RDONLY);
    if (ret != LFS_ERR_OK) {
        return 0;
    }
    ret = lfs_file_read(&fskv_lfs->lfs, &fd, data, len);
    lfs_file_close(&fskv_lfs->lfs, &fd);
    return ret > 0 ? ret : 0;
}

int luat_fskv_clear(void) {
    int ret = 0;
    ret = lfs_format(&fskv_lfs->lfs, &fskv_lfs->conf);
    if (ret != LFS_ERR_OK) {
        DBG("fskv clear ret %d", ret);
        return ret;
    }
    ret = lfs_mount(&fskv_lfs->lfs, &fskv_lfs->conf);
    if (ret != LFS_ERR_OK) {
        DBG("fskv reinit ret %d", ret);
        return ret;
    }
    return 0;
}

int luat_fskv_stat(size_t *using_sz, size_t *total, size_t *kv_count) {
    *using_sz = lfs_fs_size(&fskv_lfs->lfs) * LFS_BLOCK_DEVICE_ERASE_SIZE;
    *total = fskv_lfs->total_len;
    lfs_dir_t dir = {0};
    int ret = lfs_dir_open(&fskv_lfs->lfs, &dir, "");
    if (ret != LFS_ERR_OK) {
        DBG("lfs_dir_open ret %d", ret);
        return -1;
    }
    size_t count = 0;
    struct lfs_info info = {0};
    while (1) {
        ret = lfs_dir_read(&fskv_lfs->lfs, &dir, &info);
        if (ret > 0) {
            if (info.type == LFS_TYPE_REG)
                count ++;
        }
        else
            break;
    }
    lfs_dir_close(&fskv_lfs->lfs, &dir);
    *kv_count = count;
    return 0;
}

//int luat_fskv_size(const char* key, uint32_t *out_len) {
//    lfs_file_t fd = {0};
//    PV_Union uPV;
//    int ret = 0;
//    ret = lfs_file_open(&fskv_lfs->lfs, &fd, key, LFS_O_RDONLY);
//    if (ret != LFS_ERR_OK) {
//        return 0;
//    }
//    ret = lfs_file_size(&fskv_lfs->lfs, &fd);
//    if (ret > 1 && ret < 256) {
//        int ret2 = lfs_file_read(&fskv_lfs->lfs, &fd, buff, ret);
//        if (ret2 != ret) {
// ret = -2; // Reading failed, there must be something wrong
//        }
//    }
//    lfs_file_close(&fskv_lfs->lfs, &fd);
//    return ret;
//}

int luat_fskv_next(char* buff, size_t offset) {
    lfs_dir_t dir = {0};
    struct lfs_info info = {0};
    // The offset needs to be +2, because the first two values     are "." and ".." dirs
    offset += 2;
    int ret = lfs_dir_open(&fskv_lfs->lfs, &dir, "");
    if (ret < 0) {
        DBG("lfs_dir_open ret %d", ret);
        return -1;
    }
    ret = lfs_dir_seek(&fskv_lfs->lfs, &dir, offset);
    if (ret < 0) {
        lfs_dir_close(&fskv_lfs->lfs, &dir);
        return -2;
    }
    ret = lfs_dir_read(&fskv_lfs->lfs, &dir, &info);
    if (ret <= 0) {
        lfs_dir_close(&fskv_lfs->lfs, &dir);
        return -3;
    }
    memcpy(buff, info.name, strlen(info.name) + 1);
    lfs_dir_close(&fskv_lfs->lfs, &dir);
    return 0;
}

int luat_fskv_init(uint32_t read_start_address, uint32_t write_start_address, uint32_t total_len)
{
    if (fskv_lfs == NULL) {
        if (fskv_lfs == NULL) {
            luat_fskv_lfs_init(read_start_address, write_start_address, total_len);
        }
        if (fskv_lfs == NULL) {
            DBG("fskv init failed");
            return -1;
        }
    }
    return 0;

}
