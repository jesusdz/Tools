#!/bin/sh

source ./environment.sh

echo ${ADB}
LLDB_SERVER_DIR=${TOOLCHAIN}/lib64/clang/14.0.7/lib/linux/aarch64

pushd ${LLDB_SERVER_DIR}
${ADB} push lldb-server /data/local/tmp
popd
${ADB} shell am start -a android.intent.action.MAIN -n com.tools.game/.MainActivity

