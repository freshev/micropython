# Copyright (C) 2018 RDA Technologies Limited and/or its affiliates("RDA").
# All rights reserved.
#
# This software is supplied "AS IS" without any warranties.
# RDA assumes no responsibility or liability for the use of the software,
# conveys no license or title under any patent, copyright, or mask work
# right to the product. RDA reserves the right to make changes in the
# software without notification.  RDA also make no representation or
# warranty that such application will be suitable for the specified use
# without further testing or modification.

cmake_minimum_required(VERSION 3.13)

set(CONFIG_BUILD_MICROPYTHON "ON")
set(CSDK_VER 1.0.0)
set(FOTA_FLAG "fota")
set(FOTA_TYPE "all")
set(AM_MODEL iot_SDK_720U)
set(BUILD_TARGET "micropython_Air724")
set(MICROPY_BOARD "Air724_GENERIC")
set(TARGET_CPU "ARM Cortex-M4")
set(CONFIG_BUILD_SFFS "ON")

get_filename_component(CMAKE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../lib/Luat_CSDK_Air724U ABSOLUTE)

#message("----------------------------------------")
#message("CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")
#message("CMAKE_CURRENT_SOURCE_DIR: ${CMAKE_CURRENT_SOURCE_DIR}")
#message("CMAKE_CURRENT_BINARY_DIR: ${CMAKE_CURRENT_BINARY_DIR}")
#message("----------------------------------------")

set(SOURCE_TOP_DIR ${CMAKE_SOURCE_DIR})
set(BINARY_TOP_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(CMAKE_C_COMPILER_FORCED   1)
set(CMAKE_CXX_COMPILER_FORCED 1)

add_definitions(-D_WINSOCKAPI_)
add_definitions(-DCSDK_VER="${CSDK_VER}")
message("BUILD_TARGET: ${BUILD_TARGET}")
message("CSDK_VER: ${CSDK_VER} ")
if (NOT ((${FOTA_TYPE} STREQUAL  "app") OR (${FOTA_TYPE} STREQUAL  "core") OR (${FOTA_TYPE} STREQUAL "all")))
	message(FATAL_ERROR "FOTA_TYPE: ${FOTA_TYPE} != app or core or all")
endif()
message("${FOTA_FLAG}=${FOTA_TYPE}")

#select core_type  0=iot_SDK_720U 1:iot_SDK_720U_BT_TTS 2:iot_SDK_720U_BT_TTS_VOLTE

if ((${AM_MODEL} STREQUAL  "iot_SDK_720U"))
	message("core/iot_SDK_720U")
	set(core_type 0)	
	include(${CMAKE_SOURCE_DIR}/core/${AM_MODEL}/memd/version.cmake)
	set(core_name CSDK_RDA8910)
	set(core_vername CSDK_${CORE_VER}_RDA8910)
	set(core_bin_dir ${CMAKE_SOURCE_DIR}/core/iot_SDK_720U)
	set(core_stub_o ${core_bin_dir}/lib/core_stub.o)
	include(${core_bin_dir}/memd/core_config.cmake)
	include(${core_bin_dir}/memd/partinfo.cmake)
	include_directories(${core_bin_dir}/memd)
    set(csdk_target ${BUILD_TARGET})
	set(APP_IMG_NAME "8915DM_cat1_CSDK")
	set(UIS8915DM_cus_name "cat1_UIS8915DM_BB_RF_SS_NoVolte_cus")
elseif((${AM_MODEL} STREQUAL  "iot_SDK_720U_BT_TTS"))
	message("core/iot_SDK_720U_BT_TTS")
	set(core_type 1)
	add_definitions(-DTTS_ENABLE)
	include(${CMAKE_SOURCE_DIR}/core/${AM_MODEL}/memd/version.cmake)
	set(core_name CSDK_RDA8910_BT_TTS)
	set(core_vername CSDK_${CORE_VER}_RDA8910_BT_TTS)
	set(core_bin_dir ${CMAKE_SOURCE_DIR}/core/iot_SDK_720U_BT_TTS)
	set(core_stub_o ${core_bin_dir}/lib/core_stub.o)
	include(${core_bin_dir}/memd/core_config.cmake)
	include(${core_bin_dir}/memd/partinfo.cmake)
	include_directories(${core_bin_dir}/memd)
    set(csdk_target ${BUILD_TARGET})
	set(APP_IMG_NAME "8915DM_cat1_CSDK_BT_TTS")
    set(UIS8915DM_cus_name "cat1_UIS8915DM_BB_RF_SS_NoVolte_cus")
elseif((${AM_MODEL} STREQUAL  "iot_SDK_720U_BT_TTS_VOLTE"))
	message("core/iot_SDK_720U_BT_TTS_VOLTE")
	set(core_type 2)
	add_definitions(-DTTS_ENABLE)
	include(${CMAKE_SOURCE_DIR}/core/${AM_MODEL}/memd/version.cmake)
	set(core_name CSDK_RDA8910_BT_TTS_VOLTE)
	set(core_vername CSDK_${CORE_VER}_RDA8910_BT_TTS_VOLTE)
	set(core_bin_dir ${CMAKE_SOURCE_DIR}/core/iot_SDK_720U_BT_TTS_VOLTE)
	set(core_stub_o ${core_bin_dir}/lib/core_stub.o)
	include(${core_bin_dir}/memd/core_config.cmake)
	include(${core_bin_dir}/memd/partinfo.cmake)
	include_directories(${core_bin_dir}/memd)
    set(csdk_target ${BUILD_TARGET})
	set(APP_IMG_NAME "8915DM_cat1_CSDK_BT_TTS_VOLTE")
    set(UIS8915DM_cus_name "cat1_UIS8915DM_BB_RF_SS_cus")
else()
	message(FATAL_ERROR "AM_MODEL: ${AM_MODEL} != iot_SDK_720U or iot_SDK_720U_BT_TTS or iot_SDK_720U_BT_TTS_VOLTE")
endif()


set(out_hex_dir ${CMAKE_CURRENT_BINARY_DIR}/hex)
set(out_lib_dir ${CMAKE_CURRENT_BINARY_DIR}/lib)
set(out_rel_dir ${CMAKE_CURRENT_BINARY_DIR}/rel)
set(out_inc_dir ${CMAKE_CURRENT_BINARY_DIR}/include)
set(tools_dir ${CMAKE_SOURCE_DIR}/tools)
set(flash_img ${out_hex_dir}/oldpac/sffs.img)

set(flash_ldscript ${SOURCE_TOP_DIR}/ldscripts/app_flashimg.ld)
set(file_ldscript ${SOURCE_TOP_DIR}/ldscripts/app_fileimg.ld)
set(cmd_mkappimg dtools mkappimg)
set(pacgen_py ${tools_dir}/pacgen.py)
set(gdbComGen_py ${tools_dir}/gdbComGen.py)

#include("${CMAKE_SOURCE_DIR}/cmake/toolchain-gcc.cmake")
################################################################################
set(CMAKE_SYSTEM_NAME           Generic)
find_program(CMAKE_C_COMPILER   ${CROSS_COMPILE}gcc)
find_program(CMAKE_CXX_COMPILER ${CROSS_COMPILE}g++)
find_program(CMAKE_READELF      ${CROSS_COMPILE}readelf)

set(CMAKE_EXECUTABLE_SUFFIX_ASM .elf)
set(CMAKE_EXECUTABLE_SUFFIX_C .elf)
set(CMAKE_EXECUTABLE_SUFFIX_CXX .elf)

execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpversion OUTPUT_VARIABLE gcc_version OUTPUT_STRIP_TRAILING_WHITESPACE)

if(CONFIG_CPU_ARM_CA5)
    set(abi_options -mcpu=cortex-a5 -mtune=generic-armv7-a -mthumb -mfpu=neon-vfpv4 -mfloat-abi=hard -mno-unaligned-access)
    set(partial_link_options)
    set(libc_file_name ${CMAKE_SOURCE_DIR}/components/newlib/armca5/libc.a)
    set(libm_file_name ${CMAKE_SOURCE_DIR}/components/newlib/armca5/libm.a)
endif()

if(CONFIG_ENABLE_GCC_LTO)
    set(lto_compile_option -flto -ffat-lto-objects)
    set(lto_link_option -flto)
endif()

add_compile_options(${abi_options} -g -Os ${lto_compile_option}
    -Wall
    -fno-strict-aliasing
    -ffunction-sections -fdata-sections
    -Wno-implicit-function-declaration
    -Wno-unused-function
)
add_link_options(${abi_options} -Os ${lto_link_option})
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11") # --specs=nosys.specs
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fno-exceptions -fno-rtti -fno-threadsafe-statics") # --specs=nosys.specs

# GNU ar will alreay create index
set(CMAKE_C_ARCHIVE_FINISH "")
set(CMAKE_CXX_ARCHIVE_FINISH "")

if(WITH_WERROR)
    add_compile_options(-Werror)
endif()

if(WITH_LINK_CREF)
    set(link_cref_option -Wl,-cref)
endif()

set(multilib_opions ${abi_options})
if((${gcc_version} VERSION_GREATER 8) AND (CONFIG_CPU_ARM_CA5))
    set(multilib_opions -march=armv7-a+neon-vfpv4 -mthumb -mfloat-abi=hard)
endif()
execute_process(COMMAND ${CMAKE_C_COMPILER} ${multilib_opions} --print-file-name libgcc.a
    OUTPUT_VARIABLE libgcc_file_name
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
################################################################################

include("${CMAKE_SOURCE_DIR}/cmake/extension.cmake")
#include("${CMAKE_SOURCE_DIR}/config.cmake")



if(NOT CONFIG_DEPLOY_FW_FOLDER STREQUAL "")
    if(CONFIG_DEPLOY_FW_COMPRESS STREQUAL "y")
        add_custom_target(deploy ALL
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            #COMMAND ${MICROPY_DIR}/lib/luatos-soc-2022/tools/dtools/dep/zlib-flate -compress < merged-binary.bin > ${CONFIG_DEPLOY_FW_FOLDER}/micropython_${MICROPY_BOARD}_compressed.bin
            #COMMAND ${MICROPY_DIR}/lib/luatos-soc-2022/tools/dtools/dep/zlib-flate -compress < micropython.elf > ${CONFIG_DEPLOY_FW_FOLDER}/micropython_${MICROPY_BOARD}_compressed.elf
            #DEPENDS merge-bin
        )
    else()
        add_custom_target(deploy ALL
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            #COMMAND ${CMAKE_COMMAND} -E copy merged-binary.bin ${CONFIG_DEPLOY_FW_FOLDER}/micropython_${MICROPY_BOARD}.bin
            #COMMAND ${CMAKE_COMMAND} -E copy micropython.elf ${CONFIG_DEPLOY_FW_FOLDER}/micropython_${MICROPY_BOARD}.elf
            #DEPENDS merge-bin
        )
    endif()
endif()


# message(${CONFIG_FS_SYS_FLASH_OFFSET})
if( NOT CONFIG_BOOT_FLASH_ADDRESS )
math(EXPR CONFIG_BOOT_FLASH_ADDRESS "${CONFIG_NOR_PHY_ADDRESS}+${CONFIG_BOOT_FLASH_OFFSET}" OUTPUT_FORMAT HEXADECIMAL)
math(EXPR CONFIG_APP_FLASH_ADDRESS "${CONFIG_NOR_PHY_ADDRESS}+${CONFIG_APP_FLASH_OFFSET}" OUTPUT_FORMAT HEXADECIMAL)
math(EXPR CONFIG_FS_MODEM_FLASH_ADDRESS "${CONFIG_NOR_PHY_ADDRESS}+${CONFIG_FS_MODEM_FLASH_OFFSET}" OUTPUT_FORMAT HEXADECIMAL)
endif()
set(CONFIG_NVBIN_FIXED_SIZE 0x20000)
math(EXPR CONFIG_FS_SYS_FLASH_OFFSET "${CONFIG_FS_SYS_FLASH_ADDRESS}-${CONFIG_NOR_PHY_ADDRESS}" OUTPUT_FORMAT HEXADECIMAL)
set(CONFIG_BOOTLOADER_SIZE ${CONFIG_BOOT_FLASH_SIZE})

set(BUILD_SHARED_LIBS OFF)

include_directories(${CMAKE_SOURCE_DIR}/api/include ${CMAKE_SOURCE_DIR}/components/include ${CMAKE_SOURCE_DIR}/components/openat_inc ${CMAKE_SOURCE_DIR}/components/newlib/include)

function(cpp_only target file)
    add_library(${target} OBJECT ${file})
    set_source_files_properties(${file} PROPERTIES LANGUAGE C)
    target_compile_options(${target} PRIVATE -E -P -x c)
endfunction()

function(add_appimg target ldscript)
    set(gen_ldscript ${target}_ldscript)
    set(target_map_file ${out_hex_dir}/${target}.map)
    set(target_img_file ${out_hex_dir}/${target}.img)    
    cpp_only(${gen_ldscript} ${ldscript})
    add_executable(${target} ${ARGN} ${core_stub_o})
    set_target_properties(${target} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${out_hex_dir})
    target_link_libraries(${target} PRIVATE -T $<TARGET_OBJECTS:${gen_ldscript}>)
    target_link_libraries(${target} PRIVATE -Wl,-Map=${target_map_file} -nostdlib -Wl,--gc-sections)
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${cmd_mkappimg} $<TARGET_FILE:${target}> ${target_img_file}
        BYPRODUCTS ${target_img_file} ${target_map_file}
    )
endfunction()

configure_file(${SOURCE_TOP_DIR}/components/lib/lua.img ${out_hex_dir}/lua.img COPYONLY)
set(dtoolspath ${SOURCE_TOP_DIR}/tools/win32/dtools.exe)

execute_process(
    COMMAND ${dtoolspath} pacdump ${core_bin_dir}/${core_name}.pac ${out_hex_dir}/oldpac
    OUTPUT_STRIP_TRAILING_WHITESPACE
    WORKING_DIRECTORY ${SOURCE_TOP_DIR}
)

set(target ${csdk_target})

set(hex_bin_dir ${CMAKE_CURRENT_SOURCE_DIR}/hex/${target})
set(hex_map_dir ${CMAKE_CURRENT_SOURCE_DIR}/hex/${target}_map)
set(version_dir ${CMAKE_CURRENT_SOURCE_DIR}/version)

set(core_map_dir ${core_bin_dir}_map)
set(hex_app_file ${hex_bin_dir}/${target}_APP.pac)
set(pac_delete_file ${out_hex_dir}/appimg_flash_delete.pac) 
set(hex_delete_file ${hex_bin_dir}/appimg_flash_delete.pac)
set(hex_app_with_core_file ${hex_bin_dir}/${target}.pac)
set(hex_map_file ${hex_map_dir}/${core_name}.map)
set(hex_elf_file ${hex_map_dir}/${core_name}.elf)

set(appimg ${out_hex_dir}/${target}.img)

set(pac_nvitem_8910
    cfg-nvitem -n "Calibration" -i 0xFFFFFFFF --use 1 --replace 0 --continue 0 --backup 1
    cfg-nvitem -n "GSM Calibration" -i 0x26d --use 1 --replace 0 --continue 1 --backup 1
    cfg-nvitem -n "LTE Calibration" -i 0x26e --use 1 --replace 0 --continue 0 --backup 1
    cfg-nvitem -n "IMEI" -i 0xFFFFFFFF --use 1 --replace 0 --continue 0 --backup 1
)

if(CONFIG_BUILD_SFFS)
    set(file_system
        cfg-image -i SFFS -a ${CONFIG_FS_SYS_FLASH_ADDRESS} -s ${CONFIG_FS_SYS_FLASH_SIZE}
        -p ${flash_img}
    )
else(CONFIG_BUILD_SFFS)
    set(file_system
        cfg-fmt-flash -i FMT_FSSYS -b FSYS
    )
endif(CONFIG_BUILD_SFFS)

add_appimg(${target} ${flash_ldscript})

target_link_libraries(${target} PRIVATE api)
target_link_libraries(${target} PRIVATE ${libc_file_name} ${libm_file_name} ${libgcc_file_name})

#######################################################
# Parse .config
#######################################################
set(KCONFIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/.config")

if(EXISTS ${KCONFIG_FILE})
    file(READ ${KCONFIG_FILE} KCONFIG_RAW)
    string(REGEX REPLACE "\n" ";" KCONFIG_LINES "${KCONFIG_RAW}")

    foreach(line IN LISTS KCONFIG_LINES)
        if(line MATCHES "^CONFIG_([A-Za-z0-9_]+)=\"([^\"]*)\"$")
            target_compile_definitions(${target} PRIVATE
                ${CMAKE_MATCH_1}="${CMAKE_MATCH_2}"
            )
            set(${CMAKE_MATCH_1} ${CMAKE_MATCH_2})
            #message("${CMAKE_MATCH_1}= \"${CMAKE_MATCH_2}\"")
        elseif(line MATCHES "^CONFIG_([A-Za-z0-9_]+)=y$")
            target_compile_definitions(${target} PRIVATE
                ${CMAKE_MATCH_1}=1
            )
            #message("${CMAKE_MATCH_1}=1")
        elseif(line MATCHES "^CONFIG_([A-Za-z0-9_]+)=n$")
            target_compile_definitions(${target} PRIVATE
                ${CMAKE_MATCH_1}=0
            )
            #message("${CMAKE_MATCH_1}=0")
        elseif(line MATCHES "^CONFIG_([A-Za-z0-9_]+)=([0-9]+)$")
            target_compile_definitions(${target} PRIVATE
                ${CMAKE_MATCH_1}=${CMAKE_MATCH_2}
            )
			#message("${CMAKE_MATCH_1}= ${CMAKE_MATCH_2}")
        endif()
    endforeach()
else()
    message(FATAL_ERROR "File ${KCONFIG_FILE} not found. Use configure to make it.")
endif()
#######################################################

set(pac_file ${out_hex_dir}/${target}_APP.pac)
set(pac_file_with_core ${out_hex_dir}/${target}.pac)
set(pac_file_with_version ${version_dir}/${target}_${FW_VERSION}.pac)

pac_init_fdl(init_fdl)
set(pac_app_8910 cfg-image -i APPIMG -a ${CONFIG_APPIMG_FLASH_ADDRESS} -s ${CONFIG_APPIMG_FLASH_SIZE} -p ${out_hex_dir}/${target}.img)   

add_custom_command(OUTPUT ${flash_img}    
    COMMAND cmd /c "sffs.bat ${out_hex_dir}/oldpac ${CONFIG_FS_SYS_FLASH_OFFSET} ${CONFIG_FS_SYS_FLASH_SIZE}"
    WORKING_DIRECTORY ${SOURCE_TOP_DIR}
)

add_custom_command(OUTPUT ${pac_file}
    COMMAND python3 ${pacgen_py}
        ${init_fdl}
        ${pac_nvitem_8910}
        ${pac_app_8910}
        dep-gen --base ${BINARY_TOP_DIR} ${pac_file} ${pac_file}.d
        pac-gen ${pac_file}
    DEPFILE ${pac_file}.d
    DEPENDS ${appimg} 
    DEPENDS ${flash_img} 
    WORKING_DIRECTORY ${SOURCE_TOP_DIR}
)

add_custom_command(OUTPUT ${pac_file_with_core}
    COMMAND python3 ${pacgen_py}
        ${init_fdl}
        ${pac_nvitem_8910}
        cfg-phase-check
		#/*+\BUG\chenxudong\2020.9.25\add secure boot enable*/
        cfg-image -i BOOTLOADER -a ${CONFIG_BOOT_FLASH_ADDRESS} -s ${CONFIG_BOOTLOADER_SIZE}
            -p ${out_hex_dir}/oldpac/boot.img
        cfg-image -i AP -a ${CONFIG_APP_FLASH_ADDRESS} -s ${CONFIG_APP_FLASH_SIZE}
            -p ${out_hex_dir}/oldpac/${APP_IMG_NAME}.img
		#/*+\BUG\chenxudong\2020.9.25\add secure boot enable*/
        ########NO###########
        ${cfg_ap2}
        ##############
        ${pac_app_8910}
        cfg-image -i PS -a ${CONFIG_FS_MODEM_FLASH_ADDRESS} -s ${CONFIG_FS_MODEM_FLASH_SIZE}
            -p ${out_hex_dir}/oldpac/${UIS8915DM_cus_name}.img
        ${file_system}
        #########NO##########
        ${cfg_fmt_fext}
        ##############
        cfg-clear-nv
        cfg-nv -s ${CONFIG_NVBIN_FIXED_SIZE} 
            -p ${out_hex_dir}/oldpac/${UIS8915DM_cus_name}_nvitem.bin
        # cfg-pack-cpio -i PREPACK -p ${prepack_cpio}
        dep-gen --base ${BINARY_TOP_DIR} ${pac_file_with_core} ${pac_file_with_core}.d
        pac-gen ${pac_file_with_core}
    DEPFILE ${pac_file_with_core}.d
    DEPENDS ${appimg} 
    WORKING_DIRECTORY ${SOURCE_TOP_DIR}
)
add_custom_target(${target}_pacgen ALL DEPENDS ${pac_file} ${pac_file_with_core})

# if(CONFIG_APPIMG_LOAD_FILE)
    
#     if(CONFIG_BUILD_LUA)
#         add_subdirectory_if_exist(app/elua/user)
#     else()
#         add_subdirectory_if_exist(demo/${BUILD_TARGET})
#     endif(CONFIG_BUILD_LUA)    

#     target_link_libraries(${target} PRIVATE ${libc_file_name} ${libm_file_name} ${libgcc_file_name})
#     set(pac_config ${out_hex_dir}/${target}.json)
#     set(pac_file ${out_hex_dir}/${target}_APP.pac)
# 	set(pac_file_with_core ${out_hex_dir}/${target}.pac)
	
#     pac_init_fdl(init_fdl ${pac_config})
#     execute_process(
#         COMMAND python3 ${pacgen_py} ${init_fdl}
#             cfg-pack-file -i APPIMG -p ${out_hex_dir}/${target}.img
#                 -n ${CONFIG_APPIMG_LOAD_FILE_NAME} ${pac_config}
#             dep-gen --base ${SOURCE_TOP_DIR} ${pac_config}
#         OUTPUT_VARIABLE pac_dep
#         OUTPUT_STRIP_TRAILING_WHITESPACE
#         WORKING_DIRECTORY ${SOURCE_TOP_DIR}
#     )

#     add_custom_command(OUTPUT ${pac_file}
#         COMMAND python3 ${pacgen_py} pac-gen ${pac_config} ${pac_file}
#         DEPENDS ${pacgen_py} ${pac_config} ${pac_dep}
#         WORKING_DIRECTORY ${SOURCE_TOP_DIR}
#     )

# 	add_custom_command(OUTPUT ${pac_file_with_core}
# 		COMMAND python3 ${pacgen_core_py} ${out_hex_dir}/${target}.img ${pac_file_with_core} TTS_ENABLE
# 		DEPENDS ${pacgen_core_py} ${pac_dep}
#         WORKING_DIRECTORY ${SOURCE_TOP_DIR}
# 	)
#     add_custom_target(${target}_pacgen ALL DEPENDS ${pac_file} ${pac_file_with_core})
# endif()

add_appimg_delete()

add_custom_command(OUTPUT ${hex_app_file} ${hex_app_with_core_file} ${hex_delete_file} ${hex_map_file} ${hex_elf_file}
	COMMAND ${CMAKE_COMMAND} -E make_directory ${hex_bin_dir}
	COMMAND ${CMAKE_COMMAND} -E copy ${pac_file} ${hex_app_file}
	COMMAND ${CMAKE_COMMAND} -E copy ${pac_file_with_core} ${hex_app_with_core_file}
	COMMAND ${CMAKE_COMMAND} -E make_directory ${hex_map_dir}
	COMMAND ${CMAKE_COMMAND} -E copy ${core_map_dir}/${core_name}.map ${hex_map_file}
    COMMAND ${CMAKE_COMMAND} -E copy ${core_map_dir}/${core_name}.elf ${hex_elf_file}
    COMMAND ${CMAKE_COMMAND} -E copy ${out_hex_dir}/${target}.elf ${hex_map_dir}/app.elf
    COMMAND ${CMAKE_COMMAND} -E copy ${out_hex_dir}/${target}.map ${hex_map_dir}/app.map
    COMMAND ${CMAKE_COMMAND} -E copy ${pac_file_with_core} ${pac_file_with_version}
    COMMAND python3 ${gdbComGen_py} ${CMAKE_CURRENT_SOURCE_DIR} /hex/${target}_map/app.elf
	DEPENDS ${pac_file} ${pac_file_with_core} 
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
	

#set(hex_fota_file ${CSDK_PRO}_${CSDK_VER}_${core_vername}.dfota.bin)
#
#if (${FOTA_TYPE} STREQUAL  "all")
#	add_custom_command(OUTPUT ${hex_fota_file}
#		COMMAND ${CMAKE_COMMAND} -E copy ${out_hex_dir}/${target}.img ${target}.img
#		COMMAND ${CMAKE_COMMAND} -E copy ${core_bin_dir}/${core_name}.pac  ${core_name}.pac
#		COMMAND ${CMAKE_COMMAND} -E rename ${target}.img app.bin
#		COMMAND ${CMAKE_COMMAND} -E rename ${core_name}.pac ${core_name}.bin
#		COMMAND ${CMAKE_COMMAND} -E tar "cfv" "${hex_bin_dir}.zip" "--format=zip" "${core_name}.bin" "app.bin"
#		COMMAND ${CMAKE_COMMAND} -E rename ${hex_bin_dir}.zip ${hex_fota_file}
#		COMMAND ${CMAKE_COMMAND} -E remove [-f] ${core_name}.bin app.bin
#		DEPENDS ${out_hex_dir}/${target}.img ${hex_app_file} ${hex_app_with_core_file}
#		WORKING_DIRECTORY ${hex_bin_dir})
#		add_custom_target(${target}_release ALL DEPENDS ${hex_app_file} ${hex_app_with_core_file} ${hex_map_file} ${hex_elf_file} ${hex_fota_file})
#endif()
#
#if (${FOTA_TYPE} STREQUAL  "core")
#	add_custom_command(OUTPUT ${hex_fota_file}
#	COMMAND ${CMAKE_COMMAND} -E copy ${core_bin_dir}/${core_name}.pac  ${core_name}.pac
#	COMMAND ${CMAKE_COMMAND} -E rename ${core_name}.pac ${core_name}.bin
#	COMMAND ${CMAKE_COMMAND} -E tar "cfv" "${hex_bin_dir}.zip" "--format=zip" "${core_name}.bin" 
#	COMMAND ${CMAKE_COMMAND} -E rename ${hex_bin_dir}.zip ${hex_fota_file}
#	COMMAND ${CMAKE_COMMAND} -E remove [-f] ${core_name}.bin
#	DEPENDS ${out_hex_dir}/${target}.img ${hex_app_file} ${hex_app_with_core_file}
#	WORKING_DIRECTORY ${hex_bin_dir})
#	add_custom_target(${target}_release ALL DEPENDS ${hex_app_file} ${hex_app_with_core_file} ${hex_map_file} ${hex_elf_file} ${hex_fota_file})
#
#endif()
#
#if (${FOTA_TYPE} STREQUAL  "app")
#	add_custom_command(OUTPUT ${hex_fota_file}
#	COMMAND ${CMAKE_COMMAND} -E copy ${out_hex_dir}/${target}.img ${target}.img
#	COMMAND ${CMAKE_COMMAND} -E rename ${target}.img app.bin
#	COMMAND ${CMAKE_COMMAND} -E tar "cfv" "${hex_bin_dir}.zip" "--format=zip" "app.bin" 
#	COMMAND ${CMAKE_COMMAND} -E rename ${hex_bin_dir}.zip ${hex_fota_file}
#	COMMAND ${CMAKE_COMMAND} -E remove [-f] app.bin
#	DEPENDS ${out_hex_dir}/${target}.img ${hex_app_file} ${hex_app_with_core_file}
#	WORKING_DIRECTORY ${hex_bin_dir})
#	add_custom_target(${target}_release ALL DEPENDS ${hex_app_file} ${hex_app_with_core_file} ${hex_map_file} ${hex_elf_file} ${hex_fota_file})
#endif()
#
add_custom_target(${target}_release ALL DEPENDS ${hex_app_file} ${hex_app_with_core_file} ${hex_map_file} ${hex_elf_file})

#######################################################
#                   Micropython part
#######################################################

# Set the board directory and check that it exists.
if(NOT MICROPY_BOARD_DIR)
    set(MICROPY_BOARD_DIR ${CMAKE_CURRENT_LIST_DIR}/boards/${MICROPY_BOARD})
endif()

if(NOT EXISTS ${MICROPY_BOARD_DIR}/mpconfigboard.h)
    message(FATAL_ERROR "Invalid MICROPY_BOARD specified: ${MICROPY_BOARD}")
endif()

# If a board variant is specified, check that it exists.
#if(MICROPY_BOARD_VARIANT)
#    if(NOT EXISTS ${MICROPY_BOARD_DIR}/mpconfigvariant_${MICROPY_BOARD_VARIANT}.cmake)
#        message(FATAL_ERROR "Invalid MICROPY_BOARD_VARIANT specified: ${MICROPY_BOARD_VARIANT}")
#    endif()
#endif()

# Save the manifest file set from the cmake command line.
set(MICROPY_USER_FROZEN_MANIFEST ${MICROPY_FROZEN_MANIFEST})

# Include board config; this is expected to set (among other options):
# - SDKCONFIG_DEFAULTS
# - IDF_TARGET
#include(${MICROPY_BOARD_DIR}/mpconfigboard.cmake)
#if(NOT MICROPY_BOARD_VARIANT)
#    include(${MICROPY_BOARD_DIR}/mpconfigvariant.cmake OPTIONAL)
#else()
#    include(${MICROPY_BOARD_DIR}/mpconfigvariant_${MICROPY_BOARD_VARIANT}.cmake)
#endif()

# Set the frozen manifest file. Note if MICROPY_FROZEN_MANIFEST is set from the cmake
# command line, then it will override the default and any manifest set by the board.
if (MICROPY_USER_FROZEN_MANIFEST)
    set(MICROPY_FROZEN_MANIFEST ${MICROPY_USER_FROZEN_MANIFEST})
elseif (NOT MICROPY_FROZEN_MANIFEST)
    set(MICROPY_FROZEN_MANIFEST ${CMAKE_CURRENT_LIST_DIR}/boards/manifest.py)
endif()

include_directories(${MICROPY_BOARD_DIR} ${CMAKE_CURRENT_BINARY_DIR})

# Set location of base MicroPython directory.
if(NOT MICROPY_DIR)
    get_filename_component(MICROPY_DIR ${CMAKE_CURRENT_LIST_DIR}/../.. ABSOLUTE)
endif()

# Set location of the Air724 port directory.
if(NOT MICROPY_PORT_DIR)
    get_filename_component(MICROPY_PORT_DIR ${MICROPY_DIR}/ports/air724 ABSOLUTE)
endif()

# RISC-V specific inclusions
list(APPEND MICROPY_SOURCE_LIB
	${MICROPY_DIR}/shared/runtime/gchelper_native.c
    ${MICROPY_DIR}/shared/runtime/gchelper_thumb2.s
)

# Enable error text compression by default.
if(NOT MICROPY_ROM_TEXT_COMPRESSION)
    set(MICROPY_ROM_TEXT_COMPRESSION ON)
endif()

# Include core source components.
include(${MICROPY_DIR}/py/py.cmake)
include(${MICROPY_DIR}/py/usermod.cmake)
include(${MICROPY_DIR}/extmod/extmod.cmake)

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${MICROPY_DIR} ${MICROPY_BOARD_DIR}) # ${CMAKE_CURRENT_SOURCE_DIR}/boards/Air724_GENERIC # ${MICROPY_BOARD_DIR}

#list(APPEND MICROPY_QSTRDEFS_PORT
#    ${MICROPY_PORT_DIR}/qstrdefsport.h
#)

list(APPEND MICROPY_SOURCE_SHARED
    ${MICROPY_DIR}/shared/readline/readline.c
    ${MICROPY_DIR}/shared/netutils/netutils.c
    ${MICROPY_DIR}/shared/timeutils/timeutils.c
    ${MICROPY_DIR}/shared/runtime/interrupt_char.c
    ${MICROPY_DIR}/shared/runtime/mpirq.c
    ${MICROPY_DIR}/shared/runtime/stdout_helpers.c
    ${MICROPY_DIR}/shared/runtime/sys_stdio_mphal.c
    ${MICROPY_DIR}/shared/runtime/pyexec.c
)

#list(APPEND MICROPY_SOURCE_LIB
#    ${MICROPY_DIR}/lib/littlefs/lfs1.c
#    ${MICROPY_DIR}/lib/littlefs/lfs1_util.c
#    ${MICROPY_DIR}/lib/littlefs/lfs2.c
#    ${MICROPY_DIR}/lib/littlefs/lfs2_util.c
#    ${MICROPY_DIR}/lib/mbedtls_errors/esp32_mbedtls_errors.c
#    ${MICROPY_DIR}/lib/oofatfs/ff.c
#    ${MICROPY_DIR}/lib/oofatfs/ffunicode.c
#)

#list(APPEND MICROPY_SOURCE_DRIVERS
#    ${MICROPY_DIR}/drivers/bus/softspi.c
#    ${MICROPY_DIR}/drivers/dht/dht.c
#)

#if(MICROPY_PY_TINYUSB)
#    string(TOUPPER OPT_MCU_${IDF_TARGET} tusb_mcu)
#
#    list(APPEND MICROPY_DEF_TINYUSB
#        CFG_TUSB_MCU=${tusb_mcu}
#    )
#
#    list(APPEND MICROPY_SOURCE_TINYUSB
#        ${MICROPY_DIR}/shared/tinyusb/mp_usbd.c
#        ${MICROPY_DIR}/shared/tinyusb/mp_usbd_cdc.c
#        ${MICROPY_DIR}/shared/tinyusb/mp_usbd_descriptor.c
#        ${MICROPY_DIR}/shared/tinyusb/mp_usbd_runtime.c
#    )
#
#    list(APPEND MICROPY_INC_TINYUSB
#        ${MICROPY_DIR}/shared/tinyusb/
#    )
#endif()

list(APPEND MICROPY_SOURCE_PORT
    main.c
    gccollect.c 
    modair.c 
    mphalport.c
)

list(TRANSFORM MICROPY_SOURCE_PORT PREPEND ${MICROPY_PORT_DIR}/)
#list(APPEND MICROPY_SOURCE_PORT ${CMAKE_BINARY_DIR}/pins.c)

list(APPEND MICROPY_SOURCE_QSTR
    ${MICROPY_SOURCE_PY}
    ${MICROPY_SOURCE_EXTMOD}
    ${MICROPY_SOURCE_USERMOD}
    ${MICROPY_SOURCE_SHARED}
    ${MICROPY_SOURCE_LIB}
    ${MICROPY_SOURCE_PORT}
#    ${MICROPY_SOURCE_BOARD}
#    ${MICROPY_SOURCE_TINYUSB}
)

# Provide the default LD fragment if not set
if (MICROPY_USER_LDFRAGMENTS)
    set(MICROPY_LDFRAGMENTS ${MICROPY_USER_LDFRAGMENTS})
endif()

#if (UPDATE_SUBMODULES)
#    # ESP-IDF checks if some paths exist before CMake does. Some paths don't
#    # yet exist if this is an UPDATE_SUBMODULES pass on a brand new checkout, so remove
#    # any path which might not exist yet. A "real" build will not set UPDATE_SUBMODULES.
#    unset(MICROPY_SOURCE_TINYUSB)
#    unset(MICROPY_SOURCE_EXTMOD)
#    unset(MICROPY_SOURCE_LIB)
#    unset(MICROPY_INC_TINYUSB)
#    unset(MICROPY_INC_CORE)
#endif()

# Set the MicroPython target as the current target.
set(MICROPY_TARGET ${BUILD_TARGET})

# Define mpy-cross flags, for use with frozen code.
#if(CONFIG_IDF_TARGET_ARCH_XTENSA)
#    set(MICROPY_CROSS_FLAGS -march=xtensawin)
#elseif(CONFIG_IDF_TARGET_ARCH_RISCV)
#    set(MICROPY_CROSS_FLAGS -march=rv32imc)
#endif()

# Set compile options for this port.
target_compile_definitions(${MICROPY_TARGET} PUBLIC
#    ${MICROPY_DEF_COMPONENT}
#    ${MICROPY_DEF_CORE}
#    ${MICROPY_DEF_BOARD}
#    ${MICROPY_DEF_TINYUSB}
#    MICROPY_VFS_FAT=1
#    MICROPY_VFS_LFS2=1
     "FFCONF_H=\"${MICROPY_OOFATFS_DIR}/ffconf.h\""
#    LFS1_NO_MALLOC LFS1_NO_DEBUG LFS1_NO_WARN LFS1_NO_ERROR LFS1_NO_ASSERT
#    LFS2_NO_MALLOC LFS2_NO_DEBUG LFS2_NO_WARN LFS2_NO_ERROR LFS2_NO_ASSERT
)

# Disable some warnings to keep the build output clean.
# target_compile_options(${MICROPY_TARGET} PUBLIC
#    ${MICROPY_COMPILE_COMPONENT}
#    -Wno-clobbered
#    -Wno-deprecated-declarations
#    -Wno-missing-field-initializers
#)

# Additional include directories needed for private NimBLE headers.
#target_include_directories(${MICROPY_TARGET} PUBLIC
#    ${IDF_PATH}/components/bt/host/nimble/nimble
#)

# Add additional extmod and usermod components.
#if (MICROPY_PY_BTREE)
#    target_link_libraries(${MICROPY_TARGET} micropy_extmod_btree)
#endif()
# target_link_libraries(${MICROPY_TARGET} usermod)

# Extra linker options
# (when wrap symbols are in standalone files, --undefined ensures
# the linker doesn't skip that file.)
target_link_options(${MICROPY_TARGET} PUBLIC
  # Patch LWIP memory pool allocators (see lwip_patch.c)
  -Wl,--undefined=memp_malloc
  -Wl,--wrap=memp_malloc
  -Wl,--wrap=memp_free

  # Enable the panic handler wrapper
  -Wl,--undefined=esp_panic_handler
  -Wl,--wrap=esp_panic_handler
)

# Include the main MicroPython cmake rules.
####################################################################################
#include(${MICROPY_DIR}/py/mkrules.cmake)
####################################################################################
# CMake fragment for MicroPython rules

set(MICROPY_GENHDR_DIR "${CMAKE_BINARY_DIR}/genhdr")
set(MICROPY_MPVERSION "${MICROPY_GENHDR_DIR}/mpversion.h")
set(MICROPY_QSTRDEFS_PY "${MICROPY_PY_DIR}/qstrdefs.h")
set(MICROPY_QSTRDEFS_LAST "${MICROPY_GENHDR_DIR}/qstr.i.last")
set(MICROPY_QSTRDEFS_SPLIT "${MICROPY_GENHDR_DIR}/qstr.split")
set(MICROPY_QSTRDEFS_COLLECTED "${MICROPY_GENHDR_DIR}/qstrdefs.collected.h")
set(MICROPY_QSTRDEFS_PREPROCESSED "${MICROPY_GENHDR_DIR}/qstrdefs.preprocessed.h")
set(MICROPY_QSTRDEFS_GENERATED "${MICROPY_GENHDR_DIR}/qstrdefs.generated.h")
set(MICROPY_MODULEDEFS_SPLIT "${MICROPY_GENHDR_DIR}/moduledefs.split")
set(MICROPY_MODULEDEFS_COLLECTED "${MICROPY_GENHDR_DIR}/moduledefs.collected")
set(MICROPY_MODULEDEFS "${MICROPY_GENHDR_DIR}/moduledefs.h")
set(MICROPY_ROOT_POINTERS_SPLIT "${MICROPY_GENHDR_DIR}/root_pointers.split")
set(MICROPY_ROOT_POINTERS_COLLECTED "${MICROPY_GENHDR_DIR}/root_pointers.collected")
set(MICROPY_ROOT_POINTERS "${MICROPY_GENHDR_DIR}/root_pointers.h")
set(MICROPY_COMPRESSED_SPLIT "${MICROPY_GENHDR_DIR}/compressed.split")
set(MICROPY_COMPRESSED_COLLECTED "${MICROPY_GENHDR_DIR}/compressed.collected")
set(MICROPY_COMPRESSED_DATA "${MICROPY_GENHDR_DIR}/compressed.data.h")

if(NOT MICROPY_PREVIEW_VERSION_2)
    set(MICROPY_PREVIEW_VERSION_2 0)
endif()

# Set the board name.
if(MICROPY_BOARD)
    if(MICROPY_BOARD_VARIANT)
        set(MICROPY_BOARD_BUILD_NAME ${MICROPY_BOARD}-${MICROPY_BOARD_VARIANT})
    else()
        set(MICROPY_BOARD_BUILD_NAME ${MICROPY_BOARD})
    endif()

    target_compile_definitions(${MICROPY_TARGET} PRIVATE
        MICROPY_BOARD_BUILD_NAME="${MICROPY_BOARD_BUILD_NAME}"
    )
endif()

# Need to do this before extracting MICROPY_CPP_DEF below.
if(MICROPY_ROM_TEXT_COMPRESSION)
    target_compile_definitions(${MICROPY_TARGET} PUBLIC
        MICROPY_ROM_TEXT_COMPRESSION=\(1\)
    )
endif()

# Need to do this before extracting MICROPY_CPP_DEF below. Rest of frozen
# manifest handling is at the end of this file.
if(MICROPY_FROZEN_MANIFEST)
    target_compile_definitions(${MICROPY_TARGET} PUBLIC
        MICROPY_QSTR_EXTRA_POOL=mp_qstr_frozen_const_pool
        MICROPY_MODULE_FROZEN_MPY=\(1\)
    )
endif()

if(MICROPY_PREVIEW_VERSION_2)
    target_compile_definitions(${MICROPY_TARGET} PUBLIC
        MICROPY_PREVIEW_VERSION_2=\(1\)
    )
endif()

# Provide defaults for preprocessor flags if not already defined
if(NOT MICROPY_CPP_FLAGS)
    get_target_property(MICROPY_CPP_INC ${MICROPY_TARGET} INCLUDE_DIRECTORIES)
    get_target_property(MICROPY_CPP_DEF ${MICROPY_TARGET} COMPILE_DEFINITIONS)
endif()

# Compute MICROPY_CPP_FLAGS for preprocessor
list(APPEND MICROPY_CPP_INC ${MICROPY_CPP_INC_EXTRA})
list(APPEND MICROPY_CPP_DEF ${MICROPY_CPP_DEF_EXTRA})
set(_prefix "-I")
foreach(_arg ${MICROPY_CPP_INC})
    list(APPEND MICROPY_CPP_FLAGS ${_prefix}${_arg})
endforeach()
set(_prefix "-D")
foreach(_arg ${MICROPY_CPP_DEF})
    list(APPEND MICROPY_CPP_FLAGS ${_prefix}${_arg})
endforeach()
list(APPEND MICROPY_CPP_FLAGS ${MICROPY_CPP_FLAGS_EXTRA})

# Include anything passed in via CFLAGS_EXTRA
# in both MICROPY_CPP_FLAGS and CMAKE_C_FLAGS
if(DEFINED ENV{CFLAGS_EXTRA})
  set(CFLAGS_EXTRA $ENV{CFLAGS_EXTRA})
  string(APPEND CMAKE_C_FLAGS " ${CFLAGS_EXTRA}")  # ... not a list
  separate_arguments(CFLAGS_EXTRA)
  list(APPEND MICROPY_CPP_FLAGS ${CFLAGS_EXTRA})  # ... a list
endif()

find_package(Python3 REQUIRED COMPONENTS Interpreter)

target_sources(${MICROPY_TARGET} PRIVATE
    ${MICROPY_MPVERSION}
    ${MICROPY_QSTRDEFS_GENERATED}
    ${MICROPY_MODULEDEFS}
    ${MICROPY_ROOT_POINTERS}
)

if(MICROPY_ROM_TEXT_COMPRESSION)
    target_sources(${MICROPY_TARGET} PRIVATE
        ${MICROPY_COMPRESSED_DATA}
    )
endif()

# Command to force the build of another command

# Generate mpversion.h

add_custom_target(
    BUILD_VERSION_HEADER ALL
    BYPRODUCTS ${MICROPY_MPVERSION}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${MICROPY_GENHDR_DIR}
    COMMAND ${Python3_EXECUTABLE} ${MICROPY_DIR}/py/makeversionhdr.py ${MICROPY_MPVERSION}
)

# Generate qstrs

# If any of the dependencies in this rule change then the C-preprocessor step must be run.
# It only needs to be passed the list of MICROPY_SOURCE_QSTR files that have changed since
# it was last run, but it looks like it's not possible to specify that with cmake.
add_custom_command(
    OUTPUT ${MICROPY_QSTRDEFS_LAST}
    COMMAND ${Python3_EXECUTABLE} ${MICROPY_PY_DIR}/makeqstrdefs.py pp ${CMAKE_C_COMPILER} -E output ${MICROPY_GENHDR_DIR}/qstr.i.last cflags ${MICROPY_CPP_FLAGS} -DNO_QSTR cxxflags ${MICROPY_CPP_FLAGS} -DNO_QSTR sources ${MICROPY_SOURCE_QSTR}
    DEPENDS ${MICROPY_MPVERSION}
        ${MICROPY_SOURCE_QSTR}
    VERBATIM
    COMMAND_EXPAND_LISTS
)

add_custom_command(
    OUTPUT ${MICROPY_QSTRDEFS_SPLIT}
    COMMAND ${Python3_EXECUTABLE} ${MICROPY_PY_DIR}/makeqstrdefs.py split qstr ${MICROPY_GENHDR_DIR}/qstr.i.last ${MICROPY_GENHDR_DIR}/qstr _
    COMMAND touch ${MICROPY_QSTRDEFS_SPLIT}
    DEPENDS ${MICROPY_QSTRDEFS_LAST}
    VERBATIM
    COMMAND_EXPAND_LISTS
)

add_custom_command(
    OUTPUT ${MICROPY_QSTRDEFS_COLLECTED}
    COMMAND ${Python3_EXECUTABLE} ${MICROPY_PY_DIR}/makeqstrdefs.py cat qstr _ ${MICROPY_GENHDR_DIR}/qstr ${MICROPY_QSTRDEFS_COLLECTED}
    BYPRODUCTS "${MICROPY_QSTRDEFS_COLLECTED}.hash"
    DEPENDS ${MICROPY_QSTRDEFS_SPLIT}
    VERBATIM
    COMMAND_EXPAND_LISTS
)

# workaround SED quoted strings issue under CMD /C in Windows
# also remove pipes routing via several steps with file creating

set(QSTR_QUOTED "${GENHDR_DIR}/qstr_quoted.tmp")
set(QSTR_PREPROC "${GENHDR_DIR}/qstr_preproc.tmp")

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/genhdr/qstr_step1_cat.tmp
    COMMAND cat
        ${MICROPY_QSTRDEFS_PY}
#        ${MICROPY_QSTRDEFS_PORT}
        ${MICROPY_QSTRDEFS_COLLECTED}
        > ${CMAKE_CURRENT_BINARY_DIR}/genhdr/qstr_step1_cat.tmp
    DEPENDS
        ${MICROPY_QSTRDEFS_PY}
#        ${MICROPY_QSTRDEFS_PORT}
        ${MICROPY_QSTRDEFS_COLLECTED}
    VERBATIM
)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/genhdr/qstr_step2_quoted.tmp
    COMMAND sh -c "sed 's/^Q(.*)/\\\\x22&\\\\x22/' ${CMAKE_CURRENT_BINARY_DIR}/genhdr/qstr_step1_cat.tmp > ${CMAKE_CURRENT_BINARY_DIR}/genhdr/qstr_step2_quoted.tmp"
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/genhdr/qstr_step1_cat.tmp
    VERBATIM
)

set(FIX_SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/genhdr/fix_qstr_quotes.cmake")
file(WRITE ${FIX_SCRIPT}
    "file(READ \"${CMAKE_CURRENT_BINARY_DIR}/genhdr/qstr_step2_quoted.tmp\" CONTENT)\n"
    "string(REPLACE \"\\\\x22\" \"\\\"\" CONTENT \"\${CONTENT}\")\n"
    "file(WRITE \"${CMAKE_CURRENT_BINARY_DIR}/genhdr/qstr_step2_fixed.tmp\" \"\${CONTENT}\")\n"
)
set(FIX_SCRIPT_REV "${CMAKE_CURRENT_BINARY_DIR}/genhdr/fix_qstr_remove_quotes.cmake")
file(WRITE ${FIX_SCRIPT_REV}
    "file(READ \"${CMAKE_CURRENT_BINARY_DIR}/genhdr/qstr_step3_preproc.tmp\" CONTENT)\n"
    "string(REPLACE \"\\\"\" \"\" CONTENT \"\${CONTENT}\")\n"
    "file(WRITE \"${MICROPY_QSTRDEFS_PREPROCESSED}\" \"\${CONTENT}\")\n"
)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/genhdr/qstr_step2_fixed.tmp
    COMMAND ${CMAKE_COMMAND} -P ${FIX_SCRIPT}
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/genhdr/qstr_step2_quoted.tmp
    VERBATIM
)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/genhdr/qstr_step3_preproc.tmp
    COMMAND ${CMAKE_C_COMPILER}
        -x c
        -E ${MICROPY_CPP_FLAGS}
        ${CMAKE_CURRENT_BINARY_DIR}/genhdr/qstr_step2_fixed.tmp
        -o ${CMAKE_CURRENT_BINARY_DIR}/genhdr/qstr_step3_preproc.tmp
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/genhdr/qstr_step2_fixed.tmp
    VERBATIM
)

add_custom_command(
    OUTPUT ${MICROPY_QSTRDEFS_PREPROCESSED}
    COMMAND ${CMAKE_COMMAND} -P ${FIX_SCRIPT_REV}
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/genhdr/qstr_step3_preproc.tmp
    VERBATIM
)

add_custom_command(
    OUTPUT ${MICROPY_QSTRDEFS_GENERATED}
    COMMAND ${Python3_EXECUTABLE} ${MICROPY_PY_DIR}/makeqstrdata.py ${MICROPY_QSTRDEFS_PREPROCESSED} > ${MICROPY_QSTRDEFS_GENERATED}
    DEPENDS ${MICROPY_QSTRDEFS_PREPROCESSED}
    VERBATIM
    COMMAND_EXPAND_LISTS
)

# Generate moduledefs.h

add_custom_command(
    OUTPUT ${MICROPY_MODULEDEFS_SPLIT}
    COMMAND ${Python3_EXECUTABLE} ${MICROPY_PY_DIR}/makeqstrdefs.py split module ${MICROPY_GENHDR_DIR}/qstr.i.last ${MICROPY_GENHDR_DIR}/module _
    COMMAND touch ${MICROPY_MODULEDEFS_SPLIT}
    DEPENDS ${MICROPY_QSTRDEFS_LAST}
    VERBATIM
    COMMAND_EXPAND_LISTS
)

add_custom_command(
    OUTPUT ${MICROPY_MODULEDEFS_COLLECTED}
    COMMAND ${Python3_EXECUTABLE} ${MICROPY_PY_DIR}/makeqstrdefs.py cat module _ ${MICROPY_GENHDR_DIR}/module ${MICROPY_MODULEDEFS_COLLECTED}
    BYPRODUCTS "${MICROPY_MODULEDEFS_COLLECTED}.hash"
    DEPENDS ${MICROPY_MODULEDEFS_SPLIT}
    VERBATIM
    COMMAND_EXPAND_LISTS
)

add_custom_command(
    OUTPUT ${MICROPY_MODULEDEFS}
    COMMAND ${Python3_EXECUTABLE} ${MICROPY_PY_DIR}/makemoduledefs.py ${MICROPY_MODULEDEFS_COLLECTED} > ${MICROPY_MODULEDEFS}
    DEPENDS ${MICROPY_MODULEDEFS_COLLECTED}
)

# Generate root_pointers.h

add_custom_command(
    OUTPUT ${MICROPY_ROOT_POINTERS_SPLIT}
    COMMAND ${Python3_EXECUTABLE} ${MICROPY_PY_DIR}/makeqstrdefs.py split root_pointer ${MICROPY_GENHDR_DIR}/qstr.i.last ${MICROPY_GENHDR_DIR}/root_pointer _
    COMMAND touch ${MICROPY_ROOT_POINTERS_SPLIT}
    DEPENDS ${MICROPY_QSTRDEFS_LAST}
    VERBATIM
    COMMAND_EXPAND_LISTS
)

add_custom_command(
    OUTPUT ${MICROPY_ROOT_POINTERS_COLLECTED}
    COMMAND ${Python3_EXECUTABLE} ${MICROPY_PY_DIR}/makeqstrdefs.py cat root_pointer _ ${MICROPY_GENHDR_DIR}/root_pointer ${MICROPY_ROOT_POINTERS_COLLECTED}
    BYPRODUCTS "${MICROPY_ROOT_POINTERS_COLLECTED}.hash"
    DEPENDS ${MICROPY_ROOT_POINTERS_SPLIT}
    VERBATIM
    COMMAND_EXPAND_LISTS
)

add_custom_command(
    OUTPUT ${MICROPY_ROOT_POINTERS}
    COMMAND ${Python3_EXECUTABLE} ${MICROPY_PY_DIR}/make_root_pointers.py ${MICROPY_ROOT_POINTERS_COLLECTED} > ${MICROPY_ROOT_POINTERS}
    DEPENDS ${MICROPY_ROOT_POINTERS_COLLECTED} ${MICROPY_PY_DIR}/make_root_pointers.py
)

# Generate compressed.data.h

add_custom_command(
    OUTPUT ${MICROPY_COMPRESSED_SPLIT}
    COMMAND ${Python3_EXECUTABLE} ${MICROPY_PY_DIR}/makeqstrdefs.py split compress ${MICROPY_QSTRDEFS_LAST} ${MICROPY_GENHDR_DIR}/compress _
    COMMAND touch ${MICROPY_COMPRESSED_SPLIT}
    DEPENDS ${MICROPY_QSTRDEFS_LAST}
    VERBATIM
    COMMAND_EXPAND_LISTS
)

add_custom_command(
    OUTPUT ${MICROPY_COMPRESSED_COLLECTED}
    COMMAND ${Python3_EXECUTABLE} ${MICROPY_PY_DIR}/makeqstrdefs.py cat compress _ ${MICROPY_GENHDR_DIR}/compress ${MICROPY_COMPRESSED_COLLECTED}
    BYPRODUCTS "${MICROPY_COMPRESSED_COLLECTED}.hash"
    DEPENDS ${MICROPY_COMPRESSED_SPLIT}
    VERBATIM
    COMMAND_EXPAND_LISTS
)

add_custom_command(
    OUTPUT ${MICROPY_COMPRESSED_DATA}
    COMMAND ${Python3_EXECUTABLE} ${MICROPY_PY_DIR}/makecompresseddata.py ${MICROPY_COMPRESSED_COLLECTED} > ${MICROPY_COMPRESSED_DATA}
    DEPENDS ${MICROPY_COMPRESSED_COLLECTED} ${MICROPY_PY_DIR}/makecompresseddata.py
)

# Build frozen code if enabled

if(MICROPY_FROZEN_MANIFEST)
    set(MICROPY_FROZEN_CONTENT "${CMAKE_BINARY_DIR}/frozen_content.c")

    target_sources(${MICROPY_TARGET} PRIVATE
        ${MICROPY_FROZEN_CONTENT}
    )

    # Note: target_compile_definitions already added earlier.

    if(NOT MICROPY_LIB_DIR)
        list(APPEND GIT_SUBMODULES lib/micropython-lib)
        set(MICROPY_LIB_DIR ${MICROPY_DIR}/lib/micropython-lib)
    endif()

    if(NOT UPDATE_SUBMODULES AND NOT EXISTS ${MICROPY_LIB_DIR}/README.md)
        message(FATAL_ERROR " micropython-lib not initialized.\n Run 'make BOARD=${MICROPY_BOARD} submodules'")
    endif()

    # If MICROPY_MPYCROSS is not explicitly defined in the environment (which
    # is what makemanifest.py will use) then create an mpy-cross dependency
    # to automatically build mpy-cross if needed.
    set(MICROPY_MPYCROSS $ENV{MICROPY_MPYCROSS})
    if(NOT MICROPY_MPYCROSS)
        set(MICROPY_MPYCROSS_DEPENDENCY ${MICROPY_DIR}/mpy-cross/build/mpy-cross)
        if(NOT MICROPY_MAKE_EXECUTABLE)
            set(MICROPY_MAKE_EXECUTABLE make)
        endif()
        add_custom_command(
            OUTPUT ${MICROPY_MPYCROSS_DEPENDENCY}
            COMMAND ${MICROPY_MAKE_EXECUTABLE} -C ${MICROPY_DIR}/mpy-cross USER_C_MODULES=
        )
    endif()

    if(NOT MICROPY_CROSS_FLAGS)
        set(MICROPY_CROSS_FLAGS "")
    else()
        set(MICROPY_CROSS_FLAGS "-f${MICROPY_CROSS_FLAGS}")
    endif()

    # Set default path variables to be passed to makemanifest.py. These will
    # be available in path substitutions. Additional variables can be set
    # per-board in mpconfigboard.cmake.
    set(MICROPY_MANIFEST_PORT_DIR ${MICROPY_PORT_DIR})
    set(MICROPY_MANIFEST_BOARD_DIR ${MICROPY_BOARD_DIR})
    set(MICROPY_MANIFEST_MPY_DIR ${MICROPY_DIR})
    set(MICROPY_MANIFEST_MPY_LIB_DIR ${MICROPY_LIB_DIR})

    # Find all MICROPY_MANIFEST_* variables and turn them into command line arguments.
    get_cmake_property(_manifest_vars VARIABLES)
    list(FILTER _manifest_vars INCLUDE REGEX "MICROPY_MANIFEST_.*")
    foreach(_manifest_var IN LISTS _manifest_vars)
        list(APPEND _manifest_var_args "-v")
        string(REGEX REPLACE "MICROPY_MANIFEST_(.*)" "\\1" _manifest_var_name ${_manifest_var})
        list(APPEND _manifest_var_args "${_manifest_var_name}=${${_manifest_var}}")
    endforeach()

    add_custom_target(
        BUILD_FROZEN_CONTENT ALL
        BYPRODUCTS ${MICROPY_FROZEN_CONTENT}
        COMMAND ${Python3_EXECUTABLE} ${MICROPY_DIR}/tools/makemanifest.py -o ${MICROPY_FROZEN_CONTENT} ${_manifest_var_args} -b "${CMAKE_BINARY_DIR}" ${MICROPY_CROSS_FLAGS} --mpy-tool-flags=${MICROPY_MPY_TOOL_FLAGS} ${MICROPY_FROZEN_MANIFEST}
        DEPENDS
            ${MICROPY_QSTRDEFS_GENERATED}
            ${MICROPY_ROOT_POINTERS}
            ${MICROPY_MPYCROSS_DEPENDENCY}
        VERBATIM
        #WORKING_DIRECTORY ${MICROPY_DIR}/tools
    )
endif()

# Update submodules, this is invoked on some ports via 'make submodules'.
#
# Note: This logic has a Makefile equivalent in py/mkrules.mk
if(UPDATE_SUBMODULES AND GIT_SUBMODULES)
    macro(run_git)
      execute_process(COMMAND git ${ARGV} WORKING_DIRECTORY ${MICROPY_DIR}
          RESULT_VARIABLE RES)
    endmacro()

    list(JOIN GIT_SUBMODULES " " GIT_SUBMODULES_MSG)
    message("Updating submodules: ${GIT_SUBMODULES_MSG}")
    run_git(submodule sync ${GIT_SUBMODULES})
    if(RES EQUAL 0)
        # If available, do blobless partial clones of submodules to save time and space.
        # A blobless partial clone lazily fetches data as needed, but has all the metadata available (tags, etc.).
        run_git(submodule update --init --filter=blob:none ${GIT_SUBMODULES})
        # Fallback to standard submodule update if blobless isn't available (earlier than git 2.36.0)
        if (NOT RES EQUAL 0)
            run_git(submodule update --init ${GIT_SUBMODULES})
        endif()
    endif()

    if (NOT RES EQUAL 0)
        message(FATAL_ERROR "Submodule update failed")
    endif()
endif()

####################################################################################

# Generate source files for named pins (requires mkrules.cmake for MICROPY_GENHDR_DIR).

#set(GEN_PINS_PREFIX "${MICROPY_PORT_DIR}/boards/pins_prefix.c")
#set(GEN_PINS_MKPINS "${MICROPY_PORT_DIR}/boards/make-pins.py")
#set(GEN_PINS_SRC "${CMAKE_BINARY_DIR}/pins.c")
#set(GEN_PINS_HDR "${MICROPY_GENHDR_DIR}/pins.h")

#if(EXISTS "${MICROPY_BOARD_DIR}/pins.csv")
#    set(GEN_PINS_BOARD_CSV "${MICROPY_BOARD_DIR}/pins.csv")
#    set(GEN_PINS_BOARD_CSV_ARG --board-csv "${GEN_PINS_BOARD_CSV}")
#endif()

#target_sources(${MICROPY_TARGET} PRIVATE ${GEN_PINS_HDR})
target_sources(${MICROPY_TARGET} PRIVATE ${MICROPY_SOURCE_QSTR})

#add_custom_command(
#    OUTPUT ${GEN_PINS_SRC} ${GEN_PINS_HDR}
#    COMMAND ${Python3_EXECUTABLE} ${GEN_PINS_MKPINS} ${GEN_PINS_BOARD_CSV_ARG}
#        --prefix ${GEN_PINS_PREFIX} --output-source ${GEN_PINS_SRC} --output-header ${GEN_PINS_HDR}
#    DEPENDS
#        ${MICROPY_MPVERSION}
#        ${GEN_PINS_MKPINS}
#        ${GEN_PINS_BOARD_CSV}
#        ${GEN_PINS_PREFIX}
#    VERBATIM
#    COMMAND_EXPAND_LISTS
#)


project(micropython C CXX ASM)

add_subdirectory(${SOURCE_TOP_DIR}/components/ril ${CMAKE_CURRENT_BINARY_DIR}/components/ril)
add_subdirectory(${SOURCE_TOP_DIR}/api ${CMAKE_CURRENT_BINARY_DIR}/api)

