
#include "stdlib.h"
#include "stdint.h"

enum TZ_COMPRESS_MODE {
    TZ_COMPRESS_NONE = 0,
    TZ_COMPRESS_GZ   = 1
};

typedef struct luat_transz_cache
{
    const char *data_ptr; // Need to point to luat_transz_data_t.datas, if not the same, the cache will be disabled
    int   addr;           // The block address maintained by the luat_transz_read function itself should be set to -1 when passed in for the first time.
    char *buff; // Malloc according to the actual block_size length
}luat_transz_cache_t;


typedef struct luat_transz_data
{
    size_t total_size;
    size_t block_size;
    size_t compress_mode;
    const char *fragments;
    const char *datas;
}luat_transz_data_t;


int luat_transz_read(luat_transz_data_t* data, uint8_t* buff, size_t offset, size_t len);

