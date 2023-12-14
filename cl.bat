@echo off
setlocal enabledelayedexpansion

REM cl.bat helper script
REM Automatically finds the cl.exe executable making use of vswhere.exe
REM More info can be found here: https://github.com/microsoft/vswhere/wiki/Find-VC

REM VsWhere helps us find the location of VS components. It is always placed here.
set VsWhere="C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe"

REM Abusing the `for` statement, we can get the installation dir out of `vswhere` into the variable `InstallDir`.
for /f "usebackq tokens=*" %%i in (`%VsWhere% -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
  set InstallDir=%%i
)

if exist "%InstallDir%\VC\Auxiliary\Build\Microsoft.VCToolsVersion.default.txt" (
  set /p Version=<"%InstallDir%\VC\Auxiliary\Build\Microsoft.VCToolsVersion.default.txt"
  rem Trim
  set Version=!Version: =!
)

REM Get the latest version out of this temporary file used to locate the latest toolset
if not "%Version%"=="" (
  rem We are hardcoding x64 as the host and target architecture, but it could parsed from arguments
  "%InstallDir%\VC\Tools\MSVC\%Version%\bin\HostX64\x64\cl.exe" %*
)

