
// This file contains the default implementation of the system heap and lua heap

#include "luat_base.h"
#include <stdlib.h>
#include "luat_mem.h"

#ifdef __LUATOS__
#include "bget.h"
#endif

#define LUAT_LOG_TAG "vmheap"
#include "luat_log.h"

//------------------------------------------------
//Manage system memory
LUAT_WEAK void luat_heap_init(void){}

LUAT_WEAK void* luat_heap_malloc(size_t len) {
    return malloc(len);
}

LUAT_WEAK void luat_heap_free(void* ptr) {
    free(ptr);
}

LUAT_WEAK void* luat_heap_realloc(void* ptr, size_t len) {
    return realloc(ptr, len);
}

LUAT_WEAK void* luat_heap_calloc(size_t count, size_t _size) {
    void *ptr = luat_heap_malloc(count * _size);
    if (ptr) {
        memset(ptr, 0, _size);
    }
    return ptr;
}

LUAT_WEAK void* luat_heap_zalloc(size_t _size) {
    void *ptr = luat_heap_malloc(_size);
    if (ptr) {
        memset(ptr, 0, _size);
    }
    return ptr;
}

//------------------------------------------------

//------------------------------------------------
// ---------- Manage the memory used by LuaVM----------------
#ifdef __LUATOS__
LUAT_WEAK void* luat_heap_alloc(void *ud, void *ptr, size_t osize, size_t nsize) {
    if (0) {
        if (ptr) {
            if (nsize) {
                // Scale memory block
                LLOGD("realloc %p from %d to %d", ptr, osize, nsize);
            }
            else {
                // Release memory block
                LLOGD("free %p ", ptr);
                brel(ptr);
                return NULL;
            }
        }
        else {
            //Apply for memory block
            ptr = bget(nsize);
            LLOGD("malloc %p type=%d size=%d", ptr, osize, nsize);
            return ptr;
        }
    }

    if (nsize)
    {
    	void* ptmp = bgetr(ptr, nsize);
    	if(ptmp == NULL && osize >= nsize)
    	{
    		return ptr;
    	}
        return ptmp;
    }
    brel(ptr);
    return NULL;
}

LUAT_WEAK void luat_meminfo_luavm(size_t *total, size_t *used, size_t *max_used) {
	long curalloc, totfree, maxfree;
	unsigned long nget, nrel;
	bstats(&curalloc, &totfree, &maxfree, &nget, &nrel);
	*used = curalloc;
	*max_used = bstatsmaxget();
    *total = curalloc + totfree;
}
#endif
//-----------------------------------------------------------------------------