all: test_c

test_c: test.c ../test/test.c float16.c ../include/float16.h ../test/comp.h
	gcc -mf16c -Wall -O2 test.c -I ../test/ -I ../include/  ../test/test.c float16.c  -o test_c
    
