#!/bin/sh

source ./environment.sh

# Start application in debug mode
#${ADB} shell am start -n \
#	"${PACKAGE_NAME}/${PACKAGE_NAME}.MainActivity" \
#	-a android.intent.action.MAIN \
#	-c android.intent.category.LAUNCHER \
#	-D
#sleep 0.5
# NOTE: If we start the application in non debug mode and we attach later without using jdb, then it attaches and finds the symbols correctly

# TODO: Get the full route and put it into environment.sh
JDB="jdb"

# Get PID
PID=$(${ADB} shell pidof ${PACKAGE_NAME})
if [ ! ${PID} ]; then
	echo "Could not find process for ${PACKAGE_NAME}"
	exit -1
fi

${ADB} forward tcp:12345 jdwp:${PID}
${JDB} -attach localhost:12345

