#!/bin/bash

DESKTOP_TEMP_FILE=usb-device-formatter.desktop.tmp
DESKTOP_SOURCE_FILE=usb-device-formatter.desktop
DESKTOP_DEST_FILE=usb-device-formatter.desktop
DESKTOP_TS_DIR=translations/desktop/

if [ ! -x /usr/bin/deepin-desktop-ts-convert ]; then
    echo "The required deepin-desktop-ts-convert is NOT found, failed to generate desktop translation!!"
    exit -1
fi

/usr/bin/deepin-desktop-ts-convert ts2desktop $DESKTOP_SOURCE_FILE $DESKTOP_TS_DIR $DESKTOP_TEMP_FILE
mv $DESKTOP_TEMP_FILE $DESKTOP_DEST_FILE
