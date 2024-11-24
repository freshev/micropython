/*@Modules miniz
@summary Simple zlib compression
@version 1.0
@date 2022.8.11
@tag LUAT_USE_MINIZ
@usage
-- Prepare data
local bigdata = "123jfoiq4hlkfjbnasdilfhuqwo;hfashfp9qw38hrfaios;hfiuoaghfluaeisw"
-- Compression. The compressed data is compatible with zlib. Other languages   can be decompressed through zlib-related libraries.
local cdata = miniz.compress(bigdata)
-- Lua's string is equivalent to a char[] with length, which can store all data including 0x00
if cdata then
    -- Check the data size before and after compression
    log.info("miniz", "before", #bigdata, "after", #cdata)
    log.info("miniz", "cdata as hex", cdata:toHex())

    -- Unzip and get the original text
    local udata = miniz.uncompress(cdata)
    log.info("miniz", "udata", udata)
end*/
#include "luat_base.h"
#include "luat_mem.h"

#define LUAT_LOG_TAG "miniz"
#include "luat_log.h"

#include "miniz.h"

static mz_bool luat_output_buffer_putter(const void *pBuf, int len, void *pUser) {
    luaL_addlstring((luaL_Buffer*)pUser, pBuf, len);
    return MZ_TRUE;
}

/*Fast compression, requires 165kb of system memory and 32kb of LuaVM memory
@api miniz.compress(data, flags)
@string Data to be compressed, data less than 400 bytes is not recommended to be compressed, and the compressed data cannot be larger than 32k.
@flags compression parameters, the default is miniz.WRITE_ZLIB_HEADER, which is written to the zlib header
@return string If the compression is successful, return the data string, otherwise return nil
@usage

local bigdata = "123jfoiq4hlkfjbnasdilfhuqwo;hfashfp9qw38hrfaios;hfiuoaghfluaeisw"
local cdata = miniz.compress(bigdata)
if cdata then
    log.info("miniz", "before", #bigdata, "after", #cdata)
    log.info("miniz", "cdata as hex", cdata:toHex())
end*/
static int l_miniz_compress(lua_State* L) {
    size_t len = 0;
    tdefl_compressor *pComp;
    mz_bool succeeded;
    const char* data = luaL_checklstring(L, 1, &len);
    int flags = luaL_optinteger(L, 2, TDEFL_WRITE_ZLIB_HEADER);
    if (len > 32* 1024) {
        LLOGE("only 32k data is allow");
        return 0;
    }
    luaL_Buffer buff;
    if (NULL == luaL_buffinitsize(L, &buff, 4096)) {
        LLOGE("out of memory when malloc dst buff");
        return 0;
    }
    pComp = (tdefl_compressor *)luat_heap_malloc(sizeof(tdefl_compressor));
    if (!pComp) {
        LLOGE("out of memory when malloc tdefl_compressor size 0x%04X", sizeof(tdefl_compressor));
        return 0;
    }
    succeeded = (tdefl_init(pComp, luat_output_buffer_putter, &buff, flags) == TDEFL_STATUS_OKAY);
    succeeded = succeeded && (tdefl_compress_buffer(pComp, data, len, TDEFL_FINISH) == TDEFL_STATUS_DONE);
    luat_heap_free(pComp);
    if (!succeeded) {
        LLOGW("compress fail ret=0");
        return 0;
    }
    luaL_pushresult(&buff);
    return 1;
}

/*Fast decompression, requires 32kb of LuaVM memory
@api miniz.uncompress(data, flags)
@string The data to be decompressed, the decompressed data cannot be larger than 32k
@flags decompression parameters, the default is miniz.PARSE_ZLIB_HEADER, that is, parsing the zlib header
@return string If the decompression is successful, return the data string, otherwise return nil
@usage

local bigdata = "123jfoiq4hlkfjbnasdilfhuqwo;hfashfp9qw38hrfaios;hfiuoaghfluaeisw"
local cdata = miniz.compress(bigdata)
if cdata then
    log.info("miniz", "before", #bigdata, "after", #cdata)
    log.info("miniz", "cdata as hex", cdata:toHex())

    local udata = miniz.uncompress(cdata)
    log.info("miniz", "udata", udata)
end*/
static int l_miniz_uncompress(lua_State* L) {
    size_t len = 0;
    const char* data = luaL_checklstring(L, 1, &len);
    int flags = luaL_optinteger(L, 2, TINFL_FLAG_PARSE_ZLIB_HEADER);
    if (len > 32* 1024) {
        LLOGE("only 32k data is allow");
        return 0;
    }
    luaL_Buffer buff;
    char* dst = luaL_buffinitsize(L, &buff, TDEFL_OUT_BUF_SIZE);
    if (dst == NULL) {
        LLOGE("out of memory when malloc dst buff");
        return 0;
    }
    size_t out_buf_len = TDEFL_OUT_BUF_SIZE;
    tinfl_status status;
    tinfl_decompressor *decomp = luat_heap_malloc(sizeof(tinfl_decompressor));
    if (decomp == NULL) {
        LLOGE("out of memory when malloc tinfl_decompressor");
        return 0;
    }
    tinfl_init(decomp);
    status = tinfl_decompress(decomp, (const mz_uint8 *)data, &len, (mz_uint8 *)dst, (mz_uint8 *)dst, &out_buf_len, (flags & ~TINFL_FLAG_HAS_MORE_INPUT) | TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF);
    size_t ret = (status != TINFL_STATUS_DONE) ? TINFL_DECOMPRESS_MEM_TO_MEM_FAILED : out_buf_len;
    luat_heap_free(decomp);
    if (ret == TINFL_DECOMPRESS_MEM_TO_MEM_FAILED) {
        LLOGW("decompress fail");
        return 0;
    }
    luaL_pushresultsize(&buff, ret);
    return 1;
}

#include "rotable2.h"
static const rotable_Reg_t reg_miniz[] = {
    {"compress", ROREG_FUNC(l_miniz_compress)},
    {"uncompress", ROREG_FUNC(l_miniz_uncompress)},
    // {"inflate", ROREG_FUNC(l_miniz_inflate)},
    // {"deflate", ROREG_FUNC(l_miniz_deflate)},

    // put some constants
    //Compression parameters-------------------------
    //@const WRITE_ZLIB_HEADER int compression parameter, whether to write zlib header data, default value of compress function
    {"WRITE_ZLIB_HEADER", ROREG_INT(TDEFL_WRITE_ZLIB_HEADER)},
    //@const COMPUTE_ADLER32 int compression/decompression parameters, whether to calculate/check adler-32
    {"COMPUTE_ADLER32", ROREG_INT(TDEFL_COMPUTE_ADLER32)},
    //@const GREEDY_PARSING_FLAG int Compression parameters, whether to fast greedy processing, the default is to use slower processing mode
    {"GREEDY_PARSING_FLAG", ROREG_INT(TDEFL_GREEDY_PARSING_FLAG)},
    //@const NONDETERMINISTIC_PARSING_FLAG int compression parameters, whether to quickly initialize the compressor
    {"NONDETERMINISTIC_PARSING_FLAG", ROREG_INT(TDEFL_NONDETERMINISTIC_PARSING_FLAG)},
    //@const RLE_MATCHES int compression parameters, only scan RLE
    {"RLE_MATCHES", ROREG_INT(TDEFL_RLE_MATCHES)},
    //@const FILTER_MATCHES int compression parameters, filter characters less than 5 times
    {"FILTER_MATCHES", ROREG_INT(TDEFL_FILTER_MATCHES)},
    //@const FORCE_ALL_STATIC_BLOCKS int compression parameter, whether to disable optimized Huffman table
    {"FORCE_ALL_STATIC_BLOCKS", ROREG_INT(TDEFL_FORCE_ALL_STATIC_BLOCKS)},
    //@const FORCE_ALL_RAW_BLOCKS int compression parameters, whether only raw blocks are required
    {"FORCE_ALL_RAW_BLOCKS", ROREG_INT(TDEFL_FORCE_ALL_RAW_BLOCKS)},

    //Decompression parameters
    //@const PARSE_ZLIB_HEADER int decompression parameters, whether to process zlib header, default value of uncompress function
    {"PARSE_ZLIB_HEADER", ROREG_INT(TINFL_FLAG_PARSE_ZLIB_HEADER)},
    //@const HAS_MORE_INPUT int decompression parameters, whether there is more data, only streaming decompression is available, not supported yet
    {"HAS_MORE_INPUT", ROREG_INT(TINFL_FLAG_HAS_MORE_INPUT)},
    //@const USING_NON_WRAPPING_OUTPUT_BUF int Decompression parameters, whether the decompression interval is enough for all data, only streaming decompression is available, not supported yet
    {"USING_NON_WRAPPING_OUTPUT_BUF", ROREG_INT(TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF)},
    //@const COMPUTE_ADLER32 int Decompression parameters, whether to force adler-32 verification
    // {"COMPUTE_ADLER32", ROREG_INT(TINFL_FLAG_COMPUTE_ADLER32)},
    

    {NULL, ROREG_INT(0)}
};


LUAMOD_API int luaopen_miniz( lua_State *L ) {
    luat_newlib2(L, reg_miniz);
    return 1;
}
