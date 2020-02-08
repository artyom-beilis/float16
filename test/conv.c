#include "conv.h"
#include <stdio.h>
#include <string.h>

int main(int argc,char **argv)
{
    int to_half=0;
    if( !(argc == 3 
        && ((to_half = strcmp(argv[1],"-h")==0)
        || strcmp(argv[1],"-f")==0)))    
    {
        fprintf(stderr,"Usage conv_32_16 (-h floating_value | -f half_value)\n");
        return 1;
    }
    if(to_half) {
        unsigned short value = float32_to_float16(atof(argv[2]));
        float actual_value = float16_to_float32(value);
        printf("%s == 0x%x / %u (%.8g)\n",argv[2],value,value,actual_value);
    }
    else {
        float v = float16_to_float32((unsigned short)(atoi(argv[2])));
        printf("%s = %.10f\n",argv[2],v);
    }
}
