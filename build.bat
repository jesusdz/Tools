@echo off

IF NOT EXIST build mkdir build
pushd build
del *.pdb > NUL 2> NUL
popd

REM set CommonCompilerFlags=-diagnostics:column -WL -O2 -nologo -fp:fast -fp:except- -Gm- -GR- -EHa- -Zo -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -wd4505 -wd4127 -FC -Z7 -GS- -Gs9999999
REM set CommonCompilerFlags=-DHANDMADE_INTERNAL=1 -DHANDMADE_SLOW=1 -DHANDMADE_WIN32=1 %CommonCompilerFlags%
REM set CommonLinkerFlags=-STACK:0x100000,0x100000 -incremental:no -opt:ref user32.lib gdi32.lib winmm.lib bcrypt.lib kernel32.lib

REM Flags
set CommonCompilerFlags=-Zi
set CommonLinkerFlags=-incremental:no -opt:ref -nologo

REM Flags for preprocessor pass
REM set CommonCompilerFlags=-P -Fipreprocessed.cpp
REM set CommonLinkerFlags=

REM Interpreter
cl %CommonCompilerFlags% -D_CRT_SECURE_NO_WARNINGS main.cpp /link %CommonLinkerFlags%

REM Vulkan window
REM cl %CommonCompilerFlags% -D_CRT_SECURE_NO_WARNINGS main_xwindow.cpp /link %CommonLinkerFlags%

popd
