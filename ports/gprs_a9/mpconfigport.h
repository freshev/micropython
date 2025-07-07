// Options to control how MicroPython is built for this port,
// overriding defaults in py/mpconfig.h.

#ifndef __MPCONFIGPORT_H
#define __MPCONFIGPORT_H

#define MICROPY_HW_BOARD_NAME   "A9/A9G module"
#define MICROPY_HW_MCU_NAME     "RDA8955"
#define MICROPY_PY_SYS_PLATFORM "gprs_a9"

// #define FOTA_URL "https://example.com/firmware_debug_OLDVERSION_to_NEWVERSION.pack" // moved to KConfig
#define MACHINE_HW_WATCHDOG_TIMEOUT (500)
#define MACHINE_HW_WATCHDOG_PIN (GPIO_PIN7)

#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include "time.h"
#include "api_sys.h"

#ifndef MICROPY_CONFIG_ROM_LEVEL
#define MICROPY_CONFIG_ROM_LEVEL            (MICROPY_CONFIG_ROM_LEVEL_EXTRA_FEATURES)
#endif

// Debug activation sometimes hangs A9 (too much memory consuming)
#define MICROPY_DEBUG_VERBOSE               (0)
#define MICROPY_DEBUG_PRINTER               (&mp_debug_print)

// object representation and NLR handling
#define MICROPY_OBJ_REPR                    (MICROPY_OBJ_REPR_A)
#define MICROPY_NLR_SETJMP                  (1)

// memory allocation policies
#define MICROPY_ALLOC_PATH_MAX              (128)
#define MICROPY_NO_ALLOCA                   (1) // otherwise, #incluide <alloc.h>

// optimisations
#ifndef MICROPY_OPT_COMPUTED_GOTO
#define MICROPY_OPT_COMPUTED_GOTO           (1)
#endif
#define MICROPY_OPT_MPZ_BITWISE             (1)

// Python internal features
#define MICROPY_PY_SYS_EXC_INFO             (1)
#define MICROPY_ENABLE_COMPILER             (1)
#define MICROPY_PY_SYS_STDIO_BUFFER         (0)
#define MICROPY_REPL_EVENT_DRIVEN           (0)
#define MICROPY_ENABLE_GC                   (1)
#define MICROPY_LONGINT_IMPL                (MICROPY_LONGINT_IMPL_MPZ)
#define MICROPY_FLOAT_IMPL                  (MICROPY_FLOAT_IMPL_DOUBLE)
#define MICROPY_ENABLE_FINALISER            (MICROPY_ENABLE_GC)
#define MICROPY_STACK_CHECK                 (1)
#define MICROPY_ENABLE_EMERGENCY_EXCEPTION_BUF (1)
#define MICROPY_KBD_EXCEPTION               (1)
#define MICROPY_HELPER_REPL                 (1)
#define MICROPY_REPL_EMACS_KEYS             (1)
#define MICROPY_REPL_AUTO_INDENT            (1)
#define MICROPY_ENABLE_SOURCE_LINE          (1)
#define MICROPY_ERROR_REPORTING             (MICROPY_ERROR_REPORTING_NORMAL)
#define MICROPY_WARNINGS                    (1)
#define MICROPY_CPYTHON_COMPAT              (1)
#define MICROPY_STREAMS_NON_BLOCK           (1)
#define MICROPY_USE_INTERNAL_ERRNO          (1)
#define MICROPY_USE_INTERNAL_PRINTF         (0)
#define MICROPY_PY_BUILTINS_STR_UNICODE     (1)
#define MICROPY_PY_BUILTINS_STR_UNICODE_CHECK (0)

#define MICROPY_BEGIN_ATOMIC_SECTION()      SYS_EnterCriticalSection()
#define MICROPY_END_ATOMIC_SECTION(state)   SYS_ExitCriticalSection(state)

// MCU definition
#define MP_ENDIANNESS_LITTLE                (1)

// control over Python builtins
#define MICROPY_PY_STR_BYTES_CMP_WARN       (1)
#define MICROPY_PY_BUILTINS_HELP_TEXT       gprs_a9_help_text
#define MICROPY_PY_IO_BUFFEREDWRITER        (1)
#define MICROPY_PY_TIME_GMTIME_LOCALTIME_MKTIME (1)
#define MICROPY_PY_TIME_TIME_TIME_NS        (1)
#define MICROPY_PY_TIME_INCLUDEFILE         "ports/gprs_a9/modtime.c"
#define MICROPY_PY_OS_UNAME                 (1)
#define MICROPY_PY_OS_DUPTERM               (1)
#define MICROPY_PY_OS_DUPTERM_NOTIFY        (1)
#define MICROPY_PY_OS_DUPTERM_STREAM_DETACHED_ATTACHED (1)

#define MICROPY_PY_MACHINE                  (1)
#define MICROPY_PY_MACHINE_INCLUDEFILE      "ports/gprs_a9/modmachine.c"
#define MICROPY_PY_MACHINE_BARE_METAL_FUNCS (1)
#define MICROPY_PY_THREAD                   (0)
#define MICROPY_PY_THREAD_GIL               (0)
#define MICROPY_PY_THREAD_GIL_VM_DIVISOR    (32)

#define MICROPY_PY_MACHINE_PIN_MAKE_NEW     mp_pin_make_new
#define MICROPY_PY_MACHINE_PULSE            (1)

#define MICROPY_PY_MACHINE_I2C              (1)
#define MICROPY_PY_MACHINE_SOFTI2C          (1)

#define MICROPY_PY_MACHINE_ADC              (1)
#define MICROPY_PY_MACHINE_ADC_INCLUDEFILE  "ports/gprs_a9/machine_adc.c"

#define MICROPY_PY_MACHINE_SPI              (1)
#define MICROPY_PY_MACHINE_SOFTSPI          (0)
#define MICROPY_HW_SOFTSPI_MIN_DELAY        (0)
#define MICROPY_HW_SOFTSPI_MAX_BAUDRATE     (500000)

#define MICROPY_PY_MACHINE_UART                         (1)
#define MICROPY_PY_MACHINE_UART_INCLUDEFILE             "ports/gprs_a9/machine_uart.c"
#define MICROPY_PY_MACHINE_UART_READCHAR_WRITECHAR      (1)
//#define MICROPY_PY_MACHINE_UART_SENDBREAK             (1)

#define MICROPY_PY_MACHINE_WDT              (1)
#define MICROPY_PY_MACHINE_WDT_INCLUDEFILE  "ports/gprs_a9/machine_wdt.c"
#define MICROPY_PY_MACHINE_WDT_TIMEOUT_MS   (1)


#define MICROPY_PY_SSL                      (1)
#define MICROPY_SSL_AXTLS                   (MICROPY_PY_SSL)
#define MICROPY_STREAMS_POSIX_API           (MICROPY_PY_SSL && MICROPY_SSL_AXTLS)
#define MICROPY_PY_CRYPTOLIB                (MICROPY_PY_SSL && MICROPY_SSL_AXTLS)
#define MICROPY_PY_HASHLIB_SHA1             (MICROPY_PY_SSL && MICROPY_SSL_AXTLS)
//#define MICROPY_SSL_MBEDTLS               (1) // failed to integrate GPRS_C_SDK mbedtls (too old) & mp mbedtls

// fatfs configuration
#define MICROPY_FATFS_ENABLE_LFN            (1)
#define MICROPY_FATFS_RPATH                 (2)
#define MICROPY_FATFS_MAX_SS                (4096)
#define MICROPY_FATFS_LFN_CODE_PAGE         437 /* 1=SFN/ANSI 437=LFN/U.S.(OEM) */

#define MICROPY_VFS                         (1)
#define MICROPY_VFS_FAT                     (1)
#define MICROPY_READER_VFS                  (1)

// emitters
#define MICROPY_PERSISTENT_CODE_LOAD        (1)

// type definitions for the specific machine
#define MICROPY_MAKE_POINTER_CALLABLE(p) ((void*)((mp_uint_t)(p) | 1))

// This port is intended to be 32-bit, but unfortunately, int32_t for
// different targets may be defined in different ways - either as int
// or as long. This requires different printf formatting specifiers
// to print such value. So, we avoid int32_t and use int directly.
#define UINT_FMT "%u"
#define INT_FMT "%d"
typedef int32_t  mp_int_t; // must be pointer size
typedef uint32_t mp_uint_t; // must be pointer size
typedef long mp_off_t;

//#define MP_PLAT_PRINT_STRN(str, len) mp_hal_stdout_tx_strn_cooked(str, len)

#define MICROPY_MPHALPORT_H "mphalport.h"

#define MP_STATE_PORT MP_STATE_VM

#if MICROPY_PY_THREAD
#define MICROPY_EVENT_POLL_HOOK \
    do { \
        extern void mp_handle_pending(bool); \
        mp_handle_pending(true); \
        MP_THREAD_GIL_EXIT(); \
        MP_THREAD_GIL_ENTER(); \
    } while (0);
#else
#define MICROPY_EVENT_POLL_HOOK \
    do { \
        extern void mp_handle_pending(bool); \
        mp_handle_pending(true); \
    } while (0);
#endif

#endif

#if MICROPY_DEBUG_VERBOSE
// printer for debugging output
extern const struct _mp_print_t mp_debug_print;
#endif

//#define mp_hal_stdio_poll(poll_flags) (0) // not implemented

#undef MP_UNREACHABLE
#define MP_UNREACHABLE for (;;);

// This may work only with a particular SDK version. At least this adress is valid for SW_V2129 to SW_V2131
//#define errno (*((volatile int *) 0x820a0cb0))
#define errno (*((int *) 0x820a0cb0))

