#!/bin/bash

if [ ! -x /usr/bin/deepin-desktop-ts-convert ]; then
    echo "The required deepin-desktop-ts-convert is NOT found, translation generation failed!!"
    exit -1
fi

DESKTOP_COMPUTER_TEMP_FILE=data/applications/gxde-computer.desktop.tmp
DESKTOP_COMPUTER_FILE=data/applications/gxde-computer.desktop
DESKTOP_COMPUTER_TS_DIR=translations/gxde-computer-desktop/

/usr/bin/deepin-desktop-ts-convert ts2desktop $DESKTOP_COMPUTER_FILE $DESKTOP_COMPUTER_TS_DIR $DESKTOP_COMPUTER_TEMP_FILE
mv $DESKTOP_COMPUTER_TEMP_FILE $DESKTOP_COMPUTER_FILE


DESKTOP_TRASH_TEMP_FILE=data/applications/gxde-trash.desktop.tmp
DESKTOP_TRASH_FILE=data/applications/gxde-trash.desktop
DESKTOP_TRASH_TS_DIR=translations/gxde-trash-desktop/

/usr/bin/deepin-desktop-ts-convert ts2desktop $DESKTOP_TRASH_FILE $DESKTOP_TRASH_TS_DIR $DESKTOP_TRASH_TEMP_FILE
mv $DESKTOP_TRASH_TEMP_FILE $DESKTOP_TRASH_FILE
