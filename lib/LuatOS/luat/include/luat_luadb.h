
#ifndef LUAT_LUADB_H
#define LUAT_LUADB_H

// LuaDB read-only file system

//The latest version v2, Luatools 2.1.3 and above is generated by default
#define LUAT_LUADB_MIN_VERSION 2

//Number of files that can be opened simultaneously
#ifndef LUAT_LUADB_MAX_OPENFILE
#define LUAT_LUADB_MAX_OPENFILE 8
#endif

//Data structure of a single file
typedef struct luadb_file {
    char name[32]; //The maximum path length is 31 bytes
    size_t size;   //The maximum size is 64kb
    const char* ptr; //Point to the starting bit of file data
} luadb_file_t;

// file handle
typedef struct luadb_fd
{
    int fd;             // handle number
    luadb_file_t *file; // file pointer
    size_t fd_pos;      //The current offset of the handle
}luadb_fd_t;

typedef struct luadb_fs
{
    uint16_t version;  // File system version number, currently supports v1/v2
    uint16_t filecount; //The total number of files is actually less than 100
    luadb_fd_t fds[LUAT_LUADB_MAX_OPENFILE]; // handle array
    // luadb_file_t *inlines;
    luadb_file_t files[1]; //Array of files
} luadb_fs_t;

// Query and build the file system from the specified location
luadb_fs_t* luat_luadb_mount(const char* ptr);
// Unmount the file system, then fs will be unavailable
int luat_luadb_umount(luadb_fs_t *fs);
// Remount the file system, just force clear all handles
int luat_luadb_remount(luadb_fs_t *fs, unsigned flags);
//Open file
int luat_luadb_open(luadb_fs_t *fs, const char *path, int flags, int /*mode_t*/ mode);
// close file
int luat_luadb_close(luadb_fs_t *fs, int fd);
// read data
size_t luat_luadb_read(luadb_fs_t *fs, int fd, void *dst, size_t size);
//size_t luat_luadb_write(void *fs, int fd, const void *data, size_t size);
//Move handle
long luat_luadb_lseek(luadb_fs_t *fs, int fd, long /*off_t*/ offset, int mode);
// Get file information
luadb_file_t* luat_luadb_stat(luadb_fs_t *fs, const char *path);
// Get the file starting pointer, usually read-only
char* luat_luadb_direct_io(luadb_fs_t *fs, int fd, size_t *len);

#endif
