#!/bin/sh

source ./environment.sh

echo "platform.log:"
${ADB} shell "run-as ${PACKAGE_NAME} cat /data/data/${PACKAGE_NAME}/lldb/log/platform.log"
echo ""

echo "platform-stdout.log:"
${ADB} shell "run-as ${PACKAGE_NAME} cat /data/data/${PACKAGE_NAME}/lldb/log/platform-stdout.log"
echo ""

echo "gdb-server.log:"
${ADB} shell "run-as ${PACKAGE_NAME} cat /data/data/${PACKAGE_NAME}/lldb/log/gdb-server.log"
echo ""

