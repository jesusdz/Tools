
set JAVA_HOME="C:\path\to\java\home"
set ANDROID_HOME="C:\path\to\android\home"

if not exist %JAVA_HOME% (
	echo Configure JAVA_HOME in environment.bat.
	exit /b 1
)

if not exist %ANDROID_HOME% (
	echo Configure ANDROID_HOME in environment.bat.
	exit /b 1
)

exit /b 0

