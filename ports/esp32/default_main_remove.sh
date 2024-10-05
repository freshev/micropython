#!/bin/bash

BOOT="./modules/boot.py"
/usr/bin/rm $BOOT
echo "import uos" >> $BOOT
echo "uos.remove('main.py')" >> $BOOT

