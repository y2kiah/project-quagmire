@echo off

rem -od or -O2 here
set CommonCompilerFlags=-Od
set CommonCompilerFlags=-MT -Oi -Ot -WL -nologo -fp:fast -fp:except- -GF -Gm- -GR- -EHa- -Zo -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -wd4505 -wd4127 -FC -Z7 -GS- -Gs9999999 -Zc:inline -Zc:ternary -diagnostics:caret %CommonCompilerFlags%
set CommonCompilerFlags=-I../vendor/SDL2-2.0.9/include -I../vendor/glew-2.1.0/include %CommonCompilerFlags%
set CommonLinkerFlags=-machine:X64 -stack:0x100000,0x100000 -incremental:no -opt:ref -ignore:4099 user32.lib gdi32.lib winmm.lib kernel32.lib SDL2.lib SDL2main.lib glew32s.lib opengl32.lib -libpath:../vendor/SDL2-2.0.9/VisualC/x64/Release -libpath:../vendor/glew-2.1.0/lib/Release/x64

if not exist .\build mkdir .\build
pushd .\build

del *.pdb > NUL 2> NUL

rem echo waiting for pdb > lock.tmp
rem cl %CommonCompilerFlags% -MTd -I..\iaca-win64\ ..\handmade\code\handmade.cpp ..\handmade\code\handmade_msvc.c -Fmhandmade.map -LD /link -incremental:no -opt:ref -PDB:handmade_%random%.pdb -EXPORT:GameGetSoundSamples -EXPORT:GameUpdateAndRender -EXPORT:DEBUGGameFrameEnd
rem del lock.tmp

cl %CommonCompilerFlags% ../source/main.cpp -Fmgiggity.map -link -out:giggity.exe -pdb:giggity_%random%.pdb -subsystem:windows %CommonLinkerFlags%

popd

copy .\build\giggity.exe .\
