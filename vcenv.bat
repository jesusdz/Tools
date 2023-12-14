@echo off

if not "%VCINSTALLDIR%" == "" exit /b

REM `vswhere` provides the location of VS components. It is always placed here.
set VsWhere="C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe"

REM Abusing the `for` statement, we can get the `InstallDir` from `vswhere`.
for /f "usebackq tokens=*" %%i in (`%VsWhere% -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
  set InstallDir=%%i
)

REM Set the VC environment variables
call "%InstallDir%\VC\Auxiliary\Build\vcvars64.bat"

set VsWhere=
set InstallDir=

