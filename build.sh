#!/bin/bash

#Compile libshmemory
echo Compiling libshmemory

SRC_PATH=./src

gcc -c -Wall -Werror -fpic \
    $SRC_PATH/shmemory.c \
    $SRC_PATH/ring_buffer.c 

gcc -shared -o libshmemory.so shmemory.o ring_buffer.o

echo Compile complete