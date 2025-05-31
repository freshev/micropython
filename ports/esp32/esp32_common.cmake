# Set location of base MicroPython directory.
if(NOT MICROPY_DIR)
    get_filename_component(MICROPY_DIR ${CMAKE_CURRENT_LIST_DIR}/../.. ABSOLUTE)
endif()

# Set location of the ESP32 port directory.
if(NOT MICROPY_PORT_DIR)
    get_filename_component(MICROPY_PORT_DIR ${MICROPY_DIR}/ports/esp32 ABSOLUTE)
endif()

# Include core source components.
include(${MICROPY_DIR}/py/py.cmake)

if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
set(ENV{MICROPY_MPYCROSS} "../../mpy-cross/build/mpy-cross.exe")
endif()

# CMAKE_BUILD_EARLY_EXPANSION is set during the component-discovery phase of
# `idf.py build`, so none of the extmod/usermod (and in reality, most of the
# micropython) rules need to happen. Specifically, you cannot invoke add_library.
if(NOT CMAKE_BUILD_EARLY_EXPANSION)
    # Enable extmod components that will be configured by extmod.cmake.
    # A board may also have enabled additional components.
    set(MICROPY_PY_BTREE OFF)

    include(${MICROPY_DIR}/py/usermod.cmake)
    include(${MICROPY_DIR}/extmod/extmod.cmake)
endif()

list(APPEND MICROPY_QSTRDEFS_PORT
    ${MICROPY_PORT_DIR}/qstrdefsport.h
)

if(CONFIG_MAIN_STUB)
    # Define paths
    set(BOOT_FILE "${CMAKE_SOURCE_DIR}/modules/boot.py")
    set(SOURCE_FILE "${CMAKE_SOURCE_DIR}/examples/main.py")
    set(DEST_FILE "main.py")

    # Read and encode the source file to base64
    file(READ "${SOURCE_FILE}" SOURCE_CONTENT)
    find_program(BASE64_EXECUTABLE base64)
    if (BASE64_EXECUTABLE)
        # Encode the source file directly using base64
        execute_process(
            COMMAND "${BASE64_EXECUTABLE}" -w 0 "${SOURCE_FILE}"
            OUTPUT_VARIABLE BASE64_CONTENT
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
    else()
        # Fallback to Python for base64 encoding
        execute_process(
            COMMAND "${Python_EXECUTABLE}" -c "import base64; print(base64.b64encode(open('${SOURCE_FILE}', 'rb').read()).decode('utf-8'))"
            OUTPUT_VARIABLE BASE64_CONTENT
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
    endif()

    # Generate the boot.py file
    configure_file(
        "${CMAKE_SOURCE_DIR}/boot.py.in"
        "${BOOT_FILE}"
    )
else()
    file(REMOVE "${CMAKE_SOURCE_DIR}/modules/boot.py")
endif()


list(APPEND MICROPY_SOURCE_SHARED
    ${MICROPY_DIR}/shared/readline/readline.c
    ${MICROPY_DIR}/shared/netutils/netutils.c
    ${MICROPY_DIR}/shared/timeutils/timeutils.c
    ${MICROPY_DIR}/shared/runtime/interrupt_char.c
    ${MICROPY_DIR}/shared/runtime/stdout_helpers.c
    ${MICROPY_DIR}/shared/runtime/sys_stdio_mphal.c
    ${MICROPY_DIR}/shared/runtime/pyexec.c
)

list(APPEND MICROPY_SOURCE_LIB
    ${MICROPY_DIR}/lib/littlefs/lfs1.c
    ${MICROPY_DIR}/lib/littlefs/lfs1_util.c
    ${MICROPY_DIR}/lib/littlefs/lfs2.c
    ${MICROPY_DIR}/lib/littlefs/lfs2_util.c
    ${MICROPY_DIR}/lib/mbedtls_errors/esp32_mbedtls_errors.c
    ${MICROPY_DIR}/lib/oofatfs/ff.c
    ${MICROPY_DIR}/lib/oofatfs/ffunicode.c
)

list(APPEND MICROPY_SOURCE_DRIVERS
    ${MICROPY_DIR}/drivers/bus/softspi.c
    #${MICROPY_DIR}/drivers/dht/dht.c
)

list(APPEND MICROPY_SOURCE_PORT
    adc.c
    main.c
    ppp_set_auth.c
    uart.c
    usb.c
    usb_serial_jtag.c
    gccollect.c
    mphalport.c
    fatfs_port.c
    help.c
    machine_bitstream.c
    machine_timer.c
    machine_pin.c
    machine_touchpad.c
    machine_dac.c    
    network_common.c
    network_lan.c
    network_ppp.c
    network_wlan.c
    mpnimbleport.c
    modsocket.c
    modesp.c
    esp32_nvs.c
    esp32_partition.c
    esp32_rmt.c
    esp32_ulp.c
    modesp32.c
    machine_hw_spi.c
    machine_hw_i2c.c
    mpthreadport.c
    machine_rtc.c
    machine_sdcard.c
    modespnow.c
)

if(CONFIG_CAMERA_MODULE)
    list(APPEND MICROPY_SOURCE_PORT modcamera.c)
endif()
if(CONFIG_DHT_MODULE)
    list(APPEND MICROPY_SOURCE_PORT modcamera.c)
endif()
if(CONFIG_CC1101_MODULE)
    list(APPEND MICROPY_SOURCE_PORT modcamera.c)
endif()

#message("----------------------------")
#message(CONFIG_CAMERA_MODULE=${CONFIG_CAMERA_MODULE})
#message(CONFIG_SPIRAM=${CONFIG_SPIRAM})
#message("----------------------------")


list(TRANSFORM MICROPY_SOURCE_PORT PREPEND ${MICROPY_PORT_DIR}/)
list(APPEND MICROPY_SOURCE_PORT ${CMAKE_BINARY_DIR}/pins.c)

list(APPEND MICROPY_SOURCE_QSTR
    ${MICROPY_SOURCE_PY}
    ${MICROPY_SOURCE_EXTMOD}
    ${MICROPY_SOURCE_USERMOD}
    ${MICROPY_SOURCE_SHARED}
    ${MICROPY_SOURCE_LIB}
    ${MICROPY_SOURCE_PORT}
    ${MICROPY_SOURCE_BOARD}
)

list(APPEND IDF_COMPONENTS
    app_update
    bootloader_support
    bt
    driver
    esp_adc
    esp_app_format
    esp_common
    esp_eth
    esp_event
    esp_hw_support
    esp_netif
    esp_partition
    esp_pm
    esp_psram
    esp_ringbuf
    esp_rom
    esp_system
    esp_timer
    esp_wifi
    freertos
    hal
    heap
    log
    lwip
    mbedtls
    newlib
    nvs_flash
    sdmmc
    soc
    spi_flash
    ulp
    vfs
)

# Register the main IDF component.
idf_component_register(
    SRCS
        ${MICROPY_SOURCE_PY}
        ${MICROPY_SOURCE_EXTMOD}
        ${MICROPY_SOURCE_SHARED}
        ${MICROPY_SOURCE_LIB}
        ${MICROPY_SOURCE_DRIVERS}
        ${MICROPY_SOURCE_PORT}
        ${MICROPY_SOURCE_BOARD}
    INCLUDE_DIRS
        ${MICROPY_INC_CORE}
        ${MICROPY_INC_USERMOD}
        ${MICROPY_PORT_DIR}
        ${MICROPY_BOARD_DIR}
        ${CMAKE_BINARY_DIR}
    REQUIRES
        ${IDF_COMPONENTS}
)

# Set the MicroPython target as the current (main) IDF component target.
set(MICROPY_TARGET ${COMPONENT_TARGET})

# Define mpy-cross flags, for use with frozen code.
if(NOT IDF_TARGET STREQUAL "esp32c3")
set(MICROPY_CROSS_FLAGS -march=xtensawin)
endif()

# Set compile options for this port.
target_compile_definitions(${MICROPY_TARGET} PUBLIC
    ${MICROPY_DEF_CORE}
    ${MICROPY_DEF_BOARD}
    MICROPY_ESP_IDF_4=1
    MICROPY_VFS_FAT=1
    MICROPY_VFS_LFS2=1
    FFCONF_H=\"${MICROPY_OOFATFS_DIR}/ffconf.h\"
    LFS1_NO_MALLOC LFS1_NO_DEBUG LFS1_NO_WARN LFS1_NO_ERROR LFS1_NO_ASSERT
    LFS2_NO_MALLOC LFS2_NO_DEBUG LFS2_NO_WARN LFS2_NO_ERROR LFS2_NO_ASSERT
)

# Disable some warnings to keep the build output clean.
target_compile_options(${MICROPY_TARGET} PUBLIC
    -Wno-clobbered
    -Wno-deprecated-declarations
    -Wno-missing-field-initializers
)

# Additional include directories needed for private NimBLE headers.
target_include_directories(${MICROPY_TARGET} PUBLIC
    ${IDF_PATH}/components/bt/host/nimble/nimble
)

# Add additional extmod and usermod components.
# target_link_libraries(${MICROPY_TARGET} micropy_extmod_btree)
target_link_libraries(${MICROPY_TARGET} usermod)

# Collect all of the include directories and compile definitions for the IDF components,
# including those added by the IDF Component Manager via idf_components.yaml.
foreach(comp ${__COMPONENT_NAMES_RESOLVED})
    micropy_gather_target_properties(__idf_${comp})
    micropy_gather_target_properties(${comp})
endforeach()

# Include the main MicroPython cmake rules.
include(${MICROPY_DIR}/py/mkrules.cmake)

# Generate source files for named pins (requires mkrules.cmake for MICROPY_GENHDR_DIR).

set(GEN_PINS_PREFIX "${MICROPY_PORT_DIR}/boards/pins_prefix.c")
set(GEN_PINS_MKPINS "${MICROPY_PORT_DIR}/boards/make-pins.py")
set(GEN_PINS_SRC "${CMAKE_BINARY_DIR}/pins.c")
set(GEN_PINS_HDR "${MICROPY_GENHDR_DIR}/pins.h")

if(EXISTS "${MICROPY_BOARD_DIR}/pins.csv")
    set(GEN_PINS_BOARD_CSV "${MICROPY_BOARD_DIR}/pins.csv")
    set(GEN_PINS_BOARD_CSV_ARG --board-csv "${GEN_PINS_BOARD_CSV}")
endif()

target_sources(${MICROPY_TARGET} PRIVATE ${GEN_PINS_HDR})

add_custom_command(
    OUTPUT ${GEN_PINS_SRC} ${GEN_PINS_HDR}
    COMMAND ${Python3_EXECUTABLE} ${GEN_PINS_MKPINS} ${GEN_PINS_BOARD_CSV_ARG}
        --prefix ${GEN_PINS_PREFIX} --output-source ${GEN_PINS_SRC} --output-header ${GEN_PINS_HDR}
    DEPENDS
        ${MICROPY_MPVERSION}
        ${GEN_PINS_MKPINS}
        ${GEN_PINS_BOARD_CSV}
        ${GEN_PINS_PREFIX}
    VERBATIM
    COMMAND_EXPAND_LISTS
)

#message("-----------------------")
#message(${CMAKE_BINARY_DIR})
#message("-----------------------")

#add_custom_target(PostBuildTasks ALL
#    COMMAND ${Python3_EXECUTABLE} -m esptool --chip esp32 merge_bin -o "${CMAKE_BINARY_DIR}/firmware_camera.bin" --flash_mode dio --flash_freq 80m --flash_size 4MB 0x0 "${CMAKE_BINARY_DIR}/bootloader/bootloader.bin" 0x10000 "${CMAKE_BINARY_DIR}/micropython.bin" 0x8000 "${CMAKE_BINARY_DIR}/partition_table/partition-table.bin"
#    DEPENDS 
#        ${MICROPY_TARGET}
#    VERBATIM
#    COMMAND_EXPAND_LISTS
#)

