

/**
 * C API of memory pool
 **/

#ifndef LUAT_MALLOC_H
#define LUAT_MALLOC_H

//----------------
// This part uses system memory
void  luat_heap_init(void);
void* luat_heap_malloc(size_t len);
void  luat_heap_free(void* ptr);
void* luat_heap_realloc(void* ptr, size_t len);
void* luat_heap_calloc(size_t count, size_t _size);
void* luat_heap_zalloc(size_t _size);

//size_t luat_heap_getfree(void);
// This part is LuaVM exclusive memory
void* luat_heap_alloc(void *ud, void *ptr, size_t osize, size_t nsize);

//Two methods to obtain memory information, unit bytes
void luat_meminfo_luavm(size_t* total, size_t* used, size_t* max_used);
void luat_meminfo_sys(size_t* total, size_t* used, size_t* max_used);

#endif
