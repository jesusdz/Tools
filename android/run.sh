#!/bin/sh

mScreenOn=`adb shell "dumpsys deviceidle | grep mScreenOn | cut -d= -f2"`
echo "mScreenOn=$mScreenOn"
mScreenLocked=`adb shell "dumpsys deviceidle | grep mScreenLocked | cut -d= -f2"`
echo "mScreenLocked=$mScreenLocked"

if [ "$mScreenOn" == "false" ]; then
	adb shell input keyevent 26
	sleep 0.2
else
	if [ "$mScreenLocked" == "true" ]; then
		adb shell input keyevent 26
		adb shell input keyevent 26
		sleep 0.2
	fi
fi

if [ "$mScreenLocked" == "true" ]; then
	adb shell input swipe 540 1000 540 700
	sleep 0.3
	adb shell input motionevent DOWN 225 1322
	sleep 0.05
	adb shell input motionevent MOVE 225 1882
	sleep 0.05
	adb shell input motionevent MOVE 540 1600
	sleep 0.05
	adb shell input motionevent MOVE 852 1882
	sleep 0.05
	adb shell input motionevent UP 852 1882
	sleep 0.2
fi

adb shell am start -a android.intent.action.MAIN -n com.tools.game/.MainActivity

./logcat.sh

