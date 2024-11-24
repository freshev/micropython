/*@Modules fastlz
@summary FastLZ compression
@version 1.0
@date 2023.7.38
@tag LUAT_USE_FASTLZ
@usage
-- Differences from miniz library
-- The memory requirement is much smaller, miniz library is close to 200k, fastlz only requires 32k+ original data size
-- Compression ratio, the compression ratio of miniz is better than that of fastlz

-- Prepare data
local bigdata = "123jfoiq4hlkfjbnasdilfhuqwo;hfashfp9qw38hrfaios;hfiuoaghfluaeisw"
-- Compressed
local cdata = fastlz.compress(bigdata)
-- Lua's string is equivalent to a char[] with length, which can store all data including 0x00
if cdata then
    -- Check the data size before and after compression
    log.info("fastlz", "before", #bigdata, "after", #cdata)
    log.info("fastlz", "cdata as hex", cdata:toHex())

    -- Unzip and get the original text
    local udata = fastlz.uncompress(cdata)
    log.info("fastlz", "udata", udata)
end*/
#include "luat_base.h"
#include "luat_mem.h"

#define LUAT_LOG_TAG "fastlz"
#include "luat_log.h"

#include "fastlz.h"

#define HASH_LOG 13
#define HASH_SIZE (1 << HASH_LOG)
#define HASH_MASK (HASH_SIZE - 1)

/*fast compression
@api fastlz.compress(data, level)
@string Data to be compressed, data less than 400 bytes is not recommended to be compressed, and the compressed data cannot be larger than 32k
@int compression level, default 1, optional 1 or 2, 2 has a higher compression ratio (sometimes)
@return string If the compression is successful, return the data string, otherwise return nil
@usage
-- Note that the memory consumption of the compression process is as follows
-- System memory, fixed 32k
-- Lua memory, 1.05 times the size of the original data, occupies a minimum of 1024 bytes.*/
static int l_fastlz_compress(lua_State* L) {
    size_t len = 0;
    const char* data = luaL_checklstring(L, 1, &len);
    int level = luaL_optinteger(L, 2, 1);
    luaL_Buffer buff;
    luaL_buffinitsize(L, &buff, len > 512 ? (int)(len * 1.05) : (1024));
    uint32_t *htab = luat_heap_malloc( sizeof(uint32_t) * HASH_SIZE);
    if (htab) {
        int ret = fastlz_compress_level(level, data, len, buff.b, htab);
        luat_heap_free(htab);
        if (ret > 0) {
            luaL_pushresultsize(&buff, ret);
            return 1;
        }
    }
    return 0;
}

/*Quickly decompress
@api fastlz.uncompress(data, maxout)
@string data to be decompressed
@int Maximum size after decompression, default is 4k, can be adjusted as needed
@return string If the decompression is successful, return the data string, otherwise return nil*/
static int l_fastlz_uncompress(lua_State* L) {
    size_t len = 0;
    const char* data = luaL_checklstring(L, 1, &len);
    size_t maxout = luaL_optinteger(L, 2, 4096);
    luaL_Buffer buff;
    luaL_buffinitsize(L, &buff, maxout);
    uint32_t *htab = luat_heap_malloc( sizeof(uint32_t) * HASH_SIZE);
    if (htab) {
        int ret = fastlz_decompress(data, len, buff.b, maxout);
        luat_heap_free(htab);
        if (ret > 0) {
            luaL_pushresultsize(&buff, ret);
            return 1;
        }
    }
    return 0;
}

#include "rotable2.h"
static const rotable_Reg_t reg_fastlz[] = {
    {"compress", ROREG_FUNC(l_fastlz_compress)},
    {"uncompress", ROREG_FUNC(l_fastlz_uncompress)},
    {NULL, ROREG_INT(0)}
};


LUAMOD_API int luaopen_fastlz( lua_State *L ) {
    luat_newlib2(L, reg_fastlz);
    return 1;
}
