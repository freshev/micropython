/**
 * @file aiot_fs_api.h
 * @brief SDK linkspeech dependent file operations, all dependent file operations will be listed here
 * @date 2019-12-27
 *
 * @copyright Copyright (C) 2015-2025 Alibaba Group Holding Limited
 **/

#ifndef _AIOT_FS_API_H_
#define _AIOT_FS_API_H_

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>


/**
 * @brief A method structure used to describe to the SDK how to use the resources of its running hardware platform*/
typedef struct {
    /**
     * @brief Query file size
     *
     * @return int32_t
     * @retval -1 file does not exist
     * @retval >=0 actual file size*/
    int32_t (*file_size)(char *path);
    /**
     * @brief delete file
     *
     * @return int32_t
     * @retval 0 file deleted successfully
     * @retval -1 File modification failed*/
    int32_t (*file_delete)(char *path);
    /**
     * @brief Write file data
     *
     * @details If the file does not exist, create a new file
     * @return int32_t
     * @retval successful, returns the length of the file written
     * @retval fails, returns -1*/
    int32_t (*file_write)(char *path, uint32_t offset, uint8_t *data, uint32_t len);
    /**
     * @brief Write file data
     *
     * @details The open operation needs to be performed by the user
     * @return int32_t
     * @retval success, returns the read file length
     * @retval fails, returns -1*/
    int32_t (*file_read)(char *path, uint32_t offset, uint8_t *data, uint32_t len);
} aiot_fs_t;

#if defined(__cplusplus)
}
#endif

#endif

