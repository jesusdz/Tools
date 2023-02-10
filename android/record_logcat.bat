@echo off

adb logcat -c 
adb -d logcat *:W > log.txt

