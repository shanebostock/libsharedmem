TEST_PATH=./test

gcc -Wall -Werror $TEST_PATH/test.c -lshmemory -pthread -o test_build