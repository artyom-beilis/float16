all: conv_32_16

conv_32_16: conv.h ../include/float16.h conv.c
	gcc -mf16c -Wall -O2 conv.c  -I ../include/  -o conv_32_16
    
