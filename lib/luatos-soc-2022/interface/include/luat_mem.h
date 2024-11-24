/*
 * Copyright (c) 2022 OpenLuat & AirM2M
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef LUAT_MEM_H
#define LUAT_MEM_H
/**
 * @defgroup luatos_mem memory management
 * @{*/
#include "stdint.h"
/**
 * @brief Allocate a memory space of a specified size in the heap area to store data
 *
 * @param len length of allocated memory
 * @return NULL failure !NULL success*/

void* luat_heap_malloc(size_t len);

/**
 * @brief Releases the memory block pointed to by ptr. What is released is dynamically allocated memory space.
 *
 * @param ptr The memory block pointed to*/
void  luat_heap_free(void* ptr);

/**
 * @brief Reallocate memory and expand memory to len
 *
 * @param ptr The memory block pointed to
 * @param len length*/
void* luat_heap_realloc(void* ptr, size_t len);
/**
 * @brief dynamically allocates count contiguous spaces of length size_t in memory and initializes each byte to 0
 *
 * @param count length
 * @param _size data type
 * @return NULL failure, !NULL success*/
void* luat_heap_calloc(size_t count, size_t _size);
/**
 * @brief Get memory usage information
 *
 * @param[out] total total memory size of the Modules
 * @param[out] used Get the currently allocated memory heap size
 * @param[out] max_used gets the current maximum allocated memory heap size*/
void luat_meminfo_sys(size_t* total, size_t* used, size_t* max_used);


#define LUAT_MEM_MALLOC luat_heap_malloc ///< Allocate a memory space of a specified size in the heap area to store data
#define LUAT_MEM_FREE luat_heap_free ///< Release the memory block pointed to by ptr. What is released is the dynamically allocated memory space.
#define LUAT_MEM_REALLOC luat_heap_realloc ///<Reallocate memory and expand memory to len
#define LUAT_MEM_CALLOC luat_heap_calloc ///< Dynamically allocate count contiguous spaces of length size_t in memory and initialize each byte to 0

#define LUAT_MEM_INFO luat_meminfo_sys ///< Get memory usage information
/**
 * @}
 */
#endif
