#!/bin/sh

# -Og or -O2 here
# -v
CommonCompilerFlags="-Og"
CommonCompilerFlags="$CommonCompilerFlags -g -v -std=c++14 -fno-rtti -fext-numeric-literals"
CommonCompilerFlags="$CommonCompilerFlags -I../vendor/SDL2 -I../vendor/glew/include"

if ! [ -d ./build ]; then
    mkdir ./build
fi
cd ./build

echo "waiting for pdb" > lock.tmp
/bin/g++ $CommonCompilerFlags -fPIC -shared -rdynamic -nostartfiles -o game.so ../source/game.cpp  -lSDL2 -lGL -lGLEW -L../vendor/glew/lib
rm lock.tmp

/bin/g++ $CommonCompilerFlags -o giggity.out ../source/main.cpp -lSDL2 -lGL -lGLEW -L../vendor/glew/lib

cd ..

cp ./build/giggity.out ./
cp ./build/game.so ./