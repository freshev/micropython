#include "luat_base.h"
#include "luat_airui.h"
#include "luat_mem.h"

#define LUAT_LOG_TAG "airui"
#include "luat_log.h"

static jsmn_parser parser;

int luat_airui_load_buff(luat_airui_ctx_t** _ctx, int backend, const char* screen_name, const char* buff, size_t len) {
    int ret;
    luat_airui_ctx_t *ctx = luat_heap_malloc(sizeof(luat_airui_ctx_t));
    if (ctx == NULL) {
        LLOGW("out of memory when malloc luat_airui_ctx_t");
        return -1;
    }
    
    // First, initialize the processor
    jsmn_init(&parser);
    // Then, scan it once to get the total number of tokens. If the processing fails, a negative number will be returned.
    ret = jsmn_parse(&parser, buff, len, NULL, 0);
    if (ret <= 0) {
        LLOGW("invaild json ret %d", ret);
        return -2;
    }
    LLOGD("found json token count %d", ret);
    // Then, allocate memory
    jsmntok_t *tok = luat_heap_malloc(sizeof(jsmntok_t) * ret);
    if (tok == NULL) {
        luat_heap_free(ctx);
        LLOGW("out of memory when malloc jsmntok_t");
        return -3;
    }
    // The real analysis will definitely not go wrong
    jsmn_init(&parser);
    ret = jsmn_parse(&parser, buff, len, tok, ret);
    if (ret <= 0) {
        // Let’s defend ourselves
        luat_heap_free(tok);
        luat_heap_free(ctx);
        LLOGW("invaild json ret %d", ret);
        return -2;
    }

    // Now that the parsing is complete, start generating the component tree
    LLOGD("json parse complete, begin components jobs ...");
    ctx->data = buff;
    ctx->screen_name = screen_name;
    ret = luat_airui_load_components(ctx, tok, ret);
    LLOGD("json parse complete, end components jobs, ret %d", ret);
    luat_heap_free(tok);
    if (ret == 0) {
        *_ctx = ctx;
    }
    else {
        luat_heap_free(ctx);
    }
    return ret;
}

int luat_airui_load_file(luat_airui_ctx_t** ctx, int backend, const char* screen_name, const char* path) {
    return -1;
}

int luat_airui_get(luat_airui_ctx_t* ctx, const char* key) {
    return -1;
}

