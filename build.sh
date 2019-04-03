#!/bin/sh

CommonCompilerFlags="-O2"
CommonCompilerFlags="$CommonCompilerFlags -v -g -std=c++11"
CommonCompilerFlags="$CommonCompilerFlags -I../vendor/SDL2 -I../vendor/glew/include -lSDL2 -lGL -lGLEW -L../vendor/glew/lib"

if ! [ -d ./build ]; then
    mkdir ./build
fi
cd ./build

#rm *.pdb > NUL 2> NUL

# echo waiting for pdb > lock.tmp
# cl %CommonCompilerFlags% -MTd -I..\iaca-win64\ ..\handmade\code\handmade.cpp ..\handmade\code\handmade_msvc.c -Fmhandmade.map -LD /link -incremental:no -opt:ref -PDB:handmade_%random%.pdb -EXPORT:GameGetSoundSamples -EXPORT:GameUpdateAndRender -EXPORT:DEBUGGameFrameEnd
# del lock.tmp

/bin/g++ $CommonCompilerFlags -o giggity.out ../source/main.cpp

cd ..

cp ./build/giggity.out ./
