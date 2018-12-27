#/bin/sh

INSTALL_DIR=$1

function fixup() {
    echo $3": "$1" -> "$2
    install_name_tool -change $1 $2 $3
}

fixup_lib() {
    FROM="\"/opt/local/lib/"$1"\""
    TO="\"@executable_path/lib/"$1"\""
    fixup $FROM $TO $2
}

for BIN in $INSTALL_DIR/*; do
    if [ ! -d "$BIN" ]; then
        fixup_lib "libboost_system-mt.dylib" $BIN
        fixup_lib "libboost_filesystem-mt.dylib" $BIN
        fixup_lib "libboost_thread-mt.dylib" $BIN
        fixup_lib "libboost_date_time-mt.dylib" $BIN
        fixup_lib "libboost_chrono-mt.dylib" $BIN
        fixup_lib "libboost_regex-mt.dylib" $BIN
        fixup_lib "libboost_serialization-mt.dylib" $BIN
        fixup_lib "libboost_atomic-mt.dylib" $BIN
        fixup_lib "libboost_program_options-mt.dylib" $BIN
    fi
done
