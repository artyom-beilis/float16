all: float_z80.bin test_z80

test_z80: test.c ../test/test.c ../c_src/float16.c ../include/float16.h ../test/comp.h
	gcc -mf16c -Wall -O2 test.c -I ../test/ -I ../include/  ../test/test.c ../c_src/float16.c -lz80ex -lz80ex_dasm -o test_z80

float_z80.bin: float_z80.asm
	z80asm -r32768 -b -g float_z80.asm
    
