@echo off

IF NOT EXIST build mkdir build

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

REM Interpreter
REM cl %CommonCompilerFlags% ..\main_interpreter.cpp /link %CommonLinkerFlags%

REM Vulkan window
REM set CommonCompilerFlags=%CommonCompilerFlags% -I "C:\Program Files\VulkanSDK\Include"
REM set CommonLinkerFlags=%CommonLinkerFlags% user32.lib
REM cl %CommonCompilerFlags% ..\main_vulkan.cpp /link %CommonLinkerFlags%

REM D3D12 window
set CommonLinkerFlags=%CommonLinkerFlags% d3d12.lib dxgi.lib user32.lib
cl %CommonCompilerFlags% ..\main_d3d12.cpp /link %CommonLinkerFlags%

popd

pushd shaders

REM Build shaders
REM glslc -fshader-stage=vertex vertex.glsl -o vertex.spv
REM glslc -fshader-stage=fragment fragment.glsl -o fragment.spv

popd

