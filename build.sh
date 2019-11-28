#!/bin/sh

# -Od or -O2 here
# -v
CommonCompilerFlags="-O2"
CommonCompilerFlags="$CommonCompilerFlags -g -std=c++11 -fno-rtti -fext-numeric-literals"
CommonCompilerFlags="$CommonCompilerFlags -I../vendor/SDL2 -I../vendor/glew/include"

if ! [ -d ./build ]; then
    mkdir ./build
fi
cd ./build

echo "waiting for pdb" > lock.tmp
/bin/g++ $CommonCompilerFlags -fPIC -shared -rdynamic -nostartfiles -o game.so ../source/game.cpp  -lSDL2 -lGL
rm lock.tmp

/bin/g++ $CommonCompilerFlags -o giggity.out ../source/main.cpp -lSDL2 -lGL -lGLEW -L../vendor/glew/lib

cd ..

cp ./build/giggity.out ./
cp ./build/game.so ./