#!/bin/bash

URL="https://asque.ru/firmware"
BOOT="./modules/boot.py"

#insert file in boot.py
function insert_boot {
    curl --silent --compressed ${URL}/$1/$2 > ./examples/$3
    echo    "try:" >> ${BOOT}
    echo    "    os.stat('$3')" >> ${BOOT}
    #echo    "    os.remove('$3')" >> ${BOOT}
    echo    "except:" >> ${BOOT}
    echo    "    f=open('$3', 'w')" >> ${BOOT}
    echo -n "    f.write(binascii.a2b_base64('">> ${BOOT}
    base64 ./examples/$3 | tr -d \\n >> ${BOOT}
    echo    "'))" >> ${BOOT}
    echo    "    f.close()" >> ${BOOT}
    echo    "    pass" >> ${BOOT}
    rm ./examples/$3
}

#make boot.py
#TODO use CONFIG_MAIN_STUB_RESPAWN from $1
rm ${BOOT}
echo "import os" >> ${BOOT}
echo "import binascii" >> ${BOOT}
insert_boot Device main.py main.py

#add other "built-in" modules for A9
curl --silent --compressed ${URL}/Device/filesys.py > ./modules/filesys.py
curl --silent --compressed ${URL}/Device/settings.py > ./modules/settings.py
curl --silent --compressed ${URL}/Device/umqtt.py > ./modules/umqtt.py
curl --silent --compressed ${URL}/Device/uota.py > ./modules/uota.py
curl --silent --compressed ${URL}/Device/urequests.py > ./modules/urequests.py

