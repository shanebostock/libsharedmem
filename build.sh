#!/bin/bash

#Compile libshmemory
echo Compiling libshmemory

SRC_PATH=./src

gcc -c -Wall -Werror -fpic $SRC_PATH/shmemory.c
gcc -shared -o libshmemory.so shmemory.o

echo Compile complete