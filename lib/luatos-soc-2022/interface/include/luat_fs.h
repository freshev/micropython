/*
 * Copyright (c) 2022 OpenLuat & AirM2M
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef LUAT_FS_H
#define LUAT_FS_H
//#include "luat_base.h"
#include "stdio.h"


/**
 * @defgroup luat_fs file system interface
 * @{*/

#ifndef LUAT_WEAK
#define LUAT_WEAK __attribute__((weak))
#endif

typedef struct luat_fs_conf {
    char* busname;
    char* type;
    char* filesystem;
    const char* mount_point;
} luat_fs_conf_t;

typedef struct luat_fs_info
{
    char filesystem[8]; // file system type
    unsigned char type;   // Connection method, on-chip, spi flash, tf card, etc.
    size_t total_block;
    size_t block_used;
    size_t block_size;
}luat_fs_info_t;


/**
 * @brief file system initialization
 * @return int =0 success, others failure*/
int luat_fs_init(void);

// int luat_fs_mkfs(luat_fs_conf_t *conf);
int luat_fs_mount(luat_fs_conf_t *conf);
// int luat_fs_umount(luat_fs_conf_t *conf);
/**
 * @brief Get file system status
 * @param path[IN] Mount path, usually /
 * @param info[OUT] file system information
 * @return int =0 success, others failure*/
int luat_fs_info(const char* path, luat_fs_info_t *info);

/**
 * @brief Open file, similar to fopen
 * @param path[IN] file path
 * @param mode[IN] Open mode, with posix type, such as "r" "rw" "w" "w+" "a"
 * @return FILE* file handle, returns NULL on failure*/
FILE* luat_fs_fopen(const char *filename, const char *mode);

/**
 * @brief reads a single byte, similar to getc
 * @param stream[IN] file handle
 * @return int >=0 returns successfully when reading, -1 fails, for example, reading to the end of the file*/
int luat_fs_getc(FILE* stream);
/**
 * @brief sets the handle position, similar to fseek
 * @param stream[IN] file handle
 * @param offset[IN] offset
 * @param origin[IN] reference point, such as SEEK_SET absolute coordinates, SEEK_END end, SEEK_CUR current
 * @return int >=0 success, otherwise failure*/
int luat_fs_fseek(FILE* stream, long int offset, int origin);
/**
 * @brief Get the handle position, similar to ftell
 * @param stream[IN] file handle
 * @return int >=0 current position, otherwise failure*/
int luat_fs_ftell(FILE* stream);
/**
 * @brief Close handle position, similar to fclose
 * @param stream[IN] file handle
 * @return int =0 success, otherwise failure*/
int luat_fs_fclose(FILE* stream);
/**
 * @brief Whether the end of the file has been reached, similar to feof
 * @param stream[IN] file handle
 * @return int =0 has not reached the end of the file, and the rest has reached the end of the file.*/
int luat_fs_feof(FILE* stream);
/**
 * @brief Is there a file system error, similar to ferror
 * @param stream[IN] file handle
 * @return int =0 no error, the rest are error values*/
int luat_fs_ferror(FILE *stream);

/**
 * @brief reads files, similar to fread
 * @param ptr[OUT] buffer to store read data
 * @param size[IN] Single read size
 * @param nmemb[IN] number of reads
 * @param stream[IN] file handle
 * @return int >=0 the actual number read, <0 error*/
size_t luat_fs_fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
/**
 * @brief Write to file, similar to fwrite
 * @param ptr[OUT] Buffer to store written data
 * @param size[IN] Single read size
 * @param nmemb[IN] number of reads
 * @param stream[IN] file handle
 * @return int >=0 the actual number written, <0 error*/
size_t luat_fs_fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);


int luat_fs_fflush(FILE *stream);

/**
 * @brief Delete files, similar to remove
 * @param filename[IN] file path
 * @return int =0 success, otherwise failure*/
int luat_fs_remove(const char *filename);

/**
 * @brief file rename, similar to rename
 * @param old_filename[IN] original file path
 * @param new_filename[IN] new file path
 * @return int =0 success, otherwise failure*/
int luat_fs_rename(const char *old_filename, const char *new_filename);


/**
 * @brief file size, similar to fsize
 * @param filename[IN] file path
 * @return int >=0 file size, if the file does not exist, it will return 0*/
size_t luat_fs_fsize(const char *filename);
/**
 * @brief Whether the file exists, similar to fexist
 * @param filename[IN] file path
 * @return int =0 does not exist, otherwise it exists*/
int luat_fs_fexist(const char *filename);

/**
 * @brief truncate file, similar to ftruncate
 * @param fp[IN] file handle
 * @return int >=0The file size after truncation, otherwise it fails*/
// int luat_fs_ftruncate(FILE* fp, size_t len);


/**
 * @brief truncate file, similar to truncate
 * @param filename[IN] file path
 * @return int >=0The file size after truncation, otherwise it fails*/
int luat_fs_truncate(const char* filename, size_t len);

// int luat_fs_readline(char * buf, int bufsize, FILE * stream);

//Folder related API

typedef struct luat_fs_dirent
{
    unsigned char d_type; //0: file; 1: folder
    char d_name[255];
}luat_fs_dirent_t;


/**
 * @brief Create folder
 * @param dir[IN] folder path
 * @return int =0 success, otherwise failure*/
int luat_fs_mkdir(char const* dir);

/**
 * @brief Delete the folder, it must be an empty folder
 * @param dir[IN] folder path
 * @return int =0 success, otherwise failure*/
int luat_fs_rmdir(char const* dir);
/**
 * @brief Traverse folders
 * @param dir[IN] folder path
 * @param ents[OUT] file list, memory must have been allocated and no less than len elements
 * @param offset[IN] How many files to skip
 * @param len[IN] Maximum number of files to read
 * @return int =>0The number of files read, otherwise it fails*/
int luat_fs_lsdir(char const* dir, luat_fs_dirent_t* ents, size_t offset, size_t len);


/**
 * @brief Whether the folder exists
 * @param dir[IN] folder name
 * @return int =0 does not exist, otherwise it exists*/
int luat_fs_dexist(const char *dir);

/**
 * @brief Is the file system ready?
 * @return int 0 not ready, >0 ready*/
int luat_fs_ready(void);


// #if LUAT_USE_FS_VFS

// #ifndef LUAT_VFS_FILESYSTEM_MAX
// #define LUAT_VFS_FILESYSTEM_MAX 8
// #endif

// #ifndef LUAT_VFS_FILESYSTEM_MOUNT_MAX
// #define LUAT_VFS_FILESYSTEM_MOUNT_MAX 8
// #endif

// #ifndef LUAT_VFS_FILESYSTEM_FD_MAX
// #define LUAT_VFS_FILESYSTEM_FD_MAX 16
// #endif

// struct luat_vfs_file_opts {
//     FILE* (*fopen)(void* fsdata, const char *filename, const char *mode);
//     int (*getc)(void* fsdata, FILE* stream);
//     int (*fseek)(void* fsdata, FILE* stream, long int offset, int origin);
//     int (*ftell)(void* fsdata, FILE* stream);
//     int (*fclose)(void* fsdata, FILE* stream);
//     int (*feof)(void* fsdata, FILE* stream);
//     int (*ferror)(void* fsdata, FILE *stream);
//     size_t (*fread)(void* fsdata, void *ptr, size_t size, size_t nmemb, FILE *stream);
//     size_t (*fwrite)(void* fsdata, const void *ptr, size_t size, size_t nmemb, FILE *stream);
//     void* (*mmap)(void* fsdata, FILE *stream);
// };

// struct luat_vfs_filesystem_opts {
//     int (*remove)(void* fsdata, const char *filename);
//     int (*rename)(void* fsdata, const char *old_filename, const char *new_filename);
//     size_t (*fsize)(void* fsdata, const char *filename);
//     int (*fexist)(void* fsdata, const char *filename);
//     int (*mkfs)(void* fsdata, luat_fs_conf_t *conf);

//     int (*mount)(void** fsdata, luat_fs_conf_t *conf);
//     int (*umount)(void* fsdata, luat_fs_conf_t *conf);
//     int (*info)(void* fsdata, const char* path, luat_fs_info_t *conf);

//     int (*mkdir)(void* fsdata, char const* _DirName);
//     int (*rmdir)(void* fsdata, char const* _DirName);
//     int (*lsdir)(void* fsdata, char const* _DirName, luat_fs_dirent_t* ents, size_t offset, size_t len);
// };

// typedef struct luat_vfs_filesystem {
//     char name[16];
//     struct luat_vfs_filesystem_opts opts;
//     struct luat_vfs_file_opts fopts;
// }luat_vfs_filesystem_t;

// typedef struct luat_vfs_mount {
//     struct luat_vfs_filesystem *fs;
//     void *userdata;
//     char prefix[16];
//     int ok;
// } luat_vfs_mount_t;

// typedef struct luat_vfs_fd{
//     FILE* fd;
//     luat_vfs_mount_t *fsMount;
// }luat_vfs_fd_t;


// typedef struct luat_vfs
// {
//     struct luat_vfs_filesystem* fsList[LUAT_VFS_FILESYSTEM_MAX];
//     luat_vfs_mount_t mounted[LUAT_VFS_FILESYSTEM_MOUNT_MAX];
//     luat_vfs_fd_t fds[LUAT_VFS_FILESYSTEM_FD_MAX+1];
// }luat_vfs_t;

// int luat_vfs_init(void* params);
// int luat_vfs_reg(const struct luat_vfs_filesystem* fs);
// FILE* luat_vfs_add_fd(FILE* fd, luat_vfs_mount_t * mount);
// int luat_vfs_rm_fd(FILE* fd);
// const char* luat_vfs_mmap(FILE* fd);
// #endif

/** @}*/

#endif


