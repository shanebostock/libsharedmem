#!/bin/bash

INCLUDES_PATH=/usr/local/include
LIBS_PATH=/usr/local/lib
CONF_PATH=/etc/ld.so.conf.d
SRC_PATH=./src
TEST_PATH=./test

echo building libshmemory...
bash build.sh
echo complete.

echo installing libshmemory...
echo copying incldues to $INCLUDES_PATH...
sudo cp $SRC_PATH/*.h $INCLUDES_PATH
sudo cp libshmemory.so $LIBS_PATH

echo setting up linker for libshmemory
sudo cp libshmemory.conf $CONF_PATH
sudo ldconfig

echo building tests...
bash build_test.sh

echo running tests...
./test_build

echo cleaning up...
rm *.so
rm *.o
rm test_build
echo complete.