#!/bin/bash
set -e

#WWW_PATH=/var/opt/asque/firmware/Device_FW
VERSION=$1
VERSION="${VERSION//\"/}"

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

echo -e "Generate fota version " ${VERSION} " ...\n"
echo -e "1: ${LODCOMBINE_TOOL}"
echo -e "2: ${PLATFORM_LOD_FILE}"
echo -e "3: ${WITH_PLT_LOD_FILE}"
echo -e "4: ${WITH_PLT_OTA_FILE}"
echo "--------------------"
python ${LODCOMBINE_TOOL} gen_ota --platform ${PLATFORM_LOD_FILE} --lod ${WITH_PLT_LOD_FILE} --out ${WITH_PLT_OTA_FILE}

for f in ${VERSION_PATH}/*.lod;
do
    OLDVER=`echo $f | awk '{print substr($$1, length($$1) - 7, 4)}'`
    if [[ ${OLDVER} != ${VERSION} ]]; then
        echo -e "Make fota " ${OLDVER} "to" ${VERSION} " upgrade pack...\n"
        FOTA_PACK=${FOTA_PATH}/${FIRMWARE_NAME}_${OLDVER}_to_${VERSION}.pack
        WITH_PLT_OTA_OLD=${FOTA_PATH}/${FIRMWARE_NAME}_fota_${OLDVER}.lod
        ${FOTACREATE_TOOL} 4194304 65536 ${WITH_PLT_OTA_OLD} ${WITH_PLT_OTA_FILE} ${FOTA_PACK}
        #cp ${FOTA_PACK} ${WWW_PATH}
    fi
done

