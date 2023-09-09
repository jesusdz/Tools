#!/bin/bash

source ./environment.sh

# Get PID
PID=$(${ADB} shell pidof ${PACKAGE_NAME})
if [ ! ${PID} ]; then
	echo "$0: Could not find process for ${PACKAGE_NAME}" 1>&2
	exit -1
fi

echo ${PID}
