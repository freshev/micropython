#include "luat_base.h"
// #include "luat_lvgl.h"
#include "luat_airui.h"
#include "math.h"
#include <stdlib.h>

#define LUAT_LOG_TAG "airui"
#include "luat_log.h"

static int top_device_parser(luat_airui_ctx_t* ctx, jsmntok_t *tok, int pos);
static int top_schema_parser(luat_airui_ctx_t* ctx, jsmntok_t *tok, int pos);
static int top_screens_parser(luat_airui_ctx_t* ctx, jsmntok_t *tok, int pos);

airui_parser_t airui_top_parsers[] = {
    // Pay attention to the order, process schema first, and then screens, because the latter depends on the former.
    {.name = "device", .cb = top_device_parser},
    {.name = "screens", .cb = top_screens_parser},
    {.name = "schema", .cb = top_schema_parser},
    {.name = "", .cb = NULL}
};

// Processing device information, actually useless, only debugging logs
static int top_device_parser(luat_airui_ctx_t* ctx, jsmntok_t *tok, int pos) {
    // LLOGD("parse device");
    // int width_pos = jsmn_find_by_key(ctx->data, "width", tok, pos + 1);
    // int height_pos = jsmn_find_by_key(ctx->data, "height", tok, pos + 1);

    // if (width_pos > 0 && height_pos > 0) {
    //     LLOGD("device width %d height %d", 
    //                 jsmn_toint(ctx->data, &tok[width_pos + 1]),
    //                 jsmn_toint(ctx->data, &tok[height_pos + 1])
    //                 );
    // }
    // else {
    //     LLOGD("device width height not found");
    // }

    return 0;
}


int top_screens_one(luat_airui_ctx_t* ctx, jsmntok_t *tok, int pos);
// The highlight, parsing screens
static int top_screens_parser(luat_airui_ctx_t* ctx, jsmntok_t *tok, int pos) {
    LLOGD("parse screens");

    pos ++;

    jsmntok_t* screens = &tok[pos];
    if (screens->type != JSMN_ARRAY) {
        LLOGW("screens must be array");
        return -1;
    }
    
    if (screens->size == 0) {
        LLOGW("screens is emtry");
        return 0;
    }
    else {
        LLOGD("screens count %d", screens->size);
    }

    size_t scount = screens->size;

    pos ++; //Move to an element

    for (size_t i = 0; i < scount; i++)
    {
        top_screens_one(ctx, tok, pos);
        jsmn_skip_object(tok, &pos);
    }
    

    return 0;
}

// parse schema
static int top_schema_parser(luat_airui_ctx_t* ctx, jsmntok_t *tok, int pos) {
    //LLOGD("parse schema");
    return 0;
}
