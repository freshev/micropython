#!/bin/bash
set -e

VERSION="$1"
VERSION="${VERSION//\"/}"
ZIP_COMPRESS="$2"
ZIP_COMPRESS="${ZIP_COMPRESS//\"/}"
WWW_PATH="$3"
WWW_PATH="${WWW_PATH//\"/}"

CFG_RELEASE=debug
if [[ "$1xx" == "releasexx" ]]; then
    CFG_RELEASE=release
fi

######################## 1 ################################
# check path
PATH_CURR=$(cd `dirname $0`; pwd)
FOLDER_NAME="${PATH_CURR##*/}"
PATH_MICROPY=${PATH_CURR/\/ports\/${FOLDER_NAME}/}
CSDK_PATH=${PATH_MICROPY}/lib/GPRS_C_SDK

FIRMWARE_NAME=firmware_${CFG_RELEASE}
BIN_PATH=${PATH_CURR}/hex
FOTA_PATH=${PATH_CURR}/fota
VERSION_PATH=${PATH_CURR}/version
LODBASE=${BIN_PATH}/${FIRMWARE_NAME}_
LOD_FILE=${LODBASE}flash.lod
WITH_PLT_LOD_FILE=${VERSION_PATH}/${FIRMWARE_NAME}_full_${VERSION}.lod
WITH_PLT_OTA_FILE=${FOTA_PATH}/${FIRMWARE_NAME}_fota_${VERSION}.lod
STRIP_SYMBOL_FILE=${CSDK_PATH}/platform/compilation/platform_symbols_to_strip
PLATFORM_ELF_FOLDER_PATH=${CSDK_PATH}/platform/csdk/${CFG_RELEASE}
PLATFORM_LOD_PATH=${CSDK_PATH}/platform/csdk/${CFG_RELEASE}
PLATFORM_LOD_FILE=`find ${PLATFORM_LOD_PATH} -name *.lod`
LODCOMBINE_TOOL=${CSDK_PATH}/platform/compilation/lodtool.py

PLATFORM=$(sh -c uname)
if [[ "${PLATFORM}" == "Windows_NT" ]]; then
    FOTACREATE_TOOL=${CSDK_PATH}/platform/compilation/fota/fotacreate.exe
fi
if [[ "${PLATFORM}" == "Linux" ]]; then
    FOTACREATE_TOOL=${CSDK_PATH}/platform/compilation/fota/linux/fotacreate
fi

if [[ "${FOLDER_NAME}" != "gprs_a9" ]]; then
    echo "Please exec build-fota.sh in gprs_a9 folder"
    exit 1
fi

python3 ${LODCOMBINE_TOOL} gen_ota --platform ${PLATFORM_LOD_FILE} --lod ${WITH_PLT_LOD_FILE} --out ${WITH_PLT_OTA_FILE}
if [ "${PLATFORM}" = "Windows_NT" ] ; then
    dos2unix ${WITH_PLT_OTA_FILE}
fi

for f in ${VERSION_PATH}/*.lod;
do
    OLDVER=`echo $f | awk '{print substr($$1, length($$1) - 7, 4)}'`
    if [[ ${OLDVER} != ${VERSION} ]]; then
        echo -e "Make fota " ${OLDVER} "to" ${VERSION} " upgrade pack...\n"
        FOTA_FILE=${FIRMWARE_NAME}_${OLDVER}_to_${VERSION}.pack
        FOTA_PACK=${FOTA_PATH}/${FOTA_FILE}
        WITH_PLT_OTA_OLD=${FOTA_PATH}/${FIRMWARE_NAME}_fota_${OLDVER}.lod
        ${FOTACREATE_TOOL} 4194304 65536 ${WITH_PLT_OTA_OLD} ${WITH_PLT_OTA_FILE} ${FOTA_PACK}
        #A9G FOTA does not understand deflated files
        #if [[ ${ZIP_COMPRESS} == "y" && -d "${WWW_PATH}" ]]; then
        #    /usr/bin/zlib-flate -compress < ${FOTA_PACK} > ${WWW_PATH}/${FOTA_FILE}
        #fi
        #if [[ ${ZIP_COMPRESS} != "y" && -d "${WWW_PATH}" ]]; then
        #    cp ${FOTA_PACK} ${WWW_PATH}/${FOTA_FILE}
        #fi
        if [[ -d "${WWW_PATH}" ]]; then
            cp ${FOTA_PACK} ${WWW_PATH}/${FOTA_FILE}
        fi
    fi
done

