#!/bin/bash
# this file is used to auto-generate .qm file from .ts file.
# author: shibowen at linuxdeepin.com

# Qt6: /usr/lib/qt6/bin/lrelease, Qt5: lrelease (via qtchooser)
if [ -x /usr/lib/qt6/bin/lrelease ]; then
    LRELEASE=/usr/lib/qt6/bin/lrelease
else
    LRELEASE=lrelease
fi

ts_list=(`ls */translations/*.ts 2>/dev/null`)

for ts in "${ts_list[@]}"
do
    printf "\nprocess ${ts}\n"
    $LRELEASE "${ts}"
done
