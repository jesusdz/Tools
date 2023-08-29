#!/bin/sh

# Install successfully finished in 146 ms.
# $ adb shell am start -n "com.example.myapplication/com.example.myapplication.MainActivity" -a android.intent.action.MAIN -c android.intent.category.LAUNCHER -D
# Connected to process 14791 on device 'realme-rmx3085-YTLNLJEYTGVGJBH6'.
# Now Launching Native Debug Session
# $ adb shell cat /data/local/tmp/lldb-server | run-as com.example.myapplication sh -c 'cat > /data/data/com.example.myapplication/lldb/bin/lldb-server && chmod 700 /data/data/com.example.myapplication/lldb/bin/lldb-server'
# $ adb shell cat /data/local/tmp/start_lldb_server.sh | run-as com.example.myapplication sh -c 'cat > /data/data/com.example.myapplication/lldb/bin/start_lldb_server.sh && chmod 700 /data/data/com.example.myapplication/lldb/bin/start_lldb_server.sh'
# Starting LLDB server: /data/data/com.example.myapplication/lldb/bin/start_lldb_server.sh /data/data/com.example.myapplication/lldb unix-abstract /com.example.myapplication-0 platform-1693166601774.sock "lldb process:gdb-remote packets"
# Debugger attached to process 14791

source ./environment.sh

TEMP_DIR=/data/local/tmp
LLDB_DIR=/data/data/${PACKAGE_NAME}/lldb/bin

# Start application in debug mode
${ADB} shell am start -n \
	"${PACKAGE_NAME}/${PACKAGE_NAME}.MainActivity" \
	-a android.intent.action.MAIN \
	-c android.intent.category.LAUNCHER \
	-D

# Push lldb-server and start_lldb_server.sh
${ADB} push ${LLDB_SERVER} ${TEMP_DIR}
${ADB} push start_lldb_server.sh ${TEMP_DIR}

# Copy lldb-server and start_lldb_server.sh to app folder with permissions
${ADB} shell run-as ${PACKAGE_NAME} mkdir -p ${LLDB_DIR}
${ADB} shell "run-as ${PACKAGE_NAME} killall -q lldb-server 2>/dev/null"
${ADB} shell "cat ${TEMP_DIR}/lldb-server | run-as ${PACKAGE_NAME} sh -c 'cat > ${LLDB_DIR}/lldb-server && chmod 700 ${LLDB_DIR}/lldb-server'"
${ADB} shell "cat ${TEMP_DIR}/start_lldb_server.sh | run-as ${PACKAGE_NAME} sh -c 'cat > ${LLDB_DIR}/start_lldb_server.sh && chmod 700 ${LLDB_DIR}/start_lldb_server.sh'"

# Launch start_lldb_server.sh
${ADB} shell "run-as ${PACKAGE_NAME} ${LLDB_DIR}/start_lldb_server.sh /data/data/${PACKAGE_NAME}/lldb unix-abstract /${PACKAGE_NAME}-0 platform-0000.sock \"lldb process:gdb-remote packets\""


################################################################################
# Old debug.sh. Left here for reference (although it never worked anyways).
#!/bin/sh
#
# source ./environment.sh
#
# echo ${ADB}
# LLDB_SERVER_DIR=${TOOLCHAIN}/lib64/clang/14.0.7/lib/linux/aarch64
#
# pushd ${LLDB_SERVER_DIR}
# ${ADB} push lldb-server /data/local/tmp
# popd
# ${ADB} shell am start -a android.intent.action.MAIN -n com.tools.game/.MainActivity
################################################################################

