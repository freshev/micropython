#!/bin/bash

BOOT="./modules/boot.py"

URL="$1"
URL="${URL//\"/}"
ZIP_COMPRESS="$2"
ZIP_COMPRESS="${ZIP_COMPRESS//\"/}"
RESPAWN="$3"
RESPAWN="${RESPAWN//\"/}"

#echo "ULR: ${URL}" 
#echo "ZIP_COMPRESS: ${ZIP_COMPRESS}" 
#echo "RESPAWN: ${RESPAWN}" 

if [ "$URL" !=  "" ]; then
	if [ "$ZIP_COMPRESS" == "y" ]; then
		curl --silent --compressed $URL > ./examples/main.py
	else
		curl --silent $URL > ./examples/main.py
	fi
fi

if [ -f ${BOOT} ]; then
	rm ${BOOT}
fi

if [ "$RESPAWN" == "y" ]; then
	echo "import os" >> ${BOOT}
	echo "import binascii" >> ${BOOT}
	echo    "try:" >> ${BOOT}
    echo    "    os.stat('main.py')" >> ${BOOT}
    #echo    "    os.remove('$3')" >> ${BOOT}
    echo    "except:" >> ${BOOT}
    echo    "    f=open('main.py', 'w')" >> ${BOOT}
    echo -n "    f.write(binascii.a2b_base64('">> ${BOOT}
    base64 ./examples/main.py | tr -d \\n >> ${BOOT}
    echo    "'))" >> ${BOOT}
    echo    "    f.close()" >> ${BOOT}
    echo    "    pass" >> ${BOOT}
fi
