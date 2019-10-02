#!/bin/bash
mkdir -p ../build
pushd ../build
CommonCompilerFlags=(-Wall -Werror -Wl,-rpath,'$ORIGIN'
		     -Wno-unused-function -Wno-write-strings -Wno-unused-variable -g -Wno-null-dereference
		    -Wno-unused-but-set-variable)
CommonLinkerFlags=(-lGL -lGLEW `sdl2-config --cflags --libs` -ldl)

gcc ${CommonCompilerFlags[*]} ../code/main.c -o rtracer ${CommonLinkerFlags[*]}

popd

