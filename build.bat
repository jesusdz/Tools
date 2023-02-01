@echo off

IF NOT EXIST build mkdir build

:: Target to build
set target=%1
if [%target%]==[] set target=main_d3d12.cpp

:: Build binaries
pushd build

del /Q *.* > NUL 2> NUL

REM set CommonCompilerFlags=-diagnostics:column -WL -O2 -nologo -fp:fast -fp:except- -Gm- -GR- -EHa- -Zo -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -wd4505 -wd4127 -FC -Z7 -GS- -Gs9999999
REM set CommonCompilerFlags=-DHANDMADE_INTERNAL=1 -DHANDMADE_SLOW=1 -DHANDMADE_WIN32=1 %CommonCompilerFlags%
REM set CommonLinkerFlags=-STACK:0x100000,0x100000 -incremental:no -opt:ref user32.lib gdi32.lib winmm.lib bcrypt.lib kernel32.lib

REM Flags
set CommonCompilerFlags=-Zi -D_CRT_SECURE_NO_WARNINGS
set CommonLinkerFlags=-incremental:no -opt:ref -nologo

REM Flags for preprocessor pass
REM set CommonCompilerFlags=-P -Fipreprocessed.cpp
REM set CommonLinkerFlags=

if "%target%" == "main_interpreter.cpp" (

	set CommonCompilerFlags=%CommonCompilerFlags%
	set CommonLinkerFlags=%CommonLinkerFlags%

) else if "%target%" == "main_vulkan.cpp" (

	set CommonCompilerFlags=%CommonCompilerFlags% -I "C:\Program Files\VulkanSDK\Include"
	set CommonLinkerFlags=%CommonLinkerFlags% user32.lib

) else if "%target%" == "main_d3d12.cpp" (

	set CommonCompilerFlags=%CommonCompilerFlags% /EHsc
	set CommonLinkerFlags=%CommonLinkerFlags% d3d12.lib dxgi.lib dxguid.lib user32.lib d3dcompiler.lib

) else if "%target%" == "main_atof.cpp" (

	set CommonCompilerFlags=%CommonCompilerFlags%
	set CommonLinkerFlags=%CommonLinkerFlags%

) else (

	echo Invalid target: %target%
	popd
	exit /b
)

cl %CommonCompilerFlags% ..\%target% /link %CommonLinkerFlags%

popd

:: Build shaders
pushd shaders

del /Q *.spv *.dxil *.pdb

if "%target%"=="main_vulkan.cpp" (
	glslc -fshader-stage=vertex vertex.glsl -o vertex.spv
	glslc -fshader-stage=fragment fragment.glsl -o fragment.spv
)

if "%target%"=="main_d3d12.cpp" (
	dxc -nologo -E main -T vs_6_0 -Zi -Fd vertex.pdb -Fo vertex.cso vertex.hlsl
	dxc -nologo -E main -T ps_6_0 -Zi -Fd fragment.pdb -Fo fragment.cso fragment.hlsl
)

popd

