
#include "luat_fs.h"
#define LUAT_LOG_TAG "fs"
#include "luat_log.h"

#define TAG "luat.fs"

//The default implementation of fs, pointing to the method declared by stdio.h of poisx

LUAT_WEAK FILE* luat_fs_fopen(const char *filename, const char *mode) {
    //LLOGD("fopen %s %s", filename, mode);
    return fopen(filename, mode);
}

LUAT_WEAK int luat_fs_getc(FILE* stream) {
    return getc(stream);
}

LUAT_WEAK int luat_fs_fseek(FILE* stream, long int offset, int origin) {
    return fseek(stream, offset, origin);
}

LUAT_WEAK int luat_fs_ftell(FILE* stream) {
    return ftell(stream);
}

LUAT_WEAK int luat_fs_fclose(FILE* stream) {
    return fclose(stream);
}
LUAT_WEAK int luat_fs_feof(FILE* stream) {
    return feof(stream);
}
LUAT_WEAK int luat_fs_ferror(FILE *stream) {
    return ferror(stream);
}
LUAT_WEAK size_t luat_fs_fread(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    return fread(ptr, size, nmemb, stream);
}
LUAT_WEAK size_t luat_fs_fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream) {
    return fwrite(ptr, size, nmemb, stream);
}
LUAT_WEAK int luat_fs_remove(const char *filename) {
    return remove(filename);
}
LUAT_WEAK int luat_fs_rename(const char *old_filename, const char *new_filename) {
    return rename(old_filename, new_filename);
}
LUAT_WEAK int luat_fs_fexist(const char *filename) {
    FILE* fd = luat_fs_fopen(filename, "rb");
    if (fd) {
        luat_fs_fclose(fd);
        return 1;
    }
    return 0;
}

LUAT_WEAK size_t luat_fs_fsize(const char *filename) {
    FILE *fd;
    size_t size = 0;
    fd = luat_fs_fopen(filename, "rb");
    if (fd) {
        luat_fs_fseek(fd, 0, SEEK_END);
        size = luat_fs_ftell(fd); 
        luat_fs_fclose(fd);
    }
    return size;
}

LUAT_WEAK int luat_fs_readline(char * buf, int bufsize, FILE * stream){
    int get_len = 0;
    char buff[1];
    for (size_t i = 0; i < bufsize; i++){
        int len = luat_fs_fread(buff, sizeof(char), 1, stream);
        if (len>0){
            get_len = get_len+len;
            memcpy(buf+i, buff, len);
            if (memcmp(buff, "\n", 1)==0){
                break;
            }
        }else{
            break;
        }
    }
    return get_len;
}

LUAT_WEAK int luat_fs_mkfs(luat_fs_conf_t *conf) {
    LLOGE("not support yet : mkfs");
    return -1;
}
LUAT_WEAK int luat_fs_mount(luat_fs_conf_t *conf) {
    LLOGE("not support yet : mount");
    return -1;
}
LUAT_WEAK int luat_fs_umount(luat_fs_conf_t *conf) {
    LLOGE("not support yet : umount");
    return -1;
}

LUAT_WEAK int luat_fs_mkdir(char const* _DirName) {
    LLOGE("not support yet : mkdir");
    return -1;
}
LUAT_WEAK int luat_fs_rmdir(char const* _DirName) {
    LLOGE("not support yet : rmdir");
    return -1;
}

LUAT_WEAK int luat_fs_lsdir(char const* _DirName, luat_fs_dirent_t* ents, size_t offset, size_t len) {
    LLOGE("not support yet : lsdir");
    return 0;
}
