/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * Development of the code in this file was sponsored by Microbric Pty Ltd
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Paul Sokolovsky
 * Copyright (c) 2016 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdio.h>

#include "esp_flash.h"
#include "esp_log.h"

#include "py/runtime.h"
#include "py/mperrno.h"
#include "py/mphal.h"

static mp_obj_t esp_osdebug(size_t n_args, const mp_obj_t *args) {
    esp_log_level_t level = LOG_LOCAL_LEVEL; // Maximum available level
    if (n_args == 2) {
        level = mp_obj_get_int(args[1]);
    }
    if (args[0] == mp_const_none) {
        // Set logging back to boot default of ESP_LOG_ERROR
        esp_log_level_set("*", ESP_LOG_ERROR);
    } else {
        // Enable logging at the given level
        // TODO args[0] should set the UART to which debug is sent
        esp_log_level_set("*", level);
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(esp_osdebug_obj, 1, 2, esp_osdebug);

static mp_obj_t esp_flash_read_(mp_obj_t offset_in, mp_obj_t buf_in) {
    mp_int_t offset = mp_obj_get_int(offset_in);
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(buf_in, &bufinfo, MP_BUFFER_WRITE);
    esp_err_t res = esp_flash_read(NULL, bufinfo.buf, offset, bufinfo.len);
    if (res != ESP_OK) {
        mp_raise_OSError(MP_EIO);
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(esp_flash_read_obj, esp_flash_read_);

static mp_obj_t esp_flash_write_(mp_obj_t offset_in, mp_obj_t buf_in) {
    mp_int_t offset = mp_obj_get_int(offset_in);
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(buf_in, &bufinfo, MP_BUFFER_READ);
    esp_err_t res = esp_flash_write(NULL, bufinfo.buf, offset, bufinfo.len);
    if (res != ESP_OK) {
        mp_raise_OSError(MP_EIO);
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(esp_flash_write_obj, esp_flash_write_);

static mp_obj_t esp_flash_erase(mp_obj_t sector_in) {
    mp_int_t sector = mp_obj_get_int(sector_in);
    esp_err_t res = esp_flash_erase_region(NULL, sector * 4096, 4096);
    if (res != ESP_OK) {
        mp_raise_OSError(MP_EIO);
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(esp_flash_erase_obj, esp_flash_erase);

static mp_obj_t esp_flash_size(void) {
    uint32_t size;
    esp_flash_get_size(NULL, &size);
    return mp_obj_new_int_from_uint(size);
}
static MP_DEFINE_CONST_FUN_OBJ_0(esp_flash_size_obj, esp_flash_size);

static mp_obj_t esp_flash_user_start(void) {
    return MP_OBJ_NEW_SMALL_INT(0x200000);
}
static MP_DEFINE_CONST_FUN_OBJ_0(esp_flash_user_start_obj, esp_flash_user_start);

static mp_obj_t esp_gpio_matrix_in(mp_obj_t pin, mp_obj_t sig, mp_obj_t inv) {
    esp_rom_gpio_connect_in_signal(mp_obj_get_int(pin), mp_obj_get_int(sig), mp_obj_get_int(inv));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_3(esp_gpio_matrix_in_obj, esp_gpio_matrix_in);

static mp_obj_t esp_gpio_matrix_out(size_t n_args, const mp_obj_t *args) {
    (void)n_args;
    esp_rom_gpio_connect_out_signal(mp_obj_get_int(args[0]), mp_obj_get_int(args[1]), mp_obj_get_int(args[2]), mp_obj_get_int(args[3]));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(esp_gpio_matrix_out_obj, 4, 4, esp_gpio_matrix_out);


//--------------------------------------------------------
static int vprintf_redirected(const char *fmt, va_list ap)
{
    int ret = mp_vprintf(&mp_plat_print, fmt, ap);
    return ret;
}

static vprintf_like_t orig_log_func = NULL;
static vprintf_like_t prev_log_func = NULL;
static vprintf_like_t mp_log_func = &vprintf_redirected;

//--------------------------------------------------------------------------
static mp_obj_t esp_log_level (mp_obj_t tag_in, mp_obj_t level_in) {
    const char *tag = mp_obj_str_get_str(tag_in);
    int32_t level = mp_obj_get_int(level_in);
    if ((level < 0) || (level > 5)) {
        mp_raise_ValueError(MP_ERROR_TEXT("Log level 0~5 expected"));
    }

    esp_log_level_set(tag, level);

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(esp_log_level_obj, esp_log_level);

//---------------------------------------
static mp_obj_t esp_logto_mp () {
    if (orig_log_func == NULL) {
        orig_log_func = esp_log_set_vprintf(mp_log_func);
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(esp_logto_mp_obj, esp_logto_mp);

//----------------------------------------
static mp_obj_t esp_logto_esp () {
    if (orig_log_func != NULL) {
        prev_log_func = esp_log_set_vprintf(orig_log_func);
        orig_log_func = NULL;
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(esp_logto_esp_obj, esp_logto_esp);

//--------------------------------------------------
static void print_heap_info(multi_heap_info_t *info)
{
    mp_printf(&mp_plat_print, "              Free: %u\n", info->total_free_bytes);
    mp_printf(&mp_plat_print, "         Allocated: %u\n", info->total_allocated_bytes);
    mp_printf(&mp_plat_print, "      Minimum free: %u\n", info->minimum_free_bytes);
    mp_printf(&mp_plat_print, "      Total blocks: %u\n", info->total_blocks);
    mp_printf(&mp_plat_print, "Largest free block: %u\n", info->largest_free_block);
    mp_printf(&mp_plat_print, "  Allocated blocks: %u\n", info->allocated_blocks);
    mp_printf(&mp_plat_print, "       Free blocks: %u\n", info->free_blocks);
}

//---------------------------------------
static mp_obj_t esp_heap_info(void) {
    multi_heap_info_t info;

    mp_printf(&mp_plat_print, "Heap outside of MicroPython heap:\n---------------------------------\n");

    heap_caps_get_info(&info, MALLOC_CAP_INTERNAL | MALLOC_CAP_32BIT | MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
    print_heap_info(&info);

#if CONFIG_SPIRAM
#if CONFIG_SPIRAM_USE_MEMMAP
        mp_printf(&mp_plat_print, "\nSPIRAM info (MEMMAP used):\n--------------------------\n");
        mp_printf(&mp_plat_print, "            Total: %u\n", CONFIG_SPIRAM_SIZE);
        mp_printf(&mp_plat_print, "Used for MPy heap: %u\n", mpy_heap_size);
        mp_printf(&mp_plat_print, "  Free (not used): %u\n", CONFIG_SPIRAM_SIZE - mpy_heap_size);
#else
        mp_printf(&mp_plat_print, "\nSPIRAM info:\n------------\n");
        heap_caps_get_info(&info, MALLOC_CAP_SPIRAM);
        print_heap_info(&info);
#endif
#endif

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(esp_heap_info_obj, esp_heap_info);
//---------------------------------------



static const mp_rom_map_elem_t esp_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_esp) },

    { MP_ROM_QSTR(MP_QSTR_osdebug), MP_ROM_PTR(&esp_osdebug_obj) },

    { MP_ROM_QSTR(MP_QSTR_flash_read), MP_ROM_PTR(&esp_flash_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_flash_write), MP_ROM_PTR(&esp_flash_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_flash_erase), MP_ROM_PTR(&esp_flash_erase_obj) },
    { MP_ROM_QSTR(MP_QSTR_flash_size), MP_ROM_PTR(&esp_flash_size_obj) },
    { MP_ROM_QSTR(MP_QSTR_flash_user_start), MP_ROM_PTR(&esp_flash_user_start_obj) },

    { MP_ROM_QSTR(MP_QSTR_gpio_matrix_in), MP_ROM_PTR(&esp_gpio_matrix_in_obj) },
    { MP_ROM_QSTR(MP_QSTR_gpio_matrix_out), MP_ROM_PTR(&esp_gpio_matrix_out_obj) },

    // Constants for second arg of osdebug()
    { MP_ROM_QSTR(MP_QSTR_LOG_NONE), MP_ROM_INT((mp_uint_t)ESP_LOG_NONE)},
    { MP_ROM_QSTR(MP_QSTR_LOG_ERROR), MP_ROM_INT((mp_uint_t)ESP_LOG_ERROR)},
    { MP_ROM_QSTR(MP_QSTR_LOG_WARNING), MP_ROM_INT((mp_uint_t)ESP_LOG_WARN)},
    { MP_ROM_QSTR(MP_QSTR_LOG_INFO), MP_ROM_INT((mp_uint_t)ESP_LOG_INFO)},
    { MP_ROM_QSTR(MP_QSTR_LOG_DEBUG), MP_ROM_INT((mp_uint_t)ESP_LOG_DEBUG)},
    { MP_ROM_QSTR(MP_QSTR_LOG_VERBOSE), MP_ROM_INT((mp_uint_t)ESP_LOG_VERBOSE)},

    // Logging
    { MP_OBJ_NEW_QSTR(MP_QSTR_loglevel),    MP_ROM_PTR(&esp_log_level_obj) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_redirectlog), MP_ROM_PTR(&esp_logto_mp_obj) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_restorelog),  MP_ROM_PTR(&esp_logto_esp_obj) },

    // Heap
    { MP_ROM_QSTR(MP_QSTR_heap_info),       MP_ROM_PTR(&esp_heap_info_obj) },

};

static MP_DEFINE_CONST_DICT(esp_module_globals, esp_module_globals_table);

const mp_obj_module_t esp_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&esp_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR_esp, esp_module);
