#!/bin/bash
# this file is used to auto-generate .qm file from .ts file.
# author: shibowen at linuxdeepin.com

ts_list=(`ls translations/*.ts`)

LRELEASE=
for cand in \
    lrelease-qt6 \
    /usr/lib/qt6/bin/lrelease \
    /usr/lib/qt6/libexec/lrelease \
    /usr/lib/x86_64-linux-gnu/qt6/bin/lrelease \
    /usr/lib/x86_64-linux-gnu/qt6/libexec/lrelease \
    lrelease6 \
    lrelease
do
    if [ -x "$cand" ] || command -v "$cand" >/dev/null 2>&1; then
        LRELEASE=$cand
        break
    fi
done
if [ -z "$LRELEASE" ]; then
    echo "lrelease not found" >&2
    exit 1
fi
echo "using LRELEASE=$LRELEASE"

for ts in "${ts_list[@]}"
do
    printf "\nprocess ${ts}\n"
    "$LRELEASE" "${ts}"
done
