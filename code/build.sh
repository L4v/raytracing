#!/bin/bash
mkdir -p ../build
pushd ../build
CommonCompilerFlags=(-Wall -Werror -Wl,-rpath,'$ORIGIN'
		     -Wno-unused-function -Wno-write-strings -Wno-unused-variable -g -Wno-null-dereference
		    -Wno-unused-but-set-variable)
CommonLinkerFlags=(-lm)

gcc ${CommonCompilerFlags[*]} ../code/linux_raytracer.c -o rtracer ${CommonLinkerFlags[*]}

popd

