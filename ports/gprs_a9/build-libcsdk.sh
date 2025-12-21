#!/bin/bash
set -e

######################## 1 ################################
# check path
curr_path_abs=$(cd `dirname $0`; pwd)
curr_folder_name="${curr_path_abs##*/}"
echo $curr_path_abs
if [[ "${curr_folder_name}" != "gprs_a9" ]]; then
    echo "Plese exec build.sh in gprs_a9 folder"
    exit 1
fi

######################### 2 ###############################
# check parameters
CFG_RELEASE=debug
if [[ "$1xx" == "releasexx" ]]; then
    CFG_RELEASE=release
fi

PLATFORM=$(sh -c uname)
if [[ "${PLATFORM}" == "Windows_NT" ]]; then
    CSDTK_PATH=csdtk42-windows
fi
if [[ "${PLATFORM}" == "Linux" ]]; then
    CSDTK_PATH=csdtk42-linux
fi
echo ">> Generate CSDK lib now"
cd ../../  #root path of micropython project
cd lib/GPRS_C_SDK/platform/tools/genlib
chmod +x genlib.sh
PATH=$PATH:$curr_path_abs/../../lib/${CSDTK_PATH}/bin
export LD_LIBRARY_PATH=$curr_path_abs/../../lib/${CSDTK_PATH}/lib
MAKE_J_NUMBER=4
./genlib.sh ${CFG_RELEASE} > /dev/null
cd ../../../../../
if [[ -f "lib/GPRS_C_SDK/hex/libcsdk/libcsdk_${CFG_RELEASE}.a" ]]; then
    echo ">> Geneate CSDK (libcsdk_${CFG_RELEASE}.a) lib complete"
else
    echo ">> Generate CSDK lib fail, please check error"
    exit 1
fi
cd ${curr_path_abs}

