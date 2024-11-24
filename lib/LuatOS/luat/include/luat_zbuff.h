#ifndef LUAT_ZBUFF_H
#define LUAT_ZBUFF_H

#include "luat_mem.h"
#include "luat_msgbus.h"

#define LUAT_ZBUFF_TYPE "ZBUFF*"
#define tozbuff(L) ((luat_zbuff_t *)luaL_checkudata(L, 1, LUAT_ZBUFF_TYPE))

#define ZBUFF_SEEK_SET 0
#define ZBUFF_SEEK_CUR 1
#define ZBUFF_SEEK_END 2

#if defined ( __CC_ARM )
#pragma anon_unions
#endif

typedef struct luat_zbuff {
    LUAT_HEAP_TYPE_E type; //memory type
    uint8_t* addr;      //Data storage address
    size_t len;       //The length of the actual allocated space
    union {
    	size_t cursor;    //The current pointer position indicates how much data has been processed
    	size_t used;	//The amount of data that has been saved indicates how much data is stored.
    };

    uint32_t width; //width
    uint32_t height;//high
    uint8_t bit;    //color depth
} luat_zbuff_t;


int __zbuff_resize(luat_zbuff_t *buff, uint32_t new_size);

#endif
