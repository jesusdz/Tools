#!/bin/sh

source ./environment.sh

TEMP_DIR=/data/local/tmp
LLDB_DIR=/data/data/${PACKAGE_NAME}/lldb

# Start application in debug mode
#${ADB} shell am start -n \
#	"${PACKAGE_NAME}/${PACKAGE_NAME}.MainActivity" \
#	-a android.intent.action.MAIN \
#	-c android.intent.category.LAUNCHER \
#	-D
# NOTE: If we start the application in non debug mode and we attach later without using jdb, then it attaches and finds the symbols correctly

# Push lldb-server and start_lldb_server.sh
${ADB} push ${LLDB_SERVER} ${TEMP_DIR}
${ADB} push start_lldb_server.sh ${TEMP_DIR}

# Copy lldb-server and start_lldb_server.sh to app folder with permissions
${ADB} shell run-as ${PACKAGE_NAME} mkdir -p ${LLDB_DIR}/bin
${ADB} shell "run-as ${PACKAGE_NAME} killall -q lldb-server 2>/dev/null"
${ADB} shell "cat ${TEMP_DIR}/lldb-server | run-as ${PACKAGE_NAME} sh -c 'cat > ${LLDB_DIR}/bin/lldb-server && chmod 700 ${LLDB_DIR}/bin/lldb-server'"
${ADB} shell "cat ${TEMP_DIR}/start_lldb_server.sh | run-as ${PACKAGE_NAME} sh -c 'cat > ${LLDB_DIR}/bin/start_lldb_server.sh && chmod 700 ${LLDB_DIR}/bin/start_lldb_server.sh'"

# Launch start_lldb_server.sh
${ADB} shell "run-as ${PACKAGE_NAME} ${LLDB_DIR}/bin/start_lldb_server.sh ${LLDB_DIR} unix-abstract /${PACKAGE_NAME} debug.socket \"lldb process:gdb-remote packets\""

