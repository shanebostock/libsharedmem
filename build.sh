#!/bin/bash

#Compile libshmemory
echo Compiling libshmemory

SRC_PATH=./src

gcc -c -Wall -Werror -fpic $SRC_PATH/shmemory.c $SRC_PATH/queue.c
gcc -shared -o libshmemory.so shmemory.o queue.o

echo Compile complete