set_project("EC618")
set_xmakever("2.7.2")
set_version("0.0.2", {build = "%Y%m%d%H%M"})
add_rules("mode.debug", "mode.release")
set_defaultmode("debug")

local VM_64BIT = nil
SDK_TOP = "."
local SDK_PATH
local USER_PROJECT_NAME = "example"
local USER_PROJECT_NAME_VERSION
USER_PROJECT_DIR  = ""
local LUAT_SCRIPT_SIZE
local LUAT_SCRIPT_OTA_SIZE
local script_addr = nil
local full_addr = nil

package("gnu_rm")
    set_kind("toolchain")
    set_homepage("https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm")
    set_description("GNU Arm Embedded Toolchain")
    local version_map = {
        ["2021.10"] = "10.3-2021.10"
    }
    if is_host("windows") then
        set_urls("http://cdndownload.openluat.com/xmake/toolchains/gcc-arm/gcc-arm-none-eabi-$(version)-win32.zip", {version = function (version)
            return version_map[tostring(version)]
        end})
        add_versions("2021.10", "d287439b3090843f3f4e29c7c41f81d958a5323aecefcf705c203bfd8ae3f2e7")
    elseif is_host("linux") then
        set_urls("http://cdndownload.openluat.com/xmake/toolchains/gcc-arm/gcc-arm-none-eabi-$(version)-x86_64-linux.tar.bz2", {version = function (version)
            return version_map[tostring(version)]
        end})
        add_versions("2021.10", "97dbb4f019ad1650b732faffcc881689cedc14e2b7ee863d390e0a41ef16c9a3")
    elseif is_host("macosx") then
        set_urls("https://armkeil.blob.core.windows.net/developer/Files/downloads/gnu-rm/10.3-2021.10/gcc-arm-none-eabi-$(version)-mac.tar.bz2", {version = function (version)
            return version_map[tostring(version)]
        end})
        add_versions("2021.10", "fb613dacb25149f140f73fe9ff6c380bb43328e6bf813473986e9127e2bc283b")
    end
    on_install("@windows", "@linux", "@macosx", function (package)
        os.vcp("*", package:installdir())
    end)
package_end()

if os.getenv("GCC_PATH") then
    toolchain("arm_toolchain")
        set_kind("standalone")
        set_sdkdir(os.getenv("GCC_PATH"))
    toolchain_end()
    set_toolchains("arm_toolchain")
else
    add_requires("gnu_rm 2021.10")
    set_toolchains("gnu-rm@gnu_rm")
end

--Get project name
if os.getenv("PROJECT_NAME") then
    USER_PROJECT_NAME = os.getenv("PROJECT_NAME")
end

--Whether it is rndis csdk
if os.getenv("EC618_RNDIS") == "enable" then
    is_rndis = true
    add_defines("LUAT_EC618_RNDIS_ENABLED=1")
else
    is_rndis = false
end

--Whether to enable low-speed mode, which has larger memory but is not compatible with rndis
if is_rndis == false and os.getenv("LSPD_MODE") == "enable" then
    is_lspd = true
else
    is_lspd = false
end

--If is_lspd is enabled, add additional macros
if is_lspd == true then
    add_defines("LOW_SPEED_SERVICE_ONLY")
end

if os.getenv("ROOT_PATH") then
    SDK_TOP = os.getenv("ROOT_PATH")
else
    SDK_TOP = os.curdir()
end
SDK_TOP = SDK_TOP .. "/"
SDK_PATH = SDK_TOP

if os.getenv("PROJECT_DIR") then
    USER_PROJECT_DIR = os.getenv("PROJECT_DIR")
else
    USER_PROJECT_DIR = SDK_TOP .. "/project/" .. USER_PROJECT_NAME
end

set_plat("cross")
set_arch("arm")
set_languages("gnu99", "cxx11")
set_warnings("everything")

-- ==============================
--=== defines =====
add_defines("__EC618",
            "CHIP_EC618",
            "CORE_IS_AP",
            "SDK_REL_BUILD",
            "EC_ASSERT_FLAG",
            "PM_FEATURE_ENABLE",
            "UINILOG_FEATURE_ENABLE",
            "FEATURE_OS_ENABLE",
            "configUSE_NEWLIB_REENTRANT=1",
            "ARM_MATH_CM3",
            "FEATURE_YRCOMPRESS_ENABLE",
            "FEATURE_CCIO_ENABLE",
            "DHCPD_ENABLE_DEFINE=1",
            "LWIP_CONFIG_FILE=\"lwip_config_ec6180h00.h\"",
            "FEATURE_MBEDTLS_ENABLE",
            "LFS_NAME_MAX=63",
            "LFS_DEBUG_TRACE",
            "WDT_FEATURE_ENABLE=1",
            "FEATURE_UART_HELP_DUMP_ENABLE",
            "DEBUG_LOG_HEADER_FILE=\"debug_log_ap.h\"",
            "TRACE_LEVEL=5",
            "SOFTPACK_VERSION=\"\"",
            "HAVE_STRUCT_TIMESPEC",
            "HTTPS_WITH_CA",
            "FEATURE_HTTPC_ENABLE",
            -- "LITE_FEATURE_MODE",
            --"RTE_RNDIS_EN=0", "RTE_ETHER_EN=0",
            "RTE_USB_EN=1",
            "RTE_PPP_EN=0",
            "RTE_OPAQ_EN=0",
            "RTE_ONE_UART_AT=0",
            "RTE_TWO_UART_AT=0",
            "__USER_CODE__",
            "__PRINT_ALIGNED_32BIT__",
            "_REENT_SMALL",
            "_REENT_GLOBAL_ATEXIT"
)

if is_rndis then
    
else
    add_defines("LITE_FEATURE_MODE")
end

set_optimize("smallest")
add_cxflags("-g3",
            "-mcpu=cortex-m3",
            "-mthumb",
            "-std=gnu99",
            "-nostartfiles",
            "-mapcs-frame",
            "-ffunction-sections",
            "-fdata-sections",
            "-fno-isolate-erroneous-paths-dereference",
            "-freorder-blocks-algorithm=stc",
            -- "-Wall",
            "-Wno-incompatible-pointer-types",
            "-Wno-unused-but-set-variable",
            "-Wno-implicit-int",
            "-Wno-implicit-fallthrough",
            "-Wno-ignored-qualifiers",
            "-Wno-unused-parameter",
            "-Wno-unused-variable",
            "-Wno-unused-function",
            "-Wno-unused-label",
            "-Wno-enum-conversion",
            "-Wno-implicit-function-declaration",
            "-Wno-discarded-qualifiers",
            "-Wno-sign-compare",
            "-Wno-old-style-declaration",
            "-Wno-return-type",
            "-Wno-int-conversion",
            "-Wno-missing-field-initializers",
            "-Wno-switch",
            "-Wno-type-limits",
            "-Wno-pointer-sign",            
            "-Wno-format",
            "-gdwarf-2",
            "-fno-inline",
            "-mslow-flash-data",
            "-fstack-usage",
            "-Wstack-usage=4096",
{force=true})

add_cxflags("-Werror=maybe-uninitialized")

add_ldflags(" -Wl,--wrap=clock ",{force = true})
add_ldflags(" -Wl,--wrap=localtime ",{force = true})
add_ldflags(" -Wl,--wrap=gmtime ",{force = true})
add_ldflags(" -Wl,--wrap=time ",{force = true})
add_ldflags(" -Wl,--wrap=SetUnilogUart", {force=true})

add_ldflags("--specs=nano.specs", {force=true})
add_asflags("-Wl,--cref -Wl,--check-sections -Wl,--gc-sections -lm -Wl,--print-memory-usage -Wl,--wrap=_malloc_r -Wl,--wrap=_free_r -Wl,--wrap=_realloc_r  -mcpu=cortex-m3 -mthumb -DTRACE_LEVEL=5 -DSOFTPACK_VERSION=\"\" -DHAVE_STRUCT_TIMESPEC")

add_defines("sprintf=sprintf_")
add_defines("snprintf=snprintf_")
add_defines("vsnprintf=vsnprintf_")

--Enable network IP packet interception
if USER_PROJECT_NAME == "luatos" then
    add_defines("LUAT_NET_IP_INTERCEPT=1")
    add_ldflags("-Wl,--wrap=ps_ip_input",{force = true})
end

-- ==============================
--=== includes =====

add_includedirs(
                SDK_TOP .. "/PLAT/device/target/board/common/ARMCM3/inc",
                SDK_TOP .. "/PLAT/device/target/board/ec618_0h00/common/inc",
                SDK_TOP .. "/PLAT/device/target/board/ec618_0h00/ap/gcc",
                SDK_TOP .. "/PLAT/device/target/board/ec618_0h00/ap/inc",
                SDK_TOP .. "/PLAT/driver/board/ec618_0h00/inc",
                SDK_TOP .. "/PLAT/driver/board/ec618_0h00/inc/audio",
                SDK_TOP .. "/PLAT/driver/board/ec618_0h00/inc/camera",
                SDK_TOP .. "/PLAT/driver/board/ec618_0h00/inc/camera/bf30a2",
                SDK_TOP .. "/PLAT/driver/board/ec618_0h00/inc/camera/gc6153",
                SDK_TOP .. "/PLAT/driver/board/ec618_0h00/inc/camera/gc032A",
                SDK_TOP .. "/PLAT/driver/board/ec618_0h00/inc/camera/gc6123",
                SDK_TOP .. "/PLAT/driver/board/ec618_0h00/inc/camera/sp0A39",
                SDK_TOP .. "/PLAT/driver/board/ec618_0h00/inc/camera/sp0821",
                SDK_TOP .. "/PLAT/driver/board/ec618_0h00/inc/eeprom",
                SDK_TOP .. "/PLAT/driver/board/ec618_0h00/inc/lcd",
                SDK_TOP .. "/PLAT/driver/board/ec618_0h00/inc/lcd/ST7571",
                SDK_TOP .. "/PLAT/driver/board/ec618_0h00/inc/lcd/ST7789V2",
                SDK_TOP .. "/PLAT/driver/board/ec618_0h00/inc/ntc",
                SDK_TOP .. "/PLAT/driver/chip/ec618/ap/inc",
                SDK_TOP .. "/PLAT/driver/chip/ec618/ap/inc_cmsis",
                SDK_TOP .. "/PLAT/driver/hal/common/inc",
                SDK_TOP .. "/PLAT/driver/hal/ec618/ap/inc",
                SDK_TOP .. "/PLAT/os/freertos/inc",
                SDK_TOP .. "/PLAT/os/freertos/CMSIS/common/inc",
                SDK_TOP .. "/PLAT/os/freertos/CMSIS/ap/inc",
                SDK_TOP .. "/PLAT/os/freertos/portable/mem/tlsf",
                SDK_TOP .. "/PLAT/os/freertos/portable/gcc",
                SDK_TOP .. "/PLAT/middleware/developed/debug/inc",
                SDK_TOP .. "/PLAT/middleware/developed/nvram/inc",
                SDK_TOP .. "/PLAT/middleware/developed/cms/psdial/inc",
                SDK_TOP .. "/PLAT/middleware/developed/cms/cms/inc",
                SDK_TOP .. "/PLAT/middleware/developed/cms/psil/inc",
                SDK_TOP .. "/PLAT/middleware/developed/cms/psstk/inc",
                SDK_TOP .. "/PLAT/middleware/developed/cms/sockmgr/inc",
                SDK_TOP .. "/PLAT/middleware/developed/cms/cmsnetlight/inc",
                SDK_TOP .. "/PLAT/middleware/developed/ecapi/appmwapi/inc",
                SDK_TOP .. "/PLAT/middleware/developed/ecapi/psapi/inc",
                SDK_TOP .. "/PLAT/middleware/developed/common/inc",
                SDK_TOP .. "/PLAT/middleware/developed/psnv/inc",
                SDK_TOP .. "/PLAT/os/freertos/portable/gcc",
                SDK_TOP .. "/PLAT/middleware/developed/tcpipmgr/app/inc",
                SDK_TOP .. "/PLAT/middleware/developed/tcpipmgr/common/inc",
                SDK_TOP .. "/PLAT/os/freertos/inc",
                SDK_TOP .. "/PLAT/middleware/developed/yrcompress",
                SDK_TOP .. "/PLAT/os/freertos/portable/gcc",
                SDK_TOP .. "/PLAT/prebuild/PS/inc",
                SDK_TOP .. "/PLAT/middleware/thirdparty/lwip/src/include",
                SDK_TOP .. "/PLAT/middleware/thirdparty/lwip/src/include/lwip",
                SDK_TOP .. "/PLAT/middleware/developed/ccio/pub",
                SDK_TOP .. "/PLAT/middleware/developed/ccio/device/inc",
                SDK_TOP .. "/PLAT/middleware/developed/ccio/service/inc",
                SDK_TOP .. "/PLAT/middleware/developed/ccio/custom/inc",
                SDK_TOP .. "/PLAT/middleware/developed/fota/pub",
                SDK_TOP .. "/PLAT/middleware/developed/fota/custom/inc",
                SDK_TOP .. "/PLAT/middleware/developed/at/atdecoder/inc",
                SDK_TOP .. "/PLAT/middleware/developed/at/atps/inc",
                SDK_TOP .. "/PLAT/middleware/developed/at/atps/inc/cnfind",
                SDK_TOP .. "/PLAT/middleware/developed/at/atcust/inc",
                SDK_TOP .. "/PLAT/middleware/developed/at/atcust/inc/cnfind",
                SDK_TOP .. "/PLAT/middleware/developed/at/atentity/inc",
                SDK_TOP .. "/PLAT/middleware/developed/at/atreply/inc",
                SDK_TOP .. "/PLAT/middleware/developed/at/atref/inc",
                SDK_TOP .. "/PLAT/middleware/developed/at/atref/inc/cnfind",
                SDK_TOP .. "/PLAT/os/freertos/portable/gcc",
                SDK_TOP .. "/thirdparty/httpclient",
                SDK_TOP .. "/PLAT/os/freertos/portable/gcc",
                SDK_TOP .. "/PLAT/middleware/thirdparty/lwip/src/include",
                SDK_TOP .. "/PLAT/middleware/thirdparty/lwip/src/include/posix",
                SDK_TOP .. "/PLAT/os/freertos/inc",
                SDK_TOP .. "/PLAT/os/freertos/portable/gcc",
                SDK_TOP .. "/thirdparty/littlefs",
                SDK_TOP .. "/thirdparty/littlefs/port",
                SDK_TOP .. "/PLAT/os/freertos/portable/gcc",
                SDK_TOP .. "/PLAT/prebuild/PS/inc",
                SDK_TOP .. "/PLAT/prebuild/PLAT/inc",
                SDK_TOP .. "/PLAT/core/common/include",
                SDK_TOP .. "/PLAT/core/tts/include",
                SDK_TOP .. "/PLAT/core/multimedia/include",
                SDK_TOP .. "/PLAT/core/driver/include",
                SDK_TOP .. "/thirdparty/linksdk",
                SDK_TOP .. "/thirdparty/printf",
{public = true})

if USER_PROJECT_NAME ~= 'luatos' then
    add_defines("MBEDTLS_CONFIG_FILE=\"ssl_config.h\"")
    add_includedirs(SDK_TOP,
                    SDK_TOP .. "/interface/include", 
                    SDK_TOP .. "/interface/base_include", 
                    SDK_TOP .. "/interface/private_include", 
                    SDK_TOP .. "/thirdparty/mbedtls/include",
                    SDK_TOP .. "/thirdparty/mbedtls/include/mbedtls",
                    SDK_TOP .. "/thirdparty/mbedtls/configs",
                    SDK_TOP .. "/thirdparty/fal/inc",
                    SDK_TOP .. "/thirdparty/flashdb/inc",
                    {public = true})
else
    if os.getenv("LUAT_EC618_LITE_MODE") == "1" then
        add_defines("LUAT_EC618_LITE_MODE", "LUAT_SCRIPT_SIZE=448", "LUAT_SCRIPT_OTA_SIZE=284")
    end
    if os.getenv("LUAT_USE_TTS") == "1" then
        add_defines("LUAT_USE_TTS")
    end
    if os.getenv("LUAT_USE_TTS_ONCHIP") == "1" then
        add_defines("LUAT_USE_TTS_ONCHIP")
    end
    add_defines("__LUATOS__","LWIP_NUM_SOCKETS=8")
    add_defines("MBEDTLS_CONFIG_FILE=\"mbedtls_ec618_config.h\"")
end
--linkflags
local LD_BASE_FLAGS = "-Wl,--cref -Wl,--check-sections -Wl,--gc-sections -lm -Wl,--print-memory-usage"
LD_BASE_FLAGS = LD_BASE_FLAGS .. " -L" .. SDK_TOP .. "/PLAT/device/target/board/ec618_0h00/ap/gcc/"
--LD_BASE_FLAGS = LD_BASE_FLAGS .. " -T" .. SDK_TOP .. "/PLAT/device/target/board/ec618_0h00/ap/gcc/ec618_0h00_flash.ld -Wl,-Map,$(buildir)/"..USER_PROJECT_NAME.."/"..USER_PROJECT_NAME.."_$(mode).map "
LD_BASE_FLAGS = LD_BASE_FLAGS .. " -T" .. SDK_TOP .. "/PLAT/core/ld/ec618_0h00_flash.ld -Wl,-Map,$(buildir)/"..USER_PROJECT_NAME.."/"..USER_PROJECT_NAME.."_$(mode).map "
LD_BASE_FLAGS = LD_BASE_FLAGS .. " -Wl,--wrap=_malloc_r -Wl,--wrap=_free_r -Wl,--wrap=_realloc_r  -mcpu=cortex-m3 -mthumb -DTRACE_LEVEL=5 -DSOFTPACK_VERSION=\"\" -DHAVE_STRUCT_TIMESPEC"
local LIB_BASE = SDK_TOP .. "/PLAT/libs/libstartup.a "
LIB_BASE = LIB_BASE .. SDK_TOP .. "/PLAT/libs/libcore_airm2m.a "
LIB_BASE = LIB_BASE .. SDK_TOP .. "/PLAT/libs/libfreertos.a "
LIB_BASE = LIB_BASE .. SDK_TOP .. "/PLAT/libs/libpsnv.a "
LIB_BASE = LIB_BASE .. SDK_TOP .. "/PLAT/libs/libtcpipmgr.a "
LIB_BASE = LIB_BASE .. SDK_TOP .. "/PLAT/libs/libyrcompress.a "
LIB_BASE = LIB_BASE .. SDK_TOP .. "/PLAT/libs/libmiddleware_ec.a "
LIB_BASE = LIB_BASE .. SDK_TOP .. "/PLAT/libs/liblwip.a "
if os.getenv("LUAT_FAST_ADD_USER_LIB") == "1" then
    LIB_BASE = LIB_BASE .. SDK_TOP .. os.getenv("USER_LIB") .. " "
end

if is_rndis then
    LIB_PS_PRE = SDK_TOP .. "/PLAT/prebuild/PS/lib/gcc"
    LIB_PLAT_PRE = SDK_TOP .. "/PLAT/prebuild/PLAT/lib/gcc"
else
    LIB_PS_PRE = SDK_TOP .. "/PLAT/prebuild/PS/lib/gcc/lite"
    LIB_PLAT_PRE = SDK_TOP .. "/PLAT/prebuild/PLAT/lib/gcc/lite"
end
LIB_BASE = LIB_BASE .. LIB_PS_PRE .. "/libps.a "
LIB_BASE = LIB_BASE .. LIB_PS_PRE .. "/libpsl1.a "
LIB_BASE = LIB_BASE .. LIB_PS_PRE .. "/libpsif.a "
LIB_BASE = LIB_BASE .. LIB_PLAT_PRE .. "/libosa.a "
LIB_BASE = LIB_BASE .. LIB_PLAT_PRE .. "/libmiddleware_ec_private.a "
LIB_BASE = LIB_BASE .. LIB_PLAT_PRE .. "/libccio.a "
LIB_BASE = LIB_BASE .. LIB_PLAT_PRE .. "/libdeltapatch.a "
LIB_BASE = LIB_BASE .. LIB_PLAT_PRE .. "/libfota.a "
LIB_BASE = LIB_BASE .. LIB_PLAT_PRE .. "/libdriver_private.a "
LIB_BASE = LIB_BASE .. LIB_PLAT_PRE .. "/libusb_private.a "
LIB_USER = ""

after_load(function (target)
    for _, sourcebatch in pairs(target:sourcebatches()) do
        if sourcebatch.sourcekind == "as" then --only asm files
            for idx, objectfile in ipairs(sourcebatch.objectfiles) do
                sourcebatch.objectfiles[idx] = objectfile:gsub("%.S%.o", ".o")
            end
        end
        if sourcebatch.sourcekind == "cc" then --only c files
            for idx, objectfile in ipairs(sourcebatch.objectfiles) do
                sourcebatch.objectfiles[idx] = objectfile:gsub("%.c%.o", ".o")
            end
        end
    end
end)

target("driver")
    set_kind("static")
    add_deps(USER_PROJECT_NAME)
    --driver
    add_files(SDK_TOP .. "/PLAT/driver/board/ec618_0h00/src/**.c",
                SDK_TOP .. "/PLAT/driver/chip/ec618/ap/**.c",
                SDK_TOP .. "/PLAT/driver/chip/ec618/common/gcc/memcpy-armv7m.S",
                SDK_TOP .. "/PLAT/driver/hal/**.c",
                SDK_TOP .. "/PLAT/core/speed/*.c"
    )
    
    remove_files(SDK_TOP .. "/PLAT/driver/board/ec618_0h00/src/camera/camAT.c",
                SDK_TOP .. "/PLAT/driver/board/ec618_0h00/src/exstorage/*.c",
                SDK_TOP.."/PLAT/driver/chip/ec618/ap/src/usb/usb_device/usb_bl_test.c",
                SDK_TOP.."/PLAT/driver/chip/ec618/ap/src_cmsis/bsp_lpusart_stub.c",
                SDK_TOP.."/PLAT/driver/chip/ec618/ap/src/tls.c",
                SDK_TOP.."/PLAT/driver/chip/ec618/ap/src_cmsis/bsp_spi.c"
    )

    set_targetdir("$(buildir)/libdriver_" .. USER_PROJECT_NAME)
target_end()

includes(USER_PROJECT_DIR)

target(USER_PROJECT_NAME..".elf")
    set_kind("binary")
    -- add_deps(USER_PROJECT_NAME)
    set_targetdir("$(buildir)/"..USER_PROJECT_NAME)
    add_deps("driver")
    --if os.getenv("GCC_PATH") then
    --LD_BASE_FLAGS = " --specs=nano.specs " .. LD_BASE_FLAGS
    -- end

    if USER_PROJECT_NAME ~= 'luatos' then
        add_files(SDK_TOP .. "/interface/private_src/*.c",{public = true})
        add_files(SDK_TOP .. "/thirdparty/mbedtls/library/*.c",{public = true})
        add_files(SDK_TOP .. "/thirdparty/printf/*.c",{public = true})
        add_files(SDK_TOP.."/thirdparty/fal/src/*.c",{public = true})
        add_files(SDK_TOP.."/thirdparty/flashdb/src/*.c",{public = true})
        add_files(SDK_TOP .. "/interface/src/*.c",{public = true})
        add_files(SDK_TOP .. "/thirdparty/littlefs/**.c",{public = true})
    else
        remove_files(SDK_TOP .. "/interface/src/luat_kv_ec618.c")
    end
    

    add_ldflags(LD_BASE_FLAGS .. " -Wl,--whole-archive -Wl,--start-group " .. LIB_BASE .. LIB_USER .. " -Wl,--end-group -Wl,--no-whole-archive -Wl,--no-undefined -Wl,--no-print-map-discarded  -ldriver", {force=true})
    
    on_load(function (target)
        if USER_PROJECT_NAME == 'luatos' then
            local conf_data = io.readfile("$(projectdir)/project/luatos/inc/luat_conf_bsp.h")
            USER_PROJECT_NAME_VERSION = conf_data:match("#define LUAT_BSP_VERSION \"(%w+)\"")
            VM_64BIT = conf_data:find("\r#define LUAT_CONF_VM_64bit") or conf_data:find("\n#define LUAT_CONF_VM_64bit")
            local TTS_ONCHIP = conf_data:find("\r#define LUAT_USE_TTS_ONCHIP") or conf_data:find("\n#define LUAT_USE_TTS_ONCHIP")
            local TLS_DISABLE = conf_data:find("\r#define LUAT_USE_TLS_DISABLE") or conf_data:find("\n#define LUAT_USE_TLS_DISABLE")

            local mem_map_data = io.readfile("$(projectdir)/PLAT/device/target/board/ec618_0h00/common/inc/mem_map.h")
            FLASH_FOTA_REGION_START = tonumber(mem_map_data:match("#define FLASH_FOTA_REGION_START%s+%((%g+)%)"))
            if (TTS_ONCHIP or os.getenv("LUAT_USE_TTS_ONCHIP") == "1") and not TLS_DISABLE then
                LUAT_SCRIPT_SIZE = 64
                LUAT_SCRIPT_OTA_SIZE = 48
            elseif os.getenv("LUAT_EC618_LITE_MODE") == "1" then
                LUAT_SCRIPT_SIZE = 448
                LUAT_SCRIPT_OTA_SIZE = 284
            else
                LUAT_SCRIPT_SIZE = tonumber(conf_data:match("\r#define LUAT_SCRIPT_SIZE (%d+)") or conf_data:match("\n#define LUAT_SCRIPT_SIZE (%d+)"))
                LUAT_SCRIPT_OTA_SIZE = tonumber(conf_data:match("\r#define LUAT_SCRIPT_OTA_SIZE (%d+)") or conf_data:match("\n#define LUAT_SCRIPT_OTA_SIZE (%d+)"))
            end
            print(string.format("script zone %d ota %d", LUAT_SCRIPT_SIZE, LUAT_SCRIPT_OTA_SIZE))
            LUA_SCRIPT_ADDR = FLASH_FOTA_REGION_START - (LUAT_SCRIPT_SIZE + LUAT_SCRIPT_OTA_SIZE) * 1024
            LUA_SCRIPT_OTA_ADDR = FLASH_FOTA_REGION_START - LUAT_SCRIPT_OTA_SIZE * 1024
            script_addr = string.format("%X", LUA_SCRIPT_ADDR)
            full_addr = string.format("%X", LUA_SCRIPT_OTA_ADDR)
            -- print(FLASH_FOTA_REGION_START,LUAT_SCRIPT_SIZE,LUAT_SCRIPT_OTA_SIZE)
            -- print(script_addr,full_addr)
            
        end
    end)
    before_build(function(target)
        if os.getenv("GCC_PATH") then
            GCC_DIR = os.getenv("GCC_PATH").."/"
        else
            GCC_DIR = target:toolchains()[1]:sdkdir().."/"
        end
        if os.getenv("LSPD_MODE") == "enable" then
            FLAGS = "-DLOW_SPEED_SERVICE_ONLY"
        else
            FLAGS = ""
        end
        if USER_PROJECT_NAME == "luatos" then
            FLAGS = FLAGS .. " -D__LUATOS__ -DFLASH_AREA_SIZE=" .. string.format("%dK", 2944 - LUAT_SCRIPT_SIZE - LUAT_SCRIPT_OTA_SIZE)
        end
        os.exec(GCC_DIR .. "bin/arm-none-eabi-gcc -E " .. FLAGS .. " -I " .. SDK_PATH .. "/PLAT/device/target/board/ec618_0h00/common/inc" .. " -P " .. SDK_PATH .. "/PLAT/core/ld/ec618_0h00_flash.c" ..  " -o " .. SDK_PATH .. "/PLAT/core/ld/ec618_0h00_flash.ld")
        
    end)
    after_build(function(target)
        if os.getenv("GCC_PATH") then
            GCC_DIR = os.getenv("GCC_PATH").."/"
        else
            GCC_DIR = target:toolchains()[1]:sdkdir().."/"
        end
        OUT_PATH = SDK_PATH .. "/out/" ..USER_PROJECT_NAME
        if not os.exists(OUT_PATH) then
            os.mkdir(OUT_PATH)
        end
        os.exec(GCC_DIR .. "bin/arm-none-eabi-objcopy -O binary $(buildir)/"..USER_PROJECT_NAME.."/"..USER_PROJECT_NAME..".elf $(buildir)/"..USER_PROJECT_NAME.."/"..USER_PROJECT_NAME..".bin")
        --io.writefile("$(buildir)/"..USER_PROJECT_NAME.."/"..USER_PROJECT_NAME..".list", os.iorun(GCC_DIR .. "bin/arm-none-eabi-objdump -h -S $(buildir)/"..USER_PROJECT_NAME.."/"..USER_PROJECT_NAME..".elf"))
        io.writefile("$(buildir)/"..USER_PROJECT_NAME.."/"..USER_PROJECT_NAME..".size", os.iorun(GCC_DIR .. "bin/arm-none-eabi-size $(buildir)/"..USER_PROJECT_NAME.."/"..USER_PROJECT_NAME..".elf"))
        -- io.cat("$(buildir)/"..USER_PROJECT_NAME.."/"..USER_PROJECT_NAME..".size")
        os.exec(GCC_DIR .. "bin/arm-none-eabi-objcopy -O binary $(buildir)/"..USER_PROJECT_NAME.."/"..USER_PROJECT_NAME..".elf $(buildir)/"..USER_PROJECT_NAME.."/"..USER_PROJECT_NAME..".bin")
        os.exec(GCC_DIR .."bin/arm-none-eabi-size $(buildir)/"..USER_PROJECT_NAME.."/"..USER_PROJECT_NAME..".elf")
        os.cp("$(buildir)/"..USER_PROJECT_NAME.."/"..USER_PROJECT_NAME..".bin", "$(buildir)/"..USER_PROJECT_NAME.."/ap.bin")
        
        os.cp("$(buildir)/"..USER_PROJECT_NAME.."/*.bin", OUT_PATH)
        os.cp("$(buildir)/"..USER_PROJECT_NAME.."/*.map", OUT_PATH)
        os.cp("$(buildir)/"..USER_PROJECT_NAME.."/*.elf", OUT_PATH)
        os.cp("./PLAT/comdb.txt", OUT_PATH)

        ---------------------------------------------------------
        -------------- This part is not yet cross-platform
        local cmd = "-M -input ./PLAT/tools/ap_bootloader.bin -addrname  BL_IMG_MERGE_ADDR -flashsize BOOTLOADER_FLASH_LOAD_SIZE -input $(buildir)/"..USER_PROJECT_NAME.."/ap.bin -addrname  AP_IMG_MERGE_ADDR -flashsize AP_FLASH_LOAD_SIZE -input ./PLAT/prebuild/FW/lib/cp-demo-flash.bin -addrname CP_IMG_MERGE_ADDR -flashsize CP_FLASH_LOAD_SIZE -def ./PLAT/device/target/board/ec618_0h00/common/inc/mem_map.h "
        if os.getenv("BINPKG_CROSS") then
            --Prepare a custom packager
            cmd = os.getenv("BINPKG_CROSS") .. cmd
        else
            if is_plat("windows") then
                cmd = "./PLAT/tools/fcelf.exe " .. cmd
            elseif is_plat("macosx") then
                cmd = "bash ./fcelf-docker.sh " .. cmd
            else
                cmd = "./fcelf " .. cmd
            end
        end
        cmd = cmd .. " -outfile " .. "./out/" ..USER_PROJECT_NAME.."/"..USER_PROJECT_NAME..".binpkg"
        --If your platform does not have fcelf, you can comment out the following line and no binpkg will be generated.
        --You can still use other tools to continue flashing your phone
        print("fcelf CMD --> ", cmd)
        os.exec(cmd)
        ---------------------------------------------------------

        import("lib.detect.find_file")
        local path7z = nil
        if is_plat("windows") then
            path7z = "\"$(programdir)/winenv/bin/7z.exe\""
        elseif is_plat("linux") or is_plat("macosx") then
            path7z = find_file("7z", { "/usr/bin/", "/usr/local/bin/" })
            if not path7z then
                path7z = find_file("7zr", { "/usr/bin/"})
            end
        end
        if path7z == nil then
            print("7z not find")
            return
        end
        import("core.base.json")
        if USER_PROJECT_NAME == 'luatos' then
            os.cp("$(projectdir)/project/luatos/pack", OUT_PATH)
            local info_table = json.loadfile(OUT_PATH.."/pack/info.json")
            if VM_64BIT then
                info_table["script"]["bitw"] = 64
            end
            if script_addr then
                info_table["download"]["script_addr"] = script_addr
                info_table["rom"]["fs"]["script"]["size"] = LUAT_SCRIPT_SIZE
                io.gsub(OUT_PATH.."/pack/config_ec618_usb.ini", "filepath = .\\script.bin\nburnaddr = 0x(%g+)", "filepath = .\\script.bin\nburnaddr = 0x"..script_addr)
            end
            if full_addr then
                info_table["fota"]["full_addr"] = full_addr
            end
            json.savefile(OUT_PATH.."/pack/info.json", info_table)
            os.cp(OUT_PATH.."/luatos.binpkg", OUT_PATH.."/pack")
            os.cp(OUT_PATH.."/luatos.elf", OUT_PATH.."/pack")
            os.cp(OUT_PATH.."/*.map", OUT_PATH.."/pack")
            os.cp("./PLAT/comdb.txt", OUT_PATH.."/pack")
            os.cp("./PLAT/device/target/board/ec618_0h00/common/inc/mem_map.h", OUT_PATH .. "/pack")
            os.cp("$(projectdir)/project/luatos/inc/luat_conf_bsp.h", OUT_PATH.."/pack")
            os.exec(path7z.." a -mx9 LuatOS-SoC_"..USER_PROJECT_NAME_VERSION.."_EC618.7z "..OUT_PATH.."/pack/* -r")
            local ver = "_FULL"
            if os.getenv("LUAT_EC618_LITE_MODE") == "1" then
                ver = ""
            end
            if os.getenv("LUAT_USE_TTS") == "1" then
                ver = "_TTS"
                if os.getenv("LUAT_USE_TTS_ONCHIP") == "1" then
                    ver = "_TTS_ONCHIP"
                end
            end
            os.mv("LuatOS-SoC_"..USER_PROJECT_NAME_VERSION.."_EC618.7z", OUT_PATH.."/LuatOS-SoC_"..USER_PROJECT_NAME_VERSION.."_EC618"..ver..".soc")
            os.rm(OUT_PATH.."/pack")
        else 
            if not os.exists(OUT_PATH.."/pack") then
                os.mkdir(OUT_PATH.."/pack")
            end
            os.cp(OUT_PATH.."/*.binpkg", OUT_PATH.."/pack")
            os.cp(OUT_PATH.."/*.elf", OUT_PATH.."/pack")
            os.cp(OUT_PATH.."/*.map", OUT_PATH.."/pack")
            os.cp("./PLAT/comdb.txt", OUT_PATH.."/pack")
            os.cp("$(projectdir)/project/luatos/pack/info.json", OUT_PATH.."/pack")
            local info_table = json.loadfile(OUT_PATH.."/pack/info.json")
            info_table["rom"]["file"] = USER_PROJECT_NAME .. ".binpkg"
            json.savefile(OUT_PATH.."/pack/info.json", info_table)
            os.cp("./PLAT/device/target/board/ec618_0h00/common/inc/mem_map.h", OUT_PATH .. "/pack")
            os.exec(path7z.." a -mx9 "..USER_PROJECT_NAME.."_ec618.7z "..OUT_PATH.."/pack/* -r")
            os.mv(USER_PROJECT_NAME.."_ec618.7z", OUT_PATH.."/"..USER_PROJECT_NAME.."_ec618.soc")
            os.rm(OUT_PATH.."/pack")
        end

        --To calculate the differential package size, you need to put the old binpkg in the root directory and name it $project name.binpkg
        if os.exists(USER_PROJECT_NAME .. ".binpkg") then
            os.cp("./PLAT/tools/fcelf.exe", "tools/dtools/dep/fcelf.exe")
            os.cp(OUT_PATH.."/"..USER_PROJECT_NAME..".binpkg", "tools/dtools/new.binpkg")
            os.cp(USER_PROJECT_NAME .. ".binpkg", "tools/dtools/old.binpkg")
            os.exec("tools\\dtools\\run.bat BINPKG delta.par " .. USER_PROJECT_NAME .. ".binpkg new.binpkg")
        end
    end)
target_end()
