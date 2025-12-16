set_project("EC618")
set_xmakever("2.7.2")
set_version("0.0.2", {build = "%Y%m%d%H%M"})
add_rules("mode.debug", "mode.release")
set_defaultmode("debug")

local VM_64BIT = nil
SDK_TOP = "."
local SDK_PATH
local USER_PROJECT_NAME = "micropython_Air780"
USER_PROJECT_DIR  = ""
local LUAT_SCRIPT_SIZE
local LUAT_SCRIPT_OTA_SIZE
local script_addr = nil
local full_addr = nil

-- to configure board run: xmake f --menu
option("01 Board type")
    set_default(true)
    set_description("Board type")
    set_default("Air780_GENERIC")
    set_values("Air780_GENERIC", "Air780_EG", "XMAKE_TEST_BOARD")
option("020 Firmware version")
    set_description("Firmware version")
    set_default("v1.0")
option("021 MAIN_STUB_URL")
    set_description("URL to get main.py from")
    set_default("")
option("022 ZIP_COMPRESS")
    set_description("Get main.py from URL in compressed form")
    set_default(true)
option("03 REPL port")
    set_description("REPL port")
    set_default("REPL over USB")
    set_values("REPL over USB", "UART1", "UART2")
option("04 GPIO8-11 mux")
    set_description("GPIO8-11 mux")
    set_default("UART2 & I2C1")
    set_values("UART2 & I2C1", "SPI0")
option("04 GPIO12-15 mux") -- supported only for Air780ep !
    set_description("GPIO12-15 mux")
    set_default("UART0 & I2C0")
    set_values("UART0 & I2C0", "SPI1")
option("05 Main stub respawn")
    set_description("Respawned on delete main.py")
    set_default(true)
    set_showmenu(true)
option("06 Main stub autorun")
    set_description("Auto run main.py on load")
    set_default(true)
    set_showmenu(true)
option("07 Reset on SMS")
    set_description("Reset module on incoming SMS 'reset'")
    set_default(true)
    set_showmenu(true)
option("08 Acknowledge SMS on reset")
    set_description("Send acknowledge SMS 'Done' on reset")
    set_default(true)
    set_showmenu(true)
option("09 Configiration by SMS")
    set_description("Set module configiration by SMS")
    set_default(true)
    set_showmenu(true)
option("10 Dump and halt on core exception")
    set_description("Dump and halt on core exception")
    set_default(false)
    set_showmenu(true)
option("11 Include DHT module")
    set_description("Include DHT module")
    set_default(false)
    set_showmenu(true)
option("12 Include CC1101 module")
    set_description("Include CC1101 module")
    set_default(false)
    set_showmenu(true)

option("1 RS485_UART1_USE")
    set_description("UART1 connected to RS-485")
    set_showmenu(true)
    set_category("RS-485/UART1")
    set_default(false)    
option("2 RS485_UART1_PIN")
    set_description("UART1 RS-485 control pin")
    set_showmenu(true)
    set_category("RS-485/UART1")
    set_values(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 15, 16, 17, 23, 24, 25, 26, 27, 28, 29, 30, 31)
    set_default(16)    
option("3 RS485_UART1_DELAY")
    set_description("Delay after control pin flip (us)")
    set_category("RS-485/UART1")
    set_default("5000");
option("4 RS485_UART1_PIN_LEVEL")
    set_description("Control pin level to receive")
    set_category("RS-485/UART1")
    set_default(0);
    set_values(0, 1)
option("5 RS485_UART1_WAIT_TX")
    set_description("Wait TX done (synchronious TX)")
    set_category("RS-485/UART1")    set_default(true);


option("1 RS485_UART2_USE")
    set_description("UART2 connected to RS-485")
    set_showmenu(true)
    set_category("RS-485/UART2")
    set_default(false)    
option("2 RS485_UART2_PIN")
    set_description("UART2 RS-485 control pin")
    set_showmenu(true)
    set_category("RS-485/UART2")
    set_values(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 15, 16, 17, 23, 24, 25, 26, 27, 28, 29, 30, 31)
    set_default(30)    
option("3 RS485_UART2_DELAY")
    set_description("Delay after control pin flip (us)")
    set_category("RS-485/UART2")
    set_default("5000");
option("4 RS485_UART2_PIN_LEVEL")
    set_description("Control pin level to receive")
    set_category("RS-485/UART2")
    set_default(0);
    set_values(0, 1)
option("5 RS485_UART2_WAIT_TX")
    set_description("Wait TX done (synchronious TX)")
    set_category("RS-485/UART2")
    set_default(true);

option("1 Use FOTA routines")
    set_description("Use FOTA routines (minus 32K from micropython RAM)")
    set_category("FOTA")
    set_default(false)    
option("2 FOTA URL")
    set_category("FOTA")
    set_description("URL for FOTA")
    set_default(false)
    set_default("https://example.com/Air780_OLDVERSION_to_NEWVERSION.pack")
option("3 Remove python files on upgrade")
    set_category("FOTA")
    set_description("Remove all python files on upgrade")
    set_default(false)    

option("1 Deploy binpkg")
    set_category("Deploy")
    set_description("Deploy binpkg to folder")
    set_default("")
option("2 Deploy binpkg")
    set_category("Deploy")
    set_description("Deploy binpkg in compressed form")
    set_default(true)
option("3 Deploy pack")
    set_category("Deploy")
    set_description("Deploy FOTA pack to folder")
    set_default("")
option("4 Deploy pack")
    set_category("Deploy")
    set_description("Deploy FOTA pack in compressed form")
    set_default(false)


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

add_requires("python 3.10")
add_packages("python")

-- install sh, make, cat, touch, others --  
-- add_requires("msys2", {configs = {base_devel = true}}) 
-- add_packages("msys2")
                     
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

-- If is_lspd is enabled, add additional macros
if is_lspd == true then
    add_defines("LOW_SPEED_SERVICE_ONLY")
end

if os.getenv("ROOT_PATH") then
    SDK_TOP = os.getenv("ROOT_PATH")
else
    SDK_TOP = os.curdir()
end

SDK_TOP = SDK_TOP .. "/../../lib/luatos-soc-2022"
SDK_PATH = SDK_TOP


if os.getenv("PROJECT_DIR") then
    USER_PROJECT_DIR = os.getenv("PROJECT_DIR")
else
    USER_PROJECT_DIR = "./"
end

set_plat("cross")
set_arch("arm")
set_languages("gnu99", "cxx11")
set_warnings("everything")

-- ==============================
--=== defines =====
DEFINES={   "__EC618",
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
            "LFS_NAME_MAX=63",
            "LFS_DEBUG_TRACE",
            "WDT_FEATURE_ENABLE=1",
            "FEATURE_UART_HELP_DUMP_ENABLE",
            "FEATURE_MBEDTLS_ENABLE",
            "MBEDTLS_CONFIG_FILE=\"ssl_config.h\"",
            "DEBUG_LOG_HEADER_FILE=\"debug_log_ap.h\"",            
            "TRACE_LEVEL=5",
            -- "SOFTPACK_VERSION=\"\"",
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
            "_REENT_GLOBAL_ATEXIT",
}
add_defines(DEFINES)

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
            "-Wall",            
            "-Wno-unused-but-set-variable",
            "-Wno-ignored-qualifiers",
            "-Wno-unused-parameter",
            "-Wno-unused-variable",
            "-Wno-unused-function",
            "-Wno-unused-label",
            "-Wno-enum-conversion",
            "-Wno-implicit-function-declaration",
            "-Wno-sign-compare",
            "-Wno-old-style-declaration",
            "-Wno-return-type",
            "-Wno-missing-field-initializers",
            "-Wno-cast-function-type",
            "-Wno-discarded-qualifiers",
            "-Wno-array-bounds", 
            --"-Wno-int-conversion",
            --"-Wno-switch",
            --"-Wno-incompatible-pointer-types",
            --"-Wno-type-limits",
            --"-Wno-pointer-sign",            
            --"-Wno-format",
            --"-Wno-implicit-int",
            --"-Wno-implicit-fallthrough",
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

-- ==============================
--=== includes =====
INCLUDES={
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
                SDK_TOP .. "/PLAT/middleware/developed/tcpipmgr/app/inc",
                SDK_TOP .. "/PLAT/middleware/developed/tcpipmgr/common/inc",
                SDK_TOP .. "/PLAT/os/freertos/inc",
                SDK_TOP .. "/PLAT/prebuild/PS/inc",                
                SDK_TOP .. "/PLAT/middleware/developed/ccio/pub",
                SDK_TOP .. "/PLAT/middleware/developed/ccio/device/inc",
                SDK_TOP .. "/PLAT/middleware/developed/ccio/service/inc",
                SDK_TOP .. "/PLAT/middleware/developed/ccio/custom/inc",
                SDK_TOP .. "/PLAT/middleware/developed/fota/pub",
                SDK_TOP .. "/PLAT/middleware/developed/fota/custom/inc",
                SDK_TOP .. "/thirdparty/libhttp",
                SDK_TOP .. "/thirdparty/httpclient",
                SDK_TOP .. "/PLAT/middleware/thirdparty/lwip/src/include",
                SDK_TOP .. "/PLAT/middleware/thirdparty/lwip/src/include/posix",
                SDK_TOP .. "/PLAT/os/freertos/inc",
                SDK_TOP .. "/PLAT/prebuild/PS/inc",
                SDK_TOP .. "/PLAT/prebuild/PLAT/inc",
                SDK_TOP .. "/PLAT/core/common/include",
                SDK_TOP .. "/PLAT/core/tts/include",
                SDK_TOP .. "/PLAT/core/multimedia/include",
                SDK_TOP .. "/PLAT/core/driver/include",
                SDK_TOP .. "/interface/include", 
                SDK_TOP .. "/interface/base_include", 
                SDK_TOP .. "/interface/private_include", 
                SDK_TOP .. "/PLAT/middleware/thirdparty/lwip/src/include",      
                SDK_TOP .. "/PLAT/middleware/thirdparty/lwip/src/include/lwip", 
                SDK_TOP .. "/thirdparty/littlefs",
                SDK_TOP .. "/thirdparty/littlefs/port",
                SDK_TOP .. "/thirdparty/mbedtls/include",
                SDK_TOP .. "/thirdparty/mbedtls/configs",
                SDK_TOP .. "/thirdparty/fal/inc",
                SDK_TOP .. "/thirdparty/flashdb/inc",
                SDK_TOP .. "/thirdparty/linksdk",
                SDK_TOP .. "/thirdparty/printf",
                SDK_TOP .. "/thirdparty/cJSON",
                SDK_TOP .. "/luatos_lwip_socket/include",
                SDK_TOP .. "/thirdparty/minmea",
         }
add_includedirs(INCLUDES, {public = true})

--linkflags
local LD_BASE_FLAGS = "-Wl,--cref -Wl,--check-sections -Wl,--gc-sections -lm -Wl,--print-memory-usage"
LD_BASE_FLAGS = LD_BASE_FLAGS .. " -L" .. SDK_TOP .. "/PLAT/device/target/board/ec618_0h00/ap/gcc/"
-- LD_BASE_FLAGS = LD_BASE_FLAGS .. " -T" .. SDK_TOP .. "/PLAT/device/target/board/ec618_0h00/ap/gcc/ec618_0h00_flash.ld -Wl,-Map,$(buildir)/"..USER_PROJECT_NAME.."/"..USER_PROJECT_NAME.."_$(mode).map "
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

--------------------------------------------------------
---------------------- Parse configs -------------------
--------------------------------------------------------
local BOARD = "Air780_GENERIC"
local USER_PROJECT_NAME_VERSION = "v1.0"
local FW_VERSION = USER_PROJECT_NAME_VERSION
local HW_UART_REPL=0x20 -- LUAT_VUART_ID_0
local RTE_UART0 = 1
local RTE_UART2 = 1
local RTE_I2C0 = 1
local RTE_I2C1 = 1
local RTE_SPI0 = 1
local RTE_SPI1 = 1
local MAIN_STUB_URL = ""
local ZIP_COMPRESS = false
local DEPLOY_BINPKG_FOLDER = ""
local DEPLOY_BINPKG_COMPRESS = true
local DEPLOY_PACK_FOLDER = ""
local DEPLOY_PACK_COMPRESS = false

if get_config("01 Board type") ~= nil then
    if os.isdir("boards/" .. get_config("01 Board type")) then BOARD = get_config("01 Board type") end
end

if get_config("020 Firmware version") ~= nil then
    USER_PROJECT_NAME_VERSION = get_config("020 Firmware version")
    FW_VERSION = USER_PROJECT_NAME_VERSION
end
if get_config("021 MAIN_STUB_URL") ~= nil then
    MAIN_STUB_URL = get_config("021 MAIN_STUB_URL")
end
if get_config("022 ZIP_COMPRESS") ~= nil then
    ZIP_COMPRESS = get_config("022 ZIP_COMPRESS")
end

if get_config("03 REPL port") == "REPL over USB" then HW_UART_REPL=0x20 end
if get_config("03 REPL port") == "UART1" then HW_UART_REPL=1 end
if get_config("03 REPL port") == "UART2" then HW_UART_REPL=2 end
if get_config("04 GPIO8-11 mux") == "UART2 & I2C1" then 
    RTE_SPI0 = 0 
end

if get_config("04 GPIO8-11 mux") == "SPI0" then
   RTE_UART2 = 0
   RTE_I2C1 = 0
end

if get_config("04 GPIO12-15 mux") == "UART0 & I2C0" then 
   RTE_SPI1 = 0 
end

if get_config("04 GPIO12-15 mux") == "SPI1" then
  RTE_UART0 = 0
  RTE_I2C0 = 0
end

table.insert(DEFINES, "FW_VERSION=\"" .. FW_VERSION .. "\"")
table.insert(DEFINES, "HW_UART_REPL=" .. HW_UART_REPL)
if RTE_UART0 == 1 then table.insert(DEFINES, "RTE_UART0=" .. RTE_UART0) end
if RTE_UART2 == 1 then table.insert(DEFINES, "RTE_UART2=" .. RTE_UART2) end
if RTE_I2C0 == 1 then table.insert(DEFINES, "RTE_I2C0=" .. RTE_I2C0) end
if RTE_I2C1 == 1 then table.insert(DEFINES, "RTE_I2C1=" .. RTE_I2C1) end
if RTE_SPI0 == 1 then table.insert(DEFINES, "RTE_SPI0=" .. RTE_SPI0) end
if RTE_SPI1 == 1 then table.insert(DEFINES, "RTE_SPI1=" .. RTE_SPI1) end
if get_config("06 Main stub autorun") then table.insert(DEFINES, "MAINRUN") end
if get_config("07 Reset on SMS") then table.insert(DEFINES, "SMSRESET") end
if get_config("08 Acknowledge SMS on reset") then table.insert(DEFINES, "SMSRESETACK") end
if get_config("09 Configiration by SMS") then table.insert(DEFINES, "SMSCONFIG") end
if get_config("10 Dump and halt on core exception") then table.insert(DEFINES, "HALTONEXC") end
if get_config("11 Include DHT module") then table.insert(DEFINES, "CONFIG_DHT_MODULE") end
if get_config("12 Include CC1101 module") then table.insert(DEFINES, "CONFIG_CC1101_MODULE") end

if get_config("1 Use FOTA routines") then table.insert(DEFINES, "FOTA_USE") end
if get_config("2 FOTA URL") then 
    FOTA_URL = get_config("2 FOTA URL") 
    table.insert(DEFINES, "FOTA_URL=\"" .. string.gsub(string.gsub(FOTA_URL, "OLDVERSION", "%%s"), "NEWVERSION", "%%s") .. "\"")
end
if get_config("3 Remove python files on upgrade") then table.insert(DEFINES, "FOTA_REMOVE_PY") end


if get_config("1 RS485_UART1_USE") then table.insert(DEFINES, "RS485_UART1_USE") end
if get_config("2 RS485_UART1_PIN")       then table.insert(DEFINES, "RS485_UART1_PIN="       .. get_config("2 RS485_UART1_PIN")) end
if get_config("3 RS485_UART1_DELAY")     then table.insert(DEFINES, "RS485_UART1_DELAY="     .. get_config("3 RS485_UART1_DELAY")) end
if get_config("4 RS485_UART1_PIN_LEVEL") then table.insert(DEFINES, "RS485_UART1_PIN_LEVEL=" .. get_config("4 RS485_UART1_PIN_LEVEL")) end
if get_config("5 RS485_UART1_WAIT_TX")   then table.insert(DEFINES, "RS485_UART1_WAIT_TX") end

if get_config("1 RS485_UART2_USE") then table.insert(DEFINES, "RS485_UART2_USE") end
if get_config("2 RS485_UART2_PIN")       then table.insert(DEFINES, "RS485_UART2_PIN="       .. get_config("2 RS485_UART2_PIN")) end
if get_config("3 RS485_UART2_DELAY")     then table.insert(DEFINES, "RS485_UART2_DELAY="     .. get_config("3 RS485_UART2_DELAY")) end
if get_config("4 RS485_UART2_PIN_LEVEL") then table.insert(DEFINES, "RS485_UART2_PIN_LEVEL=" .. get_config("4 RS485_UART2_PIN_LEVEL")) end
if get_config("5 RS485_UART2_WAIT_TX")   then table.insert(DEFINES, "RS485_UART2_WAIT_TX") end

if get_config("1 Deploy binpkg") ~= nil then DEPLOY_BINPKG_FOLDER = get_config("1 Deploy binpkg") end
if get_config("2 Deploy binpkg") ~= nil then DEPLOY_BINPKG_COMPRESS = get_config("2 Deploy binpkg") end
if get_config("3 Deploy pack") ~= nil then DEPLOY_PACK_FOLDER = get_config("3 Deploy pack") end
if get_config("4 Deploy pack") ~= nil then DEPLOY_PACK_COMPRESS = get_config("4 Deploy pack") end

--------------------------------------------------------
--------------- MICROPYTHON PART START -----------------
--------------------------------------------------------
BOARD_DIR = "boards/" .. BOARD  
DEBUG_LUA = 0

-- include ../../py/mkenv.mk
------------------------------------------
local TOP =  "../.."
local BUILD = "build"
local PYTHON = "python"
local CAT = "cat"
local LIB_NAME = "lib" .. USER_PROJECT_NAME .. ".a "

PY_SRC = TOP .. "/py"
MAKE_MANIFEST = PYTHON .. " " .. TOP .. "/tools/makemanifest.py"
MAKE_FROZEN = PYTHON .. " " .. TOP .. "/tools/make-frozen.py"
MPY_TOOL = PYTHON .. " " .. TOP .. "/tools/mpy-tool.py"
MPY_LIB_SUBMODULE_DIR = TOP .. "/lib/micropython-lib"
MPY_LIB_DIR = MPY_LIB_SUBMODULE_DIR     
MPY_TOOL_FLAGS = ""

MICROPY_MPYCROSS = TOP .. "/mpy-cross/build/mpy-cross"
MICROPY_MPYCROSS_DEPENDENCY = MICROPY_MPYCROSS

MPY_CROSS_FLAGS = "" -- port not supporting dynruntime

-- # Optional
-- include $(BOARD_DIR)/mpconfigboard.mk
-- add content from $(BOARD_DIR)/mpconfigboard.mk here

-- # qstr definitions (must come before including py.mk)
QSTR_DEFS = "qstrdefsport.h" -- BUILD .. "/pins_qstr.h"
QSTR_GLOBAL_DEPENDENCIES = BOARD_DIR .. "/mpconfigboard.h"
FROZEN_MANIFEST = ""
if os.isfile("boards/manifest.py") then FROZEN_MANIFEST="boards/manifest.py" end

-- # include py core make definitions
-- include $(TOP)/py/py.mk
----------------------------------------
--            py.mk content
----------------------------------------
-- # where py object files go (they have a name prefix to prevent filename clashes)
PY_BUILD = BUILD .. "/py"

-- # where autogenerated header files go
HEADER_BUILD = BUILD .. "/genhdr"

-- # file containing qstr defs for the core Python bit
PY_QSTR_DEFS = PY_SRC .. "/qstrdefs.h"

-- # If qstr autogeneration is not disabled we specify the output header
-- # for all collected qstrings.
if QSTR_AUTOGEN_DISABLE ~= 1 then
    QSTR_DEFS_COLLECTED = HEADER_BUILD .. "/qstrdefs.collected.h"
end

-- # Any files listed by these variables will cause a full regeneration of qstrs
-- # DEPENDENCIES: included in qstr processing; REQUIREMENTS: not included
QSTR_GLOBAL_DEPENDENCIES = QSTR_GLOBAL_DEPENDENCIES .. " " .. PY_SRC .. "/mpconfig.h mpconfigport.h"
QSTR_GLOBAL_REQUIREMENTS = HEADER_BUILD .. "/mpversion.h"

-- # some code is performance bottleneck and compiled with other optimization options
CSUPEROPT = "-O3"
-- skip MICROPY_FORCE_32BIT
-- skip USER_C_MODULES

PY_CORE_O_BASENAME = {"mpstate.o", "nlr.o", "nlrx86.o", "nlrx64.o", "nlrthumb.o", "nlraarch64.o", "nlrmips.o", "nlrpowerpc.o", "nlrxtensa.o", 
    "nlrsetjmp.o", "malloc.o", "gc.o", "pystack.o", "qstr.o", "vstr.o", "mpprint.o", "unicode.o", "mpz.o", "reader.o", "lexer.o", "parse.o", 
    "scope.o", "compile.o", "emitcommon.o", "emitbc.o", "asmbase.o", "asmx64.o", "emitnx64.o", "asmx86.o", "emitnx86.o", "asmthumb.o", 
    "emitnthumb.o", "emitinlinethumb.o", "asmarm.o", "emitnarm.o", "asmxtensa.o", "emitnxtensa.o", "emitinlinextensa.o", "emitnxtensawin.o", 
    "formatfloat.o", "parsenumbase.o", "parsenum.o", "emitglue.o", "persistentcode.o", "runtime.o", "runtime_utils.o", "scheduler.o", 
    "nativeglue.o", "pairheap.o", "ringbuf.o", "stackctrl.o", "argcheck.o", "warning.o", "profile.o", "map.o", "obj.o", "objarray.o", 
    "objattrtuple.o", "objbool.o", "objboundmeth.o", "objcell.o", "objclosure.o", "objcomplex.o", "objdeque.o", "objdict.o", "objenumerate.o", 
    "objexcept.o", "objfilter.o", "objfloat.o", "objfun.o", "objgenerator.o", "objgetitemiter.o", "objint.o", "objint_longlong.o", "objint_mpz.o", 
    "objlist.o", "objmap.o", "objmodule.o", "objobject.o", "objpolyiter.o", "objproperty.o", "objnone.o", "objnamedtuple.o", "objrange.o", 
    "objreversed.o", "objset.o", "objsingleton.o", "objslice.o", "objstr.o", "objstrunicode.o", "objstringio.o", "objtuple.o", "objtype.o", 
    "objzip.o", "opmethods.o", "sequence.o", "stream.o", "binary.o", "builtinimport.o", "builtinevex.o", "builtinhelp.o", "modarray.o", 
    "modbuiltins.o", "modcollections.o", "modgc.o", "modio.o", "modmath.o", "modcmath.o", "modmicropython.o", "modstruct.o", "modsys.o", 
    "moderrno.o", "modthread.o", "vm.o", "bc.o", "showbc.o", "repl.o", "smallint.o", "frozenmod.o", 
    -- v1.26 addin
    "cstack.o", "builtinevex.o", "objringio.o", "objcode.o"
    }

for _, name in ipairs(PY_CORE_O_BASENAME) do PY_CORE_O_BASENAME[_] = "py/" .. name end

-- # Sources that may contain qstrings
SRC_QSTR_IGNORE = "py/nlr"
--SRC_QSTR += $(filter-out $(SRC_QSTR_IGNORE),$(PY_CORE_O_BASENAME:.o=.c))
SRC_QSTR = ""
for _, name in ipairs(PY_CORE_O_BASENAME) do
    if name:sub(1, #SRC_QSTR_IGNORE) ~= SRC_QSTR_IGNORE then 
        SRC_QSTR = SRC_QSTR .. " " .. TOP .. "/" .. string.gsub(name .. " ", ".o ", ".c ")
    end
end

-- # mpconfigport.mk is optional, but changes to it may drastically change
-- # overall config, so they need to be caught
-- MPCONFIGPORT_MK = $(wildcard mpconfigport.mk)
-- feature disabled for now

-- include $(TOP)/extmod/extmod.mk
----------------------------------------
--           extmod.mk content
----------------------------------------
-- # This makefile fragment adds the source code files for the core extmod modules
-- # and provides rules to build 3rd-party components for extmod modules.

SRC_EXTMOD_C = { "extmod/machine_adc.c", "extmod/machine_adc_block.c", "extmod/machine_bitstream.c", "extmod/machine_i2c.c", 
                "extmod/machine_i2s.c", "extmod/machine_mem.c", "extmod/machine_pinbase.c", "extmod/machine_pulse.c", 
                "extmod/machine_pwm.c", "extmod/machine_signal.c", "extmod/machine_spi.c", "extmod/machine_timer.c", 
                "extmod/machine_uart.c", "extmod/machine_wdt.c", "extmod/modasyncio.c", "extmod/modbinascii.c", 
                "extmod/modbluetooth.c", "extmod/modbtree.c", "extmod/modcryptolib.c", "extmod/moddeflate.c", 
                "extmod/modframebuf.c", "extmod/modhashlib.c", "extmod/modheapq.c", "extmod/modjson.c", 
                "extmod/modmachine.c", "extmod/modnetwork.c", "extmod/modonewire.c", "extmod/modos.c", 
                "extmod/modplatform.c", "extmod/modrandom.c", "extmod/modre.c", "extmod/modselect.c", 
                "extmod/modsocket.c", "extmod/modtime.c", "extmod/moductypes.c", 
                "extmod/modtls_mbedtls.c",
                "extmod/modwebrepl.c", "extmod/modwebsocket.c", "extmod/network_cyw43.c", "extmod/network_esp_hosted.c", 
                "extmod/network_ninaw10.c", "extmod/network_wiznet5k.c", "extmod/os_dupterm.c", "extmod/vfs.c", 
                "extmod/vfs_blockdev.c", "extmod/vfs_fat.c", "extmod/vfs_fat_diskio.c", "extmod/vfs_fat_file.c", 
                "extmod/vfs_lfs.c", "extmod/vfs_posix.c", "extmod/vfs_posix_file.c", "extmod/vfs_reader.c", 
                "extmod/virtpin.c", "shared/libc/abort_.c", "shared/libc/printf.c" }

SRC_THIRDPARTY_C = {  }

-- PY_O += $(addprefix $(BUILD)/, $(SRC_EXTMOD_C:.c=.o))
for _, name in ipairs(SRC_EXTMOD_C) do SRC_EXTMOD_C[_] = TOP .. "/" .. name end
SRC_QSTR = SRC_QSTR .. " " .. table.concat(SRC_EXTMOD_C, " ")


-- ################################################################################
-- # libm/libm_dbl math library

-- # Single-precision math library.
SRC_LIB_LIBM_C = {  "acoshf.c", "asinfacosf.c", "asinhf.c", "atan2f.c", "atanf.c", "atanhf.c", "ef_rem_pio2.c", "erf_lgamma.c", 
                    "fmodf.c", "kf_cos.c", "kf_rem_pio2.c", "kf_sin.c", "kf_tan.c", "log1pf.c", "math.c", "nearbyintf.c", 
                    "roundf.c", "sf_cos.c", "sf_erf.c", "sf_frexp.c", "sf_ldexp.c", "sf_modf.c", "sf_sin.c", "sf_tan.c", 
                    "wf_lgamma.c", "wf_tgamma.c" }

-- # Choose only one of these sqrt implementations, software or hardware.
SRC_LIB_LIBM_SQRT_SW_C = "lib/libm/ef_sqrt.c"
SRC_LIB_LIBM_SQRT_HW_C = "lib/libm/thumb_vfp_sqrtf.c"

-- # Double-precision math library.
SRC_LIB_LIBM_DBL_C = {  "__cos.c", "__expo2.c", "__fpclassify.c", "__rem_pio2.c", "__rem_pio2_large.c", "__signbit.c", "__sin.c", 
                        "__tan.c", "acos.c", "acosh.c", "asin.c", "asinh.c", "atan.c", "atan2.c", "atanh.c", "ceil.c", "cos.c", 
                        "cosh.c", "copysign.c", "erf.c", "exp.c", "expm1.c", "floor.c", "fmod.c", "frexp.c", "ldexp.c", "lgamma.c", 
                        "log.c", "log10.c", "log1p.c", "modf.c", "nearbyint.c", "pow.c", "rint.c", "round.c", "scalbn.c", "sin.c", 
                        "sinh.c", "tan.c", "tanh.c", "tgamma.c", "trunc.c" }

-- # Choose only one of these sqrt implementations, software or hardware.
SRC_LIB_LIBM_DBL_SQRT_SW_C = "lib/libm_dbl/sqrt.c"
SRC_LIB_LIBM_DBL_SQRT_HW_C = "lib/libm_dbl/thumb_vfp_sqrt.c"

-- # Too many warnings in libm_dbl, disable for now.
-- $(BUILD)/lib/libm_dbl/%.o: CFLAGS += -Wno-double-promotion -Wno-float-conversion -- TODO add to target

---------------------------------------------------------------------------------
-- Skip VFS, VFA FAT, btree, blootooth, lwip, mbedtls, networking from extmod.mk
---------------------------------------------------------------------------------
CFLAGS_EXTMOD = ""
CFLAGS_THIRDPARTY = ""

-- ################################################################################
-- # ussl

-- if MICROPY_PY_SSL == 1 then 
--    CFLAGS_EXTMOD = CFLAGS_EXTMOD .. "-DMICROPY_PY_SSL=1"
-- end

-- ################################################################################
-- # btree (Luat implementation has its own KV database, TODO:

-- PY_O += $(addprefix $(BUILD)/, $(SRC_THIRDPARTY_C:.c=.o))
for _, name in ipairs(SRC_THIRDPARTY_C) do SRC_THIRDPARTY_C[_] = SDK_TOP .. "/" .. name end


----------------------------------------
--           Makefile content
----------------------------------------
CFLAGS = " -I . -I ../../ -I ./" .. BUILD .. "/"
CFLAGS = CFLAGS .. " -nostdlib -std=gnu99"
CFLAGS = CFLAGS .. " -I" .. table.concat(INCLUDES, " -I")
if os.exists("./boards/" .. BOARD) then CFLAGS = CFLAGS .. " -I" .. "./boards/" .. BOARD end
CFLAGS = CFLAGS .. " -D" .. table.concat(DEFINES, " -D")
CXXFLAGS = ""

SRC_C = { "main.c", "gccollect.c", "mphalport.c", "modair.c", "help.c", "machine_pin.c", "modsocket.c", "modcellular.c", "httpclient.c", 
          "modgps.c", "moddht.c", "machine_rtc.c", "machine_hw_spi.c", "machine_i2c.c", "modcc1101.c" }

SHARED_SRC_C = {    "netutils/netutils.c", 
                    "timeutils/timeutils.c", 
                    "runtime/interrupt_char.c", 
                    "runtime/pyexec.c", 
                    "runtime/sys_stdio_mphal.c", 
                    "runtime/stdout_helpers.c",
                    "runtime/gchelper_native.c"
                }
for _, name in ipairs(SHARED_SRC_C) do SHARED_SRC_C[_] = TOP .. "/shared/" .. name end

SHARED_SRC_S =  { "runtime/gchelper_thumb2.s" }
for _, name in ipairs(SHARED_SRC_S) do SHARED_SRC_S[_] = TOP .. "/shared/" .. name end

LIB_SRC_C = { "shared/readline/readline.c" }
for _, name in ipairs(LIB_SRC_C) do LIB_SRC_C[_] = TOP .. "/" .. name end

SRC_S = { }
DRIVERS_SRC_C = { }

-- xmake build target logic differs from Makefile logic, so simplify "OBJ" to simple string
OBJ = "OBJ"

-- # List of sources for qstr extraction
SRC_QSTR = SRC_QSTR .. " " .. table.concat(SRC_C, " ") .. " ".. table.concat(SHARED_SRC_C, " ")

-- TODO refactor build logic
PROCESS_QSTR = not os.isfile("build/genhdr/qstrdefs.generated.h")

----------------------------------
-- include $(TOP)/py/mkrules.mk --
----------------------------------
HELP_BUILD_ERROR = "See \27[1;31mhttps://github.com/micropython/micropython/wiki/Build-Troubleshooting\27[0m"
HELP_MPY_LIB_SUBMODULE = "\27[1;31mError: micropython-lib submodule is not initialized.\27[0m Run 'make submodules'"

-- # Extra deps that need to happen before object compilation.
OBJ_EXTRA_ORDER_DEPS = ""

-- # Generate header files.
OBJ_EXTRA_ORDER_DEPS = OBJ_EXTRA_ORDER_DEPS .. " " .. HEADER_BUILD .. "/moduledefs.h " .. HEADER_BUILD .. "/root_pointers.h"

if MICROPY_ROM_TEXT_COMPRESSION == 1 then
    -- # If compression is enabled, trigger the build of compressed.data.h...
    OBJ_EXTRA_ORDER_DEPS = OBJ_EXTRA_ORDER_DEPS .. " " .. HEADER_BUILD .. "/compressed.data.h"
    -- # ...and enable the MP_COMPRESSED_ROM_TEXT macro (used by MP_ERROR_TEXT).
    CFLAGS = CFLAGS .. " -DMICROPY_ROM_TEXT_COMPRESSION=1"
end

-- # QSTR generation uses the same CFLAGS, with these modifications.
QSTR_GEN_FLAGS = "-DNO_QSTR"
-- # Note: := to force evaluation immediately.
QSTR_GEN_CFLAGS = CFLAGS:replace("\"", "\\\\\"")
QSTR_GEN_CFLAGS = QSTR_GEN_CFLAGS .. " " .. QSTR_GEN_FLAGS
QSTR_GEN_CXXFLAGS = CXXFLAGS
QSTR_GEN_CXXFLAGS = QSTR_GEN_CXXFLAGS .. " " .. QSTR_GEN_FLAGS


if FROZEN_MANIFEST ~= "" then 

    GIT_SUBMODULES = ""
    if MPY_LIB_DIR == MPY_LIB_SUBMODULE_DIR then 
        GIT_SUBMODULES = "lib/micropython-lib"
    end

    CFLAGS = CFLAGS .. " -DMICROPY_QSTR_EXTRA_POOL=mp_qstr_frozen_const_pool"
    CFLAGS = CFLAGS .. " -DMICROPY_MODULE_FROZEN_MPY"
    CFLAGS = CFLAGS .. " -DMICROPY_MODULE_FROZEN_STR"

    -- # Set default path variables to be passed to makemanifest.py. These will be
    -- # available in path substitutions. Additional variables can be set per-board
    -- # in mpconfigboard.mk or on the make command line.
    MICROPY_MANIFEST_MPY_LIB_DIR = MPY_LIB_DIR
    MICROPY_MANIFEST_PORT_DIR = os.curdir():replace("\\", "/")
    MICROPY_MANIFEST_BOARD_DIR = BOARD_DIR
    MICROPY_MANIFEST_MPY_DIR = TOP

    -- # Find all MICROPY_MANIFEST_* variables and turn them into command line arguments.
    -- $(foreach var,$(filter MICROPY_MANIFEST_%, $(.VARIABLES)),-v "$(subst MICROPY_MANIFEST_,,$(var))=$($(var))")

    MANIFEST_VARIABLES = ""
    MANIFEST_VARIABLES = MANIFEST_VARIABLES .. " -v \"MPY_LIB_DIR=" .. MICROPY_MANIFEST_MPY_LIB_DIR .. "\""
    MANIFEST_VARIABLES = MANIFEST_VARIABLES .. " -v \"PORT_DIR="    .. MICROPY_MANIFEST_PORT_DIR    .. "\""
    MANIFEST_VARIABLES = MANIFEST_VARIABLES .. " -v \"BOARD_DIR="   .. MICROPY_MANIFEST_BOARD_DIR   .. "\""
    MANIFEST_VARIABLES = MANIFEST_VARIABLES .. " -v \"MPY_DIR="     .. MICROPY_MANIFEST_MPY_DIR     .. "\""
    MANIFEST_VARIABLES = MANIFEST_VARIABLES .. " "
end

CFLAGS = CFLAGS .. " " .. CFLAGS_EXTMOD .. " " .. CFLAGS_THIRDPARTY
-- LDFLAGS = LDFLAGS .. "  " .. LDFLAGS_EXTMOD .. " " .. LDFLAGS_THIRDPARTY

--------------------------------------------------------
----------------- MICROPYTHON PART END -----------------
--------------------------------------------------------

--------------------------------------------------------
--                  EC618 driver target
--------------------------------------------------------
target("driver")
    set_kind("static")
    set_objectdir(BUILD)
    set_targetdir(BUILD)
    add_includedirs("./",{public = true})

    add_files(SDK_TOP .. "/PLAT/driver/board/ec618_0h00/src/**.c",
                SDK_TOP .. "/PLAT/driver/chip/ec618/ap/**.c",
                SDK_TOP .. "/PLAT/driver/chip/ec618/common/gcc/memcpy-armv7m.S",
                SDK_TOP .. "/PLAT/driver/hal/**.c",
                SDK_TOP .. "/PLAT/core/speed/*.c"
                -- SDK_TOP .. "/thirdparty/miniz/miniz.c"
    )

    remove_files(SDK_TOP .. "/PLAT/driver/board/ec618_0h00/src/camera/camAT.c",
                SDK_TOP .. "/PLAT/driver/board/ec618_0h00/src/exstorage/*.c",
                SDK_TOP.."/PLAT/driver/chip/ec618/ap/src/usb/usb_device/usb_bl_test.c",
                SDK_TOP.."/PLAT/driver/chip/ec618/ap/src_cmsis/bsp_lpusart_stub.c",
                SDK_TOP.."/PLAT/driver/chip/ec618/ap/src/tls.c",
                SDK_TOP.."/PLAT/driver/chip/ec618/ap/src_cmsis/bsp_spi.c" 
    )
target_end()
--------------------------------------------------------
--                  mpy-cross target
--------------------------------------------------------
target(MICROPY_MPYCROSS_DEPENDENCY)
    set_enabled(FROZEN_MANIFEST ~= "")
    set_policy("build.across_targets_in_parallel", false)    
    set_kind("object")
    set_objectdir(BUILD)
    add_values("MPC", MICROPY_MPYCROSS_DEPENDENCY)

    on_build(function (target)
        MPC = target:values("MPC")
        import("lib.detect.find_tool")      
        local mpy_cross = find_tool(MPC)

        if mpy_cross == nil then
            local make = find_tool("make")
            if make and make.program then
               print(path.directory(path.directory(MPC)))
               os.exec("make -C " .. path.directory(path.directory(MPC)))
            end
        end
    end)
target_end()
--------------------------------------------------------
--                         py core target
--------------------------------------------------------
target("py")    
    set_policy("build.across_targets_in_parallel", false)
    set_policy("build.intermediate_directory", false)
    set_kind("object") -- important!
    set_objectdir(BUILD)
    add_deps(OBJ)

    for _, name in ipairs(PY_CORE_O_BASENAME) do 
        add_files(TOP .. "/" .. (string.gsub(name .. " ", ".o ", ".c ")):rtrim(), { cflags = CFLAGS })
    end
target_end()
--------------------------------------------------------
--                         extmod target
--------------------------------------------------------
target("extmod")    
    set_policy("build.across_targets_in_parallel", false)
    set_policy("build.intermediate_directory", false)
    set_kind("object") -- important!
    set_objectdir(BUILD)
    add_deps(OBJ)

    for _, name in ipairs(SRC_EXTMOD_C) do 
        add_files(name, { cflags = CFLAGS .. " -Wno-clobbered -Wno-stack-usage "}) 
    end
target_end()

--------------------------------------------------------
--                    the port target
--------------------------------------------------------
target(USER_PROJECT_NAME)
    set_policy("build.intermediate_directory", false)
    set_policy("build.across_targets_in_parallel", false)

    set_kind("static")
    set_targetdir(BUILD)    

    add_includedirs(INCLUDES, { public = true })
    add_deps("py")
    add_deps("extmod")    

    ----------------------- thirdparty -----------------------
    add_includedirs("./",{public = true})
    add_files(SDK_TOP .. "/interface/private_src/*.c",{public = true})
    add_files(SDK_TOP .. "/thirdparty/mbedtls/library/*.c",{public = true})     
    add_files(SDK_TOP .. "/thirdparty/printf/*.c",{public = true})
    add_files(SDK_TOP .. "/thirdparty/iconv/*.c",{public = true})
    add_files(SDK_TOP .. "/thirdparty/fal/src/*.c",{public = true})
    add_files(SDK_TOP .. "/thirdparty/flashdb/src/*.c",{public = true})
    add_files(SDK_TOP .. "/interface/src/*.c|luat_sms_ec618.c",{public = true}) -- exclude luat_sms_ec618.c
    add_files(SDK_TOP .. "/thirdparty/littlefs/**.c",{public = true})
    add_files(SDK_TOP .. "/thirdparty/cJSON/**.c",{public = true})
    add_files(SDK_TOP .. "/luatos_lwip_socket/src/**.c",{public = true, cflags = CFLAGS .. " -Wno-override-init"})
    add_files(SDK_TOP .. "/thirdparty/minmea/minmea.c",{public = true})

    -- remove_files(SDK_TOP .. "/interface/private_src/luat_full_ota_ec618.c")  -- rewritten to use MPY mbedtls version

target_end()
--------------------------------------------------------
--                 mkrules.mk targets
--------------------------------------------------------
-- # The following rule uses | to create an order only prerequisite. Order only
-- # prerequisites only get built if they don't exist. They don't cause timestamp
-- # checking to be performed.
-- #
-- # We don't know which source files actually need the generated.h (since
-- # it is #included from str.h). The compiler generated dependencies will cause
-- # the right .o's to get recompiled if the generated.h file changes. Adding
-- # an order-only dependency to all of the .o's will cause the generated .h
-- # to get built before we try to compile any of them.
target(OBJ) 
    set_policy("build.across_targets_in_parallel", false)

    set_kind("object") -- important!
    -- $(OBJ): | $(HEADER_BUILD)/qstrdefs.generated.h $(HEADER_BUILD)/mpversion.h $(OBJ_EXTRA_ORDER_DEPS)
    if PROCESS_QSTR then
        add_deps(HEADER_BUILD .. "/qstrdefs.generated.h")
        add_deps(HEADER_BUILD .. "/mpversion.h")
    end
    if FROZEN_MANIFEST ~= "" then 
        add_deps(BUILD .. "/frozen_content.c")
        add_files(BUILD .. "/frozen_content.c", { public = true, cflags = CFLAGS })
    end

    for _, tdep in ipairs(OBJ_EXTRA_ORDER_DEPS:split(" ")) do
        if DEBUG_LUA == 1 then print("Add dep: ", tdep) end
        if PROCESS_QSTR then add_deps(tdep) end
    end

    on_load(function (target)
        -- frozen_content.c should be included in prebuild stage, so the file should exist here
        -- it replaces by generated content in target(BUILD .. "/frozen_content.c")
        os.mkdir(BUILD)
        if not os.isfile(BUILD .. "/frozen_content.c") then os.touch(BUILD .. "/frozen_content.c") end
    end)
target_end()

-- # The logic for qstr regeneration (applied by makeqstrdefs.py) is:
-- # - if anything in QSTR_GLOBAL_DEPENDENCIES is newer, then process all source files ($^)
-- # - else, if list of newer prerequisites ($?) is not empty, then process just these ($?)
-- # - else, process all source files ($^) [this covers "make -B" which can set $? to empty]
-- # See more information about this process in docs/develop/qstr.rst.

target(HEADER_BUILD .. "/qstr.i.last")
    set_enabled(PROCESS_QSTR)
    -- $(HEADER_BUILD)/qstr.i.last: $(SRC_QSTR) $(QSTR_GLOBAL_DEPENDENCIES) | $(QSTR_GLOBAL_REQUIREMENTS)
    -- add_deps(SRC_QSTR)   
    -- add_deps(QSTR_GLOBAL_DEPENDENCIES)
    add_deps(QSTR_GLOBAL_REQUIREMENTS)  

    -- $(Q)$(PYTHON) $(PY_SRC)/makeqstrdefs.py pp $(CPP) output $(HEADER_BUILD)/qstr.i.last cflags $(QSTR_GEN_CFLAGS) cxxflags $(QSTR_GEN_CXXFLAGS) sources $^ dependencies $(QSTR_GLOBAL_DEPENDENCIES) changed_sources $?
    set_values("execp1", "sh -c '" .. PYTHON .. " " .. PY_SRC .. "/makeqstrdefs.py pp ")
    set_values("execp2", " output " .. HEADER_BUILD .. "/qstr.i.last " .. " cflags " .. QSTR_GEN_CFLAGS .. " cxxflags " .. QSTR_GEN_CXXFLAGS .. " sources " .. SRC_QSTR .. " dependencies " .. QSTR_GLOBAL_DEPENDENCIES .. " changed_sources " .. SRC_QSTR .. "'")

    on_build(function (target)
        print("GEN ", target:name())
        GCC_DIR = target:toolchains()[1]:sdkdir().."/"
        CPP = GCC_DIR .. "bin/arm-none-eabi-gcc -E "
        cmd = target:values("execp1") .. CPP .. target:values("execp2")
        os.exec(cmd)
    end)
target_end()

target(HEADER_BUILD .. "/qstr.split") 
    set_enabled(PROCESS_QSTR)
    -- $(HEADER_BUILD)/qstr.split: $(HEADER_BUILD)/qstr.i.last
    add_deps(HEADER_BUILD .. "/qstr.i.last")

    -- $(Q)$(PYTHON) $(PY_SRC)/makeqstrdefs.py split qstr $< $(HEADER_BUILD)/qstr _
    -- $(Q)$(TOUCH) $@
    set_values("exec", "sh -c '" .. PYTHON .. " " .. PY_SRC .. "/makeqstrdefs.py split qstr " .. HEADER_BUILD .. "/qstr.i.last " .. HEADER_BUILD .. "/qstr " .. HEADER_BUILD .. "/qstr.i.last")

    on_build(function (target)
        print("GEN ", target:name())
        os.exec(target:values("exec") .. "'")
        os.touch(target:name())
    end)
target_end()

target(QSTR_DEFS_COLLECTED) 
    set_enabled(PROCESS_QSTR)
    -- $(QSTR_DEFS_COLLECTED): $(HEADER_BUILD)/qstr.split
    add_deps(HEADER_BUILD .. "/qstr.split")

    -- $(Q)$(PYTHON) $(PY_SRC)/makeqstrdefs.py cat qstr _ $(HEADER_BUILD)/qstr $@
    set_values("exec", "sh -c '" .. PYTHON .. " " .. PY_SRC .. "/makeqstrdefs.py cat qstr " .. HEADER_BUILD .. "/qstr.split " .. HEADER_BUILD .. "/qstr ")

    on_build(function (target)
        print("GEN ", target:name())
        os.exec(target:values("exec") .. target:name() .. "'")
    end)
target_end()

-- # Module definitions via MP_REGISTER_MODULE.
target(HEADER_BUILD .. "/moduledefs.split") 
    set_enabled(PROCESS_QSTR)
    -- $(HEADER_BUILD)/moduledefs.split: $(HEADER_BUILD)/qstr.i.last
    add_deps(HEADER_BUILD .. "/qstr.i.last")

    -- $(Q)$(PYTHON) $(PY_SRC)/makeqstrdefs.py split module $< $(HEADER_BUILD)/module _
    -- $(Q)$(TOUCH) $@
    set_values("exec", "sh -c '" .. PYTHON .. " " .. PY_SRC .. "/makeqstrdefs.py split module " .. HEADER_BUILD .. "/qstr.i.last " .. HEADER_BUILD .. "/module " .. HEADER_BUILD .. "/qstr.i.last")

    on_build(function (target)
        print("GEN ", target:name())
        os.exec(target:values("exec") .. "'")
        os.touch(target:name())
    end)
target_end()

target(HEADER_BUILD .. "/moduledefs.collected") 
    set_enabled(PROCESS_QSTR)
    -- $(HEADER_BUILD)/moduledefs.collected: $(HEADER_BUILD)/moduledefs.split
    add_deps(HEADER_BUILD .. "/moduledefs.split")

    -- $(Q)$(PYTHON) $(PY_SRC)/makeqstrdefs.py cat module _ $(HEADER_BUILD)/module $@
    set_values("exec", "sh -c '" .. PYTHON .. " " .. PY_SRC .. "/makeqstrdefs.py cat module " .. HEADER_BUILD .. "/moduledefs.split " .. HEADER_BUILD .. "/module ")

    on_build(function (target)
        print("GEN ", target:name())
        os.exec(target:values("exec") .. target:name() .. "'")
    end)
target_end()

-- # Module definitions via MP_REGISTER_ROOT_POINTER.
target(HEADER_BUILD .. "/root_pointers.split") 
    set_enabled(PROCESS_QSTR)
    -- $(HEADER_BUILD)/root_pointers.split: $(HEADER_BUILD)/qstr.i.last
    add_deps(HEADER_BUILD .. "/qstr.i.last")

    -- $(Q)$(PYTHON) $(PY_SRC)/makeqstrdefs.py split root_pointer $< $(HEADER_BUILD)/root_pointer _
    -- $(Q)$(TOUCH) $@
    set_values("exec", "sh -c '" .. PYTHON .. " " .. PY_SRC .. "/makeqstrdefs.py split root_pointer " .. HEADER_BUILD .. "/qstr.i.last " .. HEADER_BUILD .. "/root_pointer " .. HEADER_BUILD .. "/qstr.i.last")

    on_build(function (target)
        print("GEN ", target:name())
        os.exec(target:values("exec") .. "'")
        os.touch(target:name())
    end)
target_end()

target(HEADER_BUILD .. "/root_pointers.collected") 
    set_enabled(PROCESS_QSTR)
    -- $(HEADER_BUILD)/root_pointers.collected: $(HEADER_BUILD)/root_pointers.split
    add_deps(HEADER_BUILD .. "/root_pointers.split")

    -- $(Q)$(PYTHON) $(PY_SRC)/makeqstrdefs.py cat root_pointer _ $(HEADER_BUILD)/root_pointer $@
    set_values("exec", "sh -c '" .. PYTHON .. " " .. PY_SRC .. "/makeqstrdefs.py cat root_pointer " .. HEADER_BUILD .. "/root_pointers.split " .. HEADER_BUILD .. "/root_pointer ")

    on_build(function (target)
        print("GEN ", target:name())
        os.exec(target:values("exec") .. target:name() .. "'")
    end)
target_end()

-- # Compressed error strings.
target(HEADER_BUILD .. "/compressed.split") 
    set_enabled(PROCESS_QSTR)
    if MICROPY_ROM_TEXT_COMPRESSION ~= 1 then
        set_enabled(false)
    end
    -- $(HEADER_BUILD)/compressed.split: $(HEADER_BUILD)/qstr.i.last
    add_deps(HEADER_BUILD .. "/qstr.i.last")

    -- $(Q)$(PYTHON) $(PY_SRC)/makeqstrdefs.py split compress $< $(HEADER_BUILD)/compress _
    -- $(Q)$(TOUCH) $@
    set_values("exec", "sh -c '" .. PYTHON .. " " .. PY_SRC .. "/makeqstrdefs.py split compress " .. HEADER_BUILD .. "/qstr.i.last " .. HEADER_BUILD .. "/compress " .. HEADER_BUILD .. "/qstr.i.last")

    on_build(function (target)
        print("GEN ", target:name())
        os.exec(target:values("exec") .. "'")
        os.touch(target:name())
    end)
target_end()

target(HEADER_BUILD .. "/compressed.collected") 
    set_enabled(PROCESS_QSTR)
    if MICROPY_ROM_TEXT_COMPRESSION ~= 1 then
        set_enabled(false)
    end
    -- $(HEADER_BUILD)/compressed.collected: $(HEADER_BUILD)/compressed.split
    add_deps(HEADER_BUILD .. "/compressed.split")

    -- $(Q)$(PYTHON) $(PY_SRC)/makeqstrdefs.py cat compress _ $(HEADER_BUILD)/compress $@
    set_values("exec", "sh -c '" .. PYTHON .. " " .. PY_SRC .. "/makeqstrdefs.py cat compress " .. HEADER_BUILD .. "/compressed.split "  .. HEADER_BUILD .. "/compress ")

    on_build(function (target)
        print("GEN ", target:name())
        os.exec(target:values("exec") .. target:name() .. "'")
    end)
target_end()


target(HEADER_BUILD)
    set_values("HEADER_BUILD", HEADER_BUILD)
    on_build(function (target)
        BUILD = target:values("BUILD")
        HEADER_BUILD = target:values("HEADER_BUILD")        
        if not os.exists(HEADER_BUILD) then os.mkdir(HEADER_BUILD) end
    end)
target_end()


target(BUILD .. "/frozen_content.c")    
    set_enabled(FROZEN_MANIFEST ~= "")  
    set_policy("build.across_targets_in_parallel", false)   
    set_policy("build.intermediate_directory", false)
    set_kind("object")
    set_objectdir(BUILD)

    -- $(BUILD)/frozen_content.c: FORCE $(BUILD)/genhdr/qstrdefs.generated.h $(BUILD)/genhdr/root_pointers.h | $(MICROPY_MPYCROSS_DEPENDENCY)
    if PROCESS_QSTR then
        add_deps(HEADER_BUILD .. "/qstrdefs.generated.h")
        add_deps(HEADER_BUILD .. "/root_pointers.h")        
    end
    -- if FROZEN_MANIFEST ~= "" then add_deps(MICROPY_MPYCROSS_DEPENDENCY) end
    add_deps(MICROPY_MPYCROSS_DEPENDENCY)

    -- $(Q)test -e "$(MPY_LIB_DIR)/README.md" || (echo -e $(HELP_MPY_LIB_SUBMODULE); false)
    -- $(Q)$(MAKE_MANIFEST) -o $@ $(MANIFEST_VARIABLES) -b "$(BUILD)" $(if $(MPY_CROSS_FLAGS),-f"$(MPY_CROSS_FLAGS)",) --mpy-tool-flags="$(MPY_TOOL_FLAGS)" $(FROZEN_MANIFEST)
    set_values("test", MPY_LIB_DIR .. "/README.md")
    set_values("err", HELP_MPY_LIB_SUBMODULE)
    set_values("execp1", "sh -c '" .. MAKE_MANIFEST .. " -o ")

    cmd = " " .. MANIFEST_VARIABLES .. " -b \"" .. BUILD .. "\" "
    if MPY_CROSS_FLAGS ~= "" then
        cmd = cmd .. "-f\"" .. MPY_CROSS_FLAGS .. "\""
    end
    cmd = cmd .. "--mpy-tool-flags=\"" .. MPY_TOOL_FLAGS .. "\" " .. FROZEN_MANIFEST .. "'"
    set_values("execp2", cmd)

    on_build(function (target)
        test = target:values("test")
        err = target:values("err")      
        if os.isfile(test) == false then
            os.raise(err)
        else
            os.rm(target:name()) -- frozen_content.c build workaround
            cmd = target:values("execp1") .. target:name() .. target:values("execp2")
            os.exec(cmd)            
        end
    end)
target_end()



task("submodules")
    set_menu { } -- this is a xmake "feature"

    on_run(function ()
        -- xmake can not set task parameters :(
        TOP = "../.."
        GIT_SUBMODULES = "lib/micropython-lib"
        print("Updating submodules: " .. GIT_SUBMODULES)

        -- $(Q)git submodule sync $(addprefix $(TOP)/,$(GIT_SUBMODULES))
        -- $(Q)git submodule update --init $(addprefix $(TOP)/,$(GIT_SUBMODULES))
        SUB_DUR = TOP .. "/" .. GIT_SUBMODULES
        if os.isdir(SUB_DUR) then
            try { function () 
                os.exec("git rm -r -f " .. SUB_DUR) 
            end }
        end
                
        cmd0 = "git submodule add -f https://github.com/micropython/micropython-lib.git " .. TOP .. "/" .. GIT_SUBMODULES
        cmd1 = "git submodule sync " .. TOP .. "/" .. GIT_SUBMODULES
        cmd2 = "git submodule update --init " .. TOP .. "/" .. GIT_SUBMODULES
        print(cmd1)
        os.exec(cmd0)
        os.exec(cmd1)
        os.exec(cmd2)
    end)
task_end()

--------------------------------------------------------
--                     py.mk targets
--------------------------------------------------------
target (HEADER_BUILD .. "/mpversion.h")
    -- TODO: add $@
    -- $(HEADER_BUILD)/mpversion.h: FORCE | $(HEADER_BUILD)
    add_deps(HEADER_BUILD)
    -- $(Q)$(PYTHON) $(PY_SRC)/makeversionhdr.py $@
    set_values("exec", PYTHON .. " " .. PY_SRC .. "/makeversionhdr.py" .. " " .. HEADER_BUILD .. "/mpversion.h")
    on_build(function (target)
        os.exec(target:values("exec"))
    end)
target_end()


target(HEADER_BUILD .. "/qstrdefs.generated.h") 
    set_enabled(PROCESS_QSTR)
    -- $(HEADER_BUILD)/qstrdefs.generated.h: $(PY_QSTR_DEFS) $(QSTR_DEFS) $(QSTR_DEFS_COLLECTED) $(PY_SRC)/makeqstrdata.py mpconfigport.h $(MPCONFIGPORT_MK) $(PY_SRC)/mpconfig.h | $(HEADER_BUILD)
    add_deps(HEADER_BUILD)
    -- add_deps(PY_QSTR_DEFS)
    -- add_deps(QSTR_DEFS)
    add_deps(QSTR_DEFS_COLLECTED)
    -- add_deps(PY_SRC .. "/makeqstrdata.py")
    -- add_deps("mpconfigport.h")
    add_deps(MPCONFIGPORT_MK)
    --add_deps(PY_SRC .. "/mpconfig.h")
                                                                               
    -- $(ECHO) "GEN $@"                                                        
    -- $(Q)$(CAT) $(PY_QSTR_DEFS) $(QSTR_DEFS) $(QSTR_DEFS_COLLECTED) | $(SED) 's/^Q(.*)/"&"/' | $(CPP) $(CFLAGS) - | $(SED) 's/^\"\(Q(.*)\)\"/\1/' > $(HEADER_BUILD)/qstrdefs.preprocessed.h
    -- $(Q)$(PYTHON) $(PY_SRC)/makeqstrdata.py $(HEADER_BUILD)/qstrdefs.preprocessed.h > $@    

    QSTR_CAT = PY_QSTR_DEFS .. " " .. QSTR_DEFS_COLLECTED 
    if QSTR_DEFS ~= ""  and os.exists(QSTR_DEFS) then QSTR_CAT = QSTR_CAT .. " " .. QSTR_DEFS end

    set_values("HEADER_BUILD", HEADER_BUILD)
    set_values("QSTR_CAT", QSTR_CAT)
    set_values("CFLAGS", CFLAGS)
    set_values("PYTHON", PYTHON)
    set_values("PY_SRC", PY_SRC)

    on_build(function (target)      
        print("GEN ", target:name())
        GCC_DIR = target:toolchains()[1]:sdkdir().."/"
        CPP = GCC_DIR .. "bin/arm-none-eabi-gcc -E "

        HEADER_BUILD = target:values("HEADER_BUILD")
        QSTR_CAT = target:values("QSTR_CAT")
        CFLAGS = target:values("CFLAGS")
        PYTHON = target:values("PYTHON")
        PY_SRC = target:values("PY_SRC")

        temp1 = HEADER_BUILD .. "/qstrdefs.temp1.h"
        temp2 = HEADER_BUILD .. "/qstrdefs.temp2.h"
        temp3 = HEADER_BUILD .. "/qstrdefs.preprocessed.h"

        data = ""
        for _,fname in ipairs(QSTR_CAT:split(" ")) do
            data = data .. io.readfile(fname)
        end

        local function starts_with(str, start) return str:sub(1, #start) == start end
        local function ends_with(str, ending) return ending == "" or str:sub(-#ending) == ending end

        lines = {}
        for s in data:gmatch("[^\r\n]+") do table.insert(lines, s) end
        for _,line in ipairs(lines) do
            if string.len(line) > 2 then
                if starts_with(line, "Q(") then 
                    lines[_] = "\"" .. lines[_]
                    if ends_with(line, ")") then lines[_] = lines[_] .. "\"" end
                end
            end
        end
        data = ""
        for _,line in ipairs(lines) do data = data .. line .. "\r\n" end
        io.writefile(temp1, data) 

        os.exec("sh -c \"" .. CPP .. CFLAGS .. " " .. temp1 .. " > " .. temp2 .. "\"")

        data = io.readfile(temp2)
        lines = {}
        for s in data:gmatch("[^\r\n]+") do table.insert(lines, s) end
        for _,line in ipairs(lines) do
            if string.len(line) > 2 then
                if starts_with(line, "\"Q(") then 
                    lines[_] = string.sub(lines[_], 2)
                    if ends_with(line, ")\"") then lines[_] = string.sub(lines[_], 1, -2) end
                end
            end
        end

        data = ""
        for _,line in ipairs(lines) do data = data .. line .. "\r\n" end
        io.writefile(temp3, data) 

        os.exec("sh -c \"" .. PYTHON .. " " .. PY_SRC .. "/makeqstrdata.py ".. temp3 .. " > " .. target:name() .. "\"")
    end)
target_end()

target(HEADER_BUILD .. "/compressed.data.h") 
    set_enabled(PROCESS_QSTR)
    if MICROPY_ROM_TEXT_COMPRESSION ~= 1 then
        set_enabled(false)
    end

    -- $(HEADER_BUILD)/compressed.data.h: $(HEADER_BUILD)/compressed.collected
    add_deps(HEADER_BUILD .. "/compressed.collected")
    
    -- $(Q)$(PYTHON) $(PY_SRC)/makecompresseddata.py $< > $@
    set_values("exec", "sh -c '" .. PYTHON .. " " .. PY_SRC .. "/makecompresseddata.py " .. HEADER_BUILD .. "/compressed.collected > ")

    on_build(function (target)
        print("GEN ", target:name())
        os.exec(target:values("exec") .. target:name() .. "'")
    end)
target_end()

-- # build a list of registered modules for py/objmodule.c.
target(HEADER_BUILD .. "/moduledefs.h") 
    set_enabled(PROCESS_QSTR)
    -- $(HEADER_BUILD)/moduledefs.h: $(HEADER_BUILD)/moduledefs.collected
    add_deps(HEADER_BUILD.. "/moduledefs.collected")

    -- $(Q)$(PYTHON) $(PY_SRC)/makemoduledefs.py $< > $@
    set_values("execp1", "sh -c '" .. PYTHON .. " " .. PY_SRC .. "/makemoduledefs.py " .. HEADER_BUILD.. "/moduledefs.collected > ")
    on_build(function (target)
        print("GEN ", target:name())
        os.exec(target:values("execp1") .. target:name() .. "'")
    end)
target_end()

-- # build a list of registered root pointers for py/mpstate.h.
target(HEADER_BUILD .. "/root_pointers.h")
    set_enabled(PROCESS_QSTR)
    -- $(HEADER_BUILD)/root_pointers.h: $(HEADER_BUILD)/root_pointers.collected $(PY_SRC)/make_root_pointers.py
    add_deps(HEADER_BUILD .. "/root_pointers.collected")
    -- add_deps(PY_SRC .. "/make_root_pointers.py")

    -- $(Q)$(PYTHON) $(PY_SRC)/make_root_pointers.py $< > $@
    set_values("execp1", "sh -c '" .. PYTHON .. " " .. PY_SRC .. "/make_root_pointers.py " .. HEADER_BUILD .. "/root_pointers.collected > ")

    on_build(function (target)
        print("GEN ", target:name())
        os.exec(target:values("execp1") .. target:name() .. "'")
    end)
target_end()


--------------------------------------------------------
--                     base64 encoder
-- https://github.com/iskolbin/lbase64/blob/master/base64.lua
--------------------------------------------------------
local base64 = {}

function base64.makeencoder( s62, s63, spad )
    local encoder = {}
    for b64code, char in pairs{[0]='A','B','C','D','E','F','G','H','I','J',
        'K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y',
        'Z','a','b','c','d','e','f','g','h','i','j','k','l','m','n',
        'o','p','q','r','s','t','u','v','w','x','y','z','0','1','2',
        '3','4','5','6','7','8','9',s62 or '+',s63 or'/',spad or'='} do
        encoder[b64code] = char:byte()
    end
    return encoder
end
local DEFAULT_ENCODER = base64.makeencoder()

extract = function( v, from, width )
    local w = 0
    local flag = 2^from
    for i = 0, width-1 do
        local flag2 = flag + flag
        if v % flag2 >= flag then
            w = w + 2^i
        end
        flag = flag2
    end
    return w
end

local char, concat = string.char, table.concat

function base64.encode( str, encoder, usecaching )
    encoder = encoder or DEFAULT_ENCODER
    local t, k, n = {}, 1, #str
    local lastn = n % 3
    local cache = {}
    for i = 1, n-lastn, 3 do
        local a, b, c = str:byte( i, i+2 )
        local v = a*0x10000 + b*0x100 + c
        local s
        if usecaching then
            s = cache[v]
            if not s then
                s = char(encoder[extract(v,18,6)], encoder[extract(v,12,6)], encoder[extract(v,6,6)], encoder[extract(v,0,6)])
                cache[v] = s
            end
        else
            s = char(encoder[extract(v,18,6)], encoder[extract(v,12,6)], encoder[extract(v,6,6)], encoder[extract(v,0,6)])
        end
        t[k] = s
        k = k + 1
    end
    if lastn == 2 then
        local a, b = str:byte( n-1, n )
        local v = a*0x10000 + b*0x100
        t[k] = char(encoder[extract(v,18,6)], encoder[extract(v,12,6)], encoder[extract(v,6,6)], encoder[64])
    elseif lastn == 1 then
        local v = str:byte( n )*0x10000
        t[k] = char(encoder[extract(v,18,6)], encoder[extract(v,12,6)], encoder[64], encoder[64])
    end
    return concat( t )
end

--------------------------------------------------------
--                     main stub target
--------------------------------------------------------
target("mainstub")
    before_build(function (target)
        local mainfn = "main.py"
        if not os.exists("./examples/" .. mainfn) then
            if MAIN_STUB_URL ~= "" then
                try { function()
                    if ZIP_COMPRESS == true then
                        os.exec("curl --silent --compressed " .. MAIN_STUB_URL .. " -o ./examples/" .. mainfn)
                    else
                        os.exec("curl --silent " .. MAIN_STUB_URL .. " -o ./examples/" .. mainfn)
                    end
                end,
                catch { function (e) print("Failed to get " .. mainfn .. " from " .. MAIN_STUB_URL) end }  
                }
            end
        end
    end)
    on_build(function (target)
        local mainfn = "main.py"
        local bootfn = "boot.py"
        if(get_config("05 Main stub respawn")) then
            print("Generate boot.py module with inserted '" .. mainfn .. "'")
            if os.exists("examples/" .. mainfn) then
                local file = io.open("examples/" .. mainfn, "r")
                local maincontent = base64.encode(file:read("*all"))
                file:close(file)
                file = io.open("modules/" .. bootfn, "w")
                file:write("import binascii\n")
                -- file:write("os.remove('main.py')\n")
                file:write("try:\n")
                file:write("    os.stat('" .. mainfn .. "')\n")
                file:write("except:\n")
                file:write("    f=open('" .. mainfn .. "', 'w')\n")
                file:write("    f.write(binascii.a2b_base64('" .. maincontent .. "'))\n")
                file:write("    f.close()\n")
                file:write("    pass")
                file:close(file)
            else
                print(mainfn .. " not found. Continue")
            end
        else
            if os.exists("modules/" .. bootfn) then
                print("Remove '" .. bootfn .. "'")
                os.rm("modules/" .. bootfn)
            end
        end
    end)
target_end()

--------------------------------------------------------
--                     main target
--------------------------------------------------------
target(USER_PROJECT_NAME..".elf")
    set_default(true)
    add_deps("driver")
    add_deps(USER_PROJECT_NAME)

    set_kind("binary")
    set_targetdir(BUILD .. "/"..USER_PROJECT_NAME)

    add_files(SRC_C, {public = true, cflags = CFLAGS })
    if #SRC_S > 0 then add_files(SRC_S, {public = true, cflags = CFLAGS }) end
    if #SHARED_SRC_C > 0 then add_files(SHARED_SRC_C, {public = true, cflags = CFLAGS }) end
    if #SHARED_SRC_S > 0 then add_files(SHARED_SRC_S, {public = true, cflags = CFLAGS }) end
    if #LIB_SRC_C > 0 then add_files(LIB_SRC_C, {public = true, cflags = CFLAGS }) end
    if #DRIVERS_SRC_C > 0 then add_files(DRIVERS_SRC_C, {public = true, cflags = CFLAGS }) end
    if #SRC_THIRDPARTY_C > 0 then add_files(SRC_THIRDPARTY_C, {public = true, cflags = CFLAGS }) end

    add_ldflags(LD_BASE_FLAGS .. " -Wl,--whole-archive -Wl,--start-group " .. LIB_BASE .. LIB_USER .. " -Wl,--end-group -Wl,--no-whole-archive -Wl,--no-undefined -Wl,--no-print-map-discarded -ldriver -l" .. USER_PROJECT_NAME, {force=true})    

    on_load(function(target)
        print("----------------------------------------------------")
        print("Compiling Version " .. FW_VERSION .. " for " ..  BOARD .. " on " .. os.host():gsub("^%l", string.upper))
        print("----------------------------------------------------")
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
        os.exec(GCC_DIR .. "bin/arm-none-eabi-gcc -E " .. FLAGS .. " -I " .. SDK_PATH .. "/PLAT/device/target/board/ec618_0h00/common/inc" .. " -P " .. SDK_PATH .. "/PLAT/core/ld/ec618_0h00_flash.c" ..  " -o " .. SDK_PATH .. "/PLAT/core/ld/ec618_0h00_flash.ld")
    end)

    after_build(function(target)
        if os.getenv("GCC_PATH") then
            GCC_DIR = os.getenv("GCC_PATH").."/"
        else
            GCC_DIR = target:toolchains()[1]:sdkdir().."/"
        end
        OUT_PATH = "./out"
        VERSION_PATH = "./version"
        if not os.exists(OUT_PATH) then os.mkdir(OUT_PATH) end
        if not os.exists(VERSION_PATH) then os.mkdir(VERSION_PATH) end

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
        os.cp(SDK_PATH .. "/PLAT/comdb.txt", OUT_PATH)

        ---------------------------------------------------------
        -------------- This part is not yet cross-platform
        local cmd = "-M -input " .. SDK_PATH .. "/PLAT/tools/ap_bootloader.bin -addrname  BL_IMG_MERGE_ADDR -flashsize BOOTLOADER_FLASH_LOAD_SIZE -input $(buildir)/"..USER_PROJECT_NAME.."/ap.bin -addrname  AP_IMG_MERGE_ADDR -flashsize AP_FLASH_LOAD_SIZE -input " .. SDK_PATH .. "/PLAT/prebuild/FW/lib/cp-demo-flash.bin -addrname CP_IMG_MERGE_ADDR -flashsize CP_FLASH_LOAD_SIZE -def " .. SDK_PATH .. "/PLAT/device/target/board/ec618_0h00/common/inc/mem_map.h "
        if os.getenv("BINPKG_CROSS") then
            --Prepare a custom packager
            cmd = os.getenv("BINPKG_CROSS") .. cmd
        else
            if is_plat("windows") then
                cmd = SDK_PATH .. "/PLAT/tools/fcelf.exe " .. cmd
            elseif is_plat("macosx") then
                cmd = "bash ./fcelf-docker.sh " .. cmd
            else
                cmd = SDK_PATH .. "/fcelf " .. cmd
            end
        end
        cmd = cmd .. " -outfile " .. VERSION_PATH .. "/" .. USER_PROJECT_NAME .. "_" .. USER_PROJECT_NAME_VERSION .. ".binpkg"
        -- If your platform does not have fcelf, you can comment out the following line and no binpkg will be generated.
        -- You can still use other tools to continue flashing your phone
        -- print(cmd)
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
        if not os.exists(OUT_PATH.."/pack") then
            os.mkdir(OUT_PATH.."/pack")
        end

        os.cp(OUT_PATH.."/*.binpkg", OUT_PATH.."/pack")
        os.cp(OUT_PATH.."/*.elf", OUT_PATH.."/pack")
        os.cp(OUT_PATH.."/*.map", OUT_PATH.."/pack")
        os.cp(SDK_PATH .. "/PLAT/comdb.txt", OUT_PATH.."/pack")
        os.cp(SDK_PATH .. "/project/luatos/pack/info.json", OUT_PATH.."/pack")
        local info_table = json.loadfile(OUT_PATH.."/pack/info.json")
        info_table["rom"]["file"] = USER_PROJECT_NAME .. ".binpkg"
        json.savefile(OUT_PATH.."/pack/info.json", info_table)
        os.cp(SDK_PATH .. "/PLAT/device/target/board/ec618_0h00/common/inc/mem_map.h", OUT_PATH .. "/pack")
        os.exec(path7z.." a -mx9 -bd -bso0 "..USER_PROJECT_NAME.."_ec618.7z "..OUT_PATH.."/pack/* -r")
        os.mv(USER_PROJECT_NAME.."_ec618.7z", OUT_PATH.."/"..USER_PROJECT_NAME.."_ec618.soc")
        os.rm(OUT_PATH.."/pack")

        if DEPLOY_BINPKG_FOLDER ~= "" and os.exists(DEPLOY_BINPKG_FOLDER) then
            if DEPLOY_BINPKG_COMPRESS == true then
                print("----------------------------------------------------")
                print("Deploy BINPKG to " .. DEPLOY_BINPKG_FOLDER .. " (compressed)")
                if is_plat("windows") then 
                    os.exec(".\\binexport.bat " .. VERSION_PATH .. "\\" .. USER_PROJECT_NAME .. "_".. USER_PROJECT_NAME_VERSION .. ".binpkg " .. DEPLOY_BINPKG_FOLDER .. "\\" .. USER_PROJECT_NAME .. "_".. USER_PROJECT_NAME_VERSION .. ".binpkg")
                elseif is_plat("macosx") then
                    -- TODO
                else 
                    os.exec("./binexport.sh " .. VERSION_PATH .. "/" .. USER_PROJECT_NAME .. "_".. USER_PROJECT_NAME_VERSION .. ".binpkg " .. DEPLOY_BINPKG_FOLDER .. "/" .. USER_PROJECT_NAME .. "_".. USER_PROJECT_NAME_VERSION .. ".binpkg")
                end
            else
                print("----------------------------------------------------")
                print("Deploy BINPKG to " .. DEPLOY_BINPKG_FOLDER)
                os.cp(VERSION_PATH .. "/" .. USER_PROJECT_NAME .. "_".. USER_PROJECT_NAME_VERSION .. ".binpkg ", DEPLOY_BINPKG_FOLDER .. "/" .. USER_PROJECT_NAME .. "_".. USER_PROJECT_NAME_VERSION .. ".binpkg")
            end
        end
    end)    
target_end()


target("ota")
    set_default(false)
    -- add_deps(USER_PROJECT_NAME..".elf")
    set_values("FOTA_URL", FOTA_URL)

    on_build(function (target)
        print("----------------------------------------------------")
        print("Make OTA pack(s) for " .. FW_VERSION .. " on " .. os.host():gsub("^%l", string.upper))
        print("----------------------------------------------------")

        OUT_PATH = "./out"
        OUT_DEP_PATH = "./out/dep"
        FOTA_PATH = "./fota"
        VERSION_PATH = "./version"
        if not os.exists(OUT_PATH) then os.mkdir(OUT_PATH) end        
        if not os.exists(OUT_DEP_PATH) then os.mkdir(OUT_DEP_PATH) end
        if not os.exists(FOTA_PATH) then os.mkdir(FOTA_PATH) end
        if not os.exists(VERSION_PATH) then os.mkdir(VERSION_PATH) end

        if is_plat("windows") then
            fcelf = "fcelf.exe"
            sha256sum = "sha256sum.exe"
            deltagen = "deltagen.exe"
            ftk = "FotaToolkit.exe"
            ftk_run = "fota.bat"
            ftk_exec = string.gsub(OUT_PATH, "./", "") .. "\\" .. ftk_run
            file = io.open(OUT_PATH .. "/" .. ftk_run, "w")
            file:write("cd " .. string.gsub(OUT_PATH, "./", "") .. "\n")
            file:write(ftk .. " %*\n")
            file:close(file)
        elseif is_plat("macosx") then
            -- TODO
        else
            fcelf = "fcelf"
            sha256sum = "sha256sum"
            deltagen = "deltagen"
            ftk = "FotaToolkit" 
            ftk_run = "fota.sh"
            ftk_exec = "./" .. string.gsub(OUT_PATH, "./", "") .. "/" .. ftk_run
            file = io.open(OUT_PATH .. "/" .. ftk_run, "w")
            file:write("#!/bin/bash\n")
            file:write("cd " .. OUT_PATH .. "\n")
            file:write("./" .. ftk .. " $@\n")
            file:close(file)
            os.exec("chmod +x " .. OUT_PATH .. "/" .. ftk_run)
        end
        fcelf_from = SDK_PATH .. "/tools/dtools/dep/" .. fcelf
        fcelf_to = OUT_DEP_PATH .. "/" .. fcelf
        sha256sum_from = SDK_PATH .. "/tools/dtools/dep/" .. sha256sum
        sha256sum_to = OUT_DEP_PATH .. "/" .. sha256sum
        deltagen_from = SDK_PATH .. "/tools/dtools/dep/" .. deltagen
        deltagen_to = OUT_DEP_PATH .. "/" .. deltagen
        ftk_from = SDK_PATH .. "/tools/dtools/" .. ftk
        ftk_to = OUT_PATH .. "/" .. ftk 

        if not os.exists(fcelf_to) then os.cp(fcelf_from, fcelf_to) end
        if not os.exists(sha256sum_to) then os.cp(sha256sum_from, sha256sum_to) end
        if not os.exists(deltagen_to) then os.cp(deltagen_from, deltagen_to) end
        if not os.exists(ftk_to) then os.cp(ftk_from, ftk_to) end

        fota_file = ""
        target_file = path.cygwin_path(string.gsub(VERSION_PATH, "./", "") .. "/" .. USER_PROJECT_NAME .. "_" .. FW_VERSION .. ".binpkg")
        for _, source_file in ipairs(os.files(VERSION_PATH .. "/*.binpkg")) do
            base_file = path.cygwin_path(source_file)
            if (base_file ~= target_file) then
                ind1 = string.find(base_file, USER_PROJECT_NAME .. "_")
                ind2 = string.find(base_file, ".binpkg")
                if (ind1 >= 0 and ind2 >= 0) then
                    OLDVER = string.sub(base_file, ind1 + string.len(USER_PROJECT_NAME) + 1, ind2 - 1)
                    if(OLDVER ~= "") then
                        print("Make fota " .. OLDVER .. " to " .. FW_VERSION ..  " upgrade pack...")
                        fota_url = target:values("FOTA_URL")
                        index = fota_url:match'^.*()/'
                        if(index >= 0 and string.find(fota_url, "OLDVERSION") >= 0 and string.find(fota_url, "NEWVERSION") >= 0) then
                            fota_file = string.sub(fota_url, index + 1)
                            fota_file = string.gsub(fota_file, "OLDVERSION", OLDVER)
                            fota_file = string.gsub(fota_file, "NEWVERSION", FW_VERSION)
                            -- print(ftk_exec .. " -d ../ec618.json BINPKG ../" .. FOTA_PATH .. "/" .. fota_file .. " ../" .. VERSION_PATH .. "/" .. USER_PROJECT_NAME .. "_" ..  OLDVER .. ".binpkg ../" .. VERSION_PATH .. "/" .. USER_PROJECT_NAME .. "_" ..  FW_VERSION ..  ".binpkg")
                            os.exec(ftk_exec .. " -d ../ec618.json BINPKG ../" .. FOTA_PATH .. "/" .. fota_file .. " ../" .. VERSION_PATH .. "/" .. USER_PROJECT_NAME .. "_" ..  OLDVER .. ".binpkg ../" .. VERSION_PATH .. "/" .. USER_PROJECT_NAME .. "_" ..  FW_VERSION ..  ".binpkg")
                            if fota_file ~= "" and DEPLOY_PACK_FOLDER ~= "" and os.exists(DEPLOY_PACK_FOLDER) then
                                print("----------------------------------------------------")
                                if DEPLOY_PACK_COMPRESS == true then
                                    print("Deploy FOTA pack to " .. DEPLOY_PACK_FOLDER .. " (compressed)")                
                                    if is_plat("windows") then 
                                        os.exec(".\\binexport.bat " .. FOTA_PATH .. "\\" .. fota_file .. " " .. DEPLOY_PACK_FOLDER .. "\\" .. fota_file)
                                    elseif is_plat("macosx") then
                                        -- TODO
                                    else 
                                        os.exec("./binexport.sh " .. FOTA_PATH .. "/" .. fota_file .. " " .. DEPLOY_PACK_FOLDER .. "/" .. fota_file)
                                    end
                                else
                                    print("Deploy FOTA pack to " .. DEPLOY_PACK_FOLDER)
                                    os.cp(FOTA_PATH .. "/" .. fota_file,  DEPLOY_PACK_FOLDER .. "/" .. fota_file)
                                end
                                print("----------------------------------------------------")
                            end
                        else
                            print("Can not parse FOTA URL")
                        end
                    end
                end
            end
        end
    end)
target_end()
