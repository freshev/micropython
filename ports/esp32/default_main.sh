#!/bin/bash

BOOT="./modules/boot.py"

#insert file in boot.py
function insert_boot {
    echo    "try:" >> $BOOT
    echo    "    uos.stat('$2')" >> $BOOT
    echo    "except:" >> $BOOT
    echo    "    f=open('$2', 'w')" >> $BOOT
    echo -n "    f.write(ubinascii.a2b_base64('">> $BOOT
    base64 -w 0 ./$1 >> $BOOT
    echo    "'))" >> $BOOT
    echo    "    f.close()" >> $BOOT
}

#make boot.py
/usr/bin/rm $BOOT
echo "import uos" >> $BOOT
echo "import ubinascii" >> $BOOT
insert_boot default_main.py main.py

