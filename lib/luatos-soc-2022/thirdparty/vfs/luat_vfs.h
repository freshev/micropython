#include "luat_fs.h"
#define LUAT_USE_FS_VFS 1
#ifndef LUAT_VFS_FILESYSTEM_MAX
#define LUAT_VFS_FILESYSTEM_MAX 8
#endif

#ifndef LUAT_VFS_FILESYSTEM_MOUNT_MAX
#define LUAT_VFS_FILESYSTEM_MOUNT_MAX 8
#endif

#ifndef LUAT_VFS_FILESYSTEM_FD_MAX
#define LUAT_VFS_FILESYSTEM_FD_MAX 16
#endif

struct luat_vfs_file_opts {
    FILE* (*fopen)(void* fsdata, const char *filename, const char *mode);
    int (*getc)(void* fsdata, FILE* stream);
    int (*fseek)(void* fsdata, FILE* stream, long int offset, int origin);
    int (*ftell)(void* fsdata, FILE* stream);
    int (*fclose)(void* fsdata, FILE* stream);
    int (*feof)(void* fsdata, FILE* stream);
    int (*ferror)(void* fsdata, FILE *stream);
    size_t (*fread)(void* fsdata, void *ptr, size_t size, size_t nmemb, FILE *stream);
    size_t (*fwrite)(void* fsdata, const void *ptr, size_t size, size_t nmemb, FILE *stream);
    int (*fflush)(void* fsdata, FILE *stream);
    void* (*mmap)(void* fsdata, FILE *stream);
};

struct luat_vfs_filesystem_opts {
    int (*remove)(void* fsdata, const char *filename);
    int (*rename)(void* fsdata, const char *old_filename, const char *new_filename);
    size_t (*fsize)(void* fsdata, const char *filename);
    int (*fexist)(void* fsdata, const char *filename);
    int (*mkfs)(void* fsdata, luat_fs_conf_t *conf);

    int (*mount)(void** fsdata, luat_fs_conf_t *conf);
    int (*umount)(void* fsdata, luat_fs_conf_t *conf);
    int (*info)(void* fsdata, const char* path, luat_fs_info_t *conf);

    int (*mkdir)(void* fsdata, char const* _DirName);
    int (*rmdir)(void* fsdata, char const* _DirName);
    int (*lsdir)(void* fsdata, char const* _DirName, luat_fs_dirent_t* ents, size_t offset, size_t len);

    int (*truncate)(void* fsdata, char const* _DirName, size_t nsize);
};

typedef struct luat_vfs_filesystem {
    char name[16];
    struct luat_vfs_filesystem_opts opts;
    struct luat_vfs_file_opts fopts;
}luat_vfs_filesystem_t;

typedef struct luat_vfs_mount {
    struct luat_vfs_filesystem *fs;
    void *userdata;
    char prefix[16];
    int ok;
} luat_vfs_mount_t;

typedef struct luat_vfs_fd{
    FILE* fd;
    luat_vfs_mount_t *fsMount;
}luat_vfs_fd_t;


typedef struct luat_vfs
{
    struct luat_vfs_filesystem* fsList[LUAT_VFS_FILESYSTEM_MAX];
    luat_vfs_mount_t mounted[LUAT_VFS_FILESYSTEM_MOUNT_MAX];
    luat_vfs_fd_t fds[LUAT_VFS_FILESYSTEM_FD_MAX+1];
}luat_vfs_t;
