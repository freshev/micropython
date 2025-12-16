#ifndef _HTTP_DOWNLOAD_H_
#define _HTTP_DOWNLOAD_H_

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>

#define      HTTP_DOWNLOAD_ERR_URL_INVALID        -0x8001
#define      HTTP_DOWNLOAD_ERR_UNKOWN             -0x8002
#define      HTTP_DOWNLOAD_ERR_FETCH_FAILED       -0x8003
#define      HTTP_DOWNLOAD_ERR_CHECKSUM_MISMATCH  -0x8004

/**
 * @brief Additional parameters for file download*/
typedef struct {
    /*File range download, starting offset address, default is 0*/
    uint32_t range_start;
    /*File interval download, termination offset address, if not set, set it to 0*/
    uint32_t range_end;
    /*1: Use https to download 0: Use http to download, the default is 0*/
    int32_t https_enable;
} http_download_params_t;

/**
 * @brief file data saving callback function type definition
 * @details
 * When the file is saved normally, the written data length is returned.
 * When the file is saved abnormally, -1 will be returned, which will interrupt the download process.*/
typedef int32_t (*file_save_func_t)(uint32_t offset, uint8_t *data, uint32_t data_len, void *userdata);


/**
 * @brief http download file
 *
 * @param[in] url file download address
 * @param[in] extra_params Other extended parameters, fill in NULL if none
 * @param[in] save_func file data saving callback function
 * @param[in] userdata user pointer returned when executing callback
 *
 * @return http_download_result_t
 * @retval <HTTP_DOWNLOAD_SUCCESS execution failed
 * @retval HTTP_DOWNLOAD_SUCCESS execution successful*/
int32_t core_http_download_request(char *url, http_download_params_t *extra_params, file_save_func_t save_func, void* userdata);

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _CORE_HTTP_H_ */
