TEST_PATH=./test

gcc -Wall -Werror $TEST_PATH/test.c -lshmemory -pthread -o test_build
gcc -Wall -Werror $TEST_PATH/rb_test.c -lshmemory -pthread -o rb_test_build