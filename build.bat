@echo off
setlocal



REM Prepare the VC compiler environment

IF NOT EXIST build mkdir build

set RootDir=%~dp0
set RootDir=%RootDir:~0,-1%

set DXC=%RootDir%\dxc\windows\bin\x64\dxc.exe

REM set CommonCompilerFlags=-diagnostics:column -WL -O2 -nologo -fp:fast -fp:except- -Gm- -GR- -EHa- -Zo -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -wd4505 -wd4127 -FC -Z7 -GS- -Gs9999999
REM set CommonCompilerFlags=-DHANDMADE_INTERNAL=1 -DHANDMADE_SLOW=1 -DHANDMADE_WIN32=1 %CommonCompilerFlags%
REM set CommonLinkerFlags=-STACK:0x100000,0x100000 -incremental:no -opt:ref user32.lib gdi32.lib winmm.lib bcrypt.lib kernel32.lib

REM Flags
set CommonCompilerFlags=-Zi -D_CRT_SECURE_NO_WARNINGS /std:c++20
set CommonLinkerFlags=-incremental:no -opt:ref -nologo

REM Flags for preprocessor pass
REM set CommonCompilerFlags=-P -Fipreprocessed.cpp
REM set CommonLinkerFlags=




REM ######################################################
REM Target to build
REM ######################################################

set target=%1
if [%target%] == [] goto main_gfx
if "%target%" == "cast" goto cast
if "%target%" == "reflex" goto reflex
if "%target%" == "main_gfx" goto main_gfx
if "%target%" == "main_d3d12" goto main_d3d12
if "%target%" == "main_reflect_serialize" goto main_reflect_serialize
if "%target%" == "main_interpreter" goto main_interpreter
if "%target%" == "main_atof" goto main_atof
if "%target%" == "shaders" goto shaders
if "%target%" == "reflection" goto reflection
if "%target%" == "clean" goto clean
echo Invalid target
exit /b 1



REM ######################################################
REM Reflex
REM ######################################################
: reflex

call vcenv.bat
pushd build
set CommonCompilerFlags=%CommonCompilerFlags%
set CommonLinkerFlags=%CommonLinkerFlags%
cl %CommonCompilerFlags% ..\reflex.cpp /link %CommonLinkerFlags%
popd
exit /b 0



REM ######################################################
REM main_gfx
REM ######################################################
: main_gfx

call vcenv.bat
pushd build
set CommonCompilerFlags=%CommonCompilerFlags% -I %RootDir%\vulkan\include
set CommonLinkerFlags=%CommonLinkerFlags% user32.lib
cl %CommonCompilerFlags% ..\reflex.cpp /link %CommonLinkerFlags%
reflex.exe ..\assets\assets.h > ..\assets.reflex.h
cl %CommonCompilerFlags% ..\main_gfx.cpp /link %CommonLinkerFlags%
popd
exit /b 0



REM ######################################################
REM main_interpreter
REM ######################################################
: main_interpreter

call vcenv.bat
set CommonCompilerFlags=%CommonCompilerFlags%
set CommonLinkerFlags=%CommonLinkerFlags%
pushd build
cl %CommonCompilerFlags% ..\main_interpreter.cpp /link %CommonLinkerFlags%
popd
exit /b 0



REM ######################################################
REM main_d3d12
REM ######################################################
: main_d3d12

call vcenv.bat
set CommonCompilerFlags=%CommonCompilerFlags%
set CommonLinkerFlags=%CommonLinkerFlags% d3d12.lib dxgi.lib dxguid.lib user32.lib d3dcompiler.lib
pushd build
cl %CommonCompilerFlags% ..\main_d3d12.cpp /link %CommonLinkerFlags%
popd
exit /b 0



REM ######################################################
REM main_atof
REM ######################################################
: main_atof

call vcenv.bat
set CommonCompilerFlags=%CommonCompilerFlags%
set CommonLinkerFlags=%CommonLinkerFlags%
pushd build
cl %CommonCompilerFlags% ..\main_atof.cpp /link %CommonLinkerFlags%
popd
exit /b 0



REM ######################################################
REM main_spirv
REM ######################################################
: main_spirv

call vcenv.bat
set CommonCompilerFlags=%CommonCompilerFlags%
set CommonLinkerFlags=%CommonLinkerFlags%
pushd build
cl %CommonCompilerFlags% ..\main_spirv.cpp /link %CommonLinkerFlags%
popd
exit /b 0



REM ######################################################
REM cast
REM ######################################################
: cast

call vcenv.bat
set CommonCompilerFlags=%CommonCompilerFlags%
set CommonLinkerFlags=%CommonLinkerFlags%
pushd build
cl %CommonCompilerFlags% ..\cast.cpp /link %CommonLinkerFlags%
popd
exit /b 0



REM ######################################################
REM main_reflect_serialize
REM ######################################################
: main_reflect_serialize

call vcenv.bat
set CommonCompilerFlags=%CommonCompilerFlags%
set CommonLinkerFlags=%CommonLinkerFlags%
pushd build
cl %CommonCompilerFlags% ..\main_reflect_serialize.cpp /link %CommonLinkerFlags%
popd
exit /b 0



REM ######################################################
REM Shaders
REM ######################################################
: shaders

pushd shaders

del /Q *.spv *.dxil *.pdb *.cso *.dis 2>nul

%DXC% -spirv -T vs_6_7 -Fo vs_shading.spv -Fc vs_shading.dis shading.hlsl -E VSMain
%DXC% -spirv -T ps_6_7 -Fo fs_shading.spv -Fc fs_shading.dis shading.hlsl -E PSMain
%DXC% -spirv -T vs_6_7 -Fo vs_sky.spv -Fc vs_sky.dis sky.hlsl -E VSMain
%DXC% -spirv -T ps_6_7 -Fo fs_sky.spv -Fc fs_sky.dis sky.hlsl -E PSMain
%DXC% -spirv -T vs_6_7 -Fo vs_shadowmap.spv -Fc vs_shadowmap.dis shadowmap.hlsl -E VSMain
%DXC% -spirv -T ps_6_7 -Fo fs_shadowmap.spv -Fc fs_shadowmap.dis shadowmap.hlsl -E PSMain
%DXC% -spirv -T vs_6_7 -Fo vs_ui.spv -Fc vs_ui.dis ui.hlsl -E VSMain
%DXC% -spirv -T ps_6_7 -Fo fs_ui.spv -Fc fs_ui.dis ui.hlsl -E PSMain
%DXC% -spirv -T vs_6_7 -Fo vs_id.spv -Fc vs_id.dis id.hlsl -E VSMain
%DXC% -spirv -T ps_6_7 -Fo fs_id.spv -Fc fs_id.dis id.hlsl -E PSMain
%DXC% -spirv -T cs_6_7 -Fo compute_select.spv -Fc compute_select.dis compute_select.hlsl -E CSMain
%DXC% -spirv -T cs_6_7 -Fo compute_clear.spv -Fc compute_clear.dis compute.hlsl -E main_clear
%DXC% -spirv -T cs_6_7 -Fo compute_update.spv -Fc compute_update.dis compute.hlsl -E main_update

REM if "%target%"=="main_d3d12.cpp" (
REM 	%DXC% -nologo -E main -T vs_6_0 -Zi -Fd vertex.pdb -Fo vertex.cso vertex.hlsl
REM 	%DXC% -nologo -E main -T ps_6_0 -Zi -Fd fragment.pdb -Fo fragment.cso fragment.hlsl
REM )

popd

exit /b 0



REM ######################################################
REM Reflection
REM ######################################################
: reflection

echo build\reflex.exe assets\assets.h ^> assets.reflex.h
build\reflex.exe assets\assets.h > assets.reflex.h
exit /b 0



REM ######################################################
REM Clean
REM ######################################################
: clean

del /Q build\
pushd shaders
del /Q *.spv *.dxil *.pdb *.cso *.dis 2>nul
popd
exit /b 0



endlocal
