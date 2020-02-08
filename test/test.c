#include "comp.h"
#include <assert.h>
#include <string.h>
#include <emmintrin.h>
#include <immintrin.h>
#include <stdio.h>
#include <float.h>
#include <math.h>
#include "float16.h"


#include "conv.h"


unsigned short randv(unsigned short *seed)
{
    unsigned short lfsr = *seed;
    unsigned short bit;
    bit = ((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5));
    lfsr = (lfsr >> 1) | (bit << 15);
    *seed = lfsr;
    return lfsr;
}

float16_t calc_reference(int op,float16_t a,float16_t b)
{
    float v1=float16_to_float32(a);
    float v2=float16_to_float32(b);
    switch(op) {
    case '+': v1+=v2; break;
    case '-': v1-=v2; break;
    case '*': v1*=v2; break;
    case '/': v1/=v2; break;
    case '~': v1 = -v1; break;
    case '>': return v1 > v2; break;
    case '<': return v1 < v2; break;
    case '!': return v1 != v2; break;
    case '=': return v1 == v2; break;
    case 'G': return v1 >= v2; break;
    case 'L': return v1 <= v2; break;
    }
    return float32_to_float16(v1);
}

float16_t calc_actual(F16Comp *impl,int op,float16_t a,float16_t b)
{
    switch(op) {
    case '+': return impl->add(a,b); break;
    case '-': return impl->sub(a,b); break;
    case '*': return impl->mul(a,b); break;
    case '/': return impl->div(a,b); break;
    case '>': return impl->gt(a,b);  break;
    case '~': return impl->neg(a);   break;
    case '<': return impl->lt(a,b);  break;
    case '=': return impl->eq(a,b);  break;
    case '!': return impl->neq(a,b);  break;
    case 'G': return impl->gte(a,b);  break;
    case 'L': return impl->lte(a,b);  break;
    }
    return -1;
}


void test_op(F16Comp *impl,char op,float16_t a,float16_t b,int strict)
{
    float16_t ref = calc_reference(op,a,b);
    float16_t res = calc_actual(impl,op,a,b);

    if(ref == res)
        return;
    
    unsigned short sig_ref = ref & 0x8000;
    unsigned short sig_res = res & 0x8000;
    unsigned short abs_ref = ref & 0x7FFF;
    unsigned short abs_res = res & 0x7FFF;
    short adiff = abs_ref - abs_res;

    if(abs_ref == 0 && abs_res == 0)  // ignore sign difference
        return;
    if(abs_ref > 0x7C00 && abs_res > 0x7C00)
        return; // nans equal

    int fail=0;
    if(sig_ref != sig_res) 
        fail=1;
    if((strict && adiff!=0) || (!strict && abs(adiff)>1))
        fail=1;
    if(!fail)
        return;

    fprintf(stderr,"%10.10f %c %10.10f = %10.10f != %10.10f expected; distance=%d ticks\n",
            float16_to_float32(a),op,float16_to_float32(b),
            float16_to_float32(res),float16_to_float32(ref),adiff);
    fprintf(stderr,"A=%u %x; B=%u %x\n",a,a,b,b);
    exit(1);
}


void test_general(F16Comp *impl,float16_t a,float16_t b)
{
    test_op(impl,'+',a,b,0);
    test_op(impl,'-',a,b,0);
    test_op(impl,'*',a,b,0);
    test_op(impl,'/',a,b,0);
    test_op(impl,'>',a,b,1);
    test_op(impl,'<',a,b,1);
    test_op(impl,'G',a,b,1);
    test_op(impl,'L',a,b,1);
    test_op(impl,'=',a,b,1);
    test_op(impl,'!',a,b,1);
    test_op(impl,'~',a,b,1);
}

void invalids_test(F16Comp *impl)
{
    //                            -nan   -nan2   -inf     -1,  -HALF_MIN, -0,   0 HALF_MIN , 1,    +inf  , +nan2, +nan
    unsigned short values[] = { 0xFFFF, 0xFC01, 0xFC00, 0xBC00, 0x8001, 0x8000, 0, 1,       0x3C00,0x7C00,0x7C01,0x7FFF };
    for(unsigned i=0;i<sizeof(values)/sizeof(values[0]);i++) {
        for(unsigned j=0;j<sizeof(values)/sizeof(values[0]);j++) {
            float16_t a=values[i];
            float16_t b=values[j];
            test_op(impl,'+',a,b,0);
            test_op(impl,'-',a,b,0);
            test_op(impl,'*',a,b,0);
            test_op(impl,'/',a,b,0);
            test_op(impl,'>',a,b,1);
            test_op(impl,'<',a,b,1);
            test_op(impl,'G',a,b,1);
            test_op(impl,'L',a,b,1);
            test_op(impl,'=',a,b,1);
            test_op(impl,'!',a,b,1);
            test_op(impl,'~',a,b,1);
        }
    }
}

void sanity_test_run(F16Comp *impl)
{
    unsigned short s1=0xACE1;
    unsigned short s2=0x1d24;

    
    for(int i=0;i<10000;i++) {
        randv(&s1);
        randv(&s2);
        if((s1 & 0x7C00) == 0x7C00 || (s2 & 0x7C00) == 0x7C00){
            i--;
            continue;
        }
        
        test_general(impl,s1,s2);
    }
    for(short a=0;a<0x7C00;) {
        for(short b=0;b<0x7C00;) {
            short na=a|0x8000;
            short nb=b|0x8000;
            test_general(impl,a, b );
            test_general(impl,a, nb);
            test_general(impl,na,a );
            test_general(impl,na,nb);
            if(b < 10)
                b++;
            else if(b < 100)
                b += 1 + randv(&s2) % 10;
            else
                b += 1 + randv(&s2) % 100;
        }
        if(a < 10)
            a++;
        else if(a < 100)
            a += 1 + randv(&s1) % 10;
        else
            a += 1 + randv(&s1) % 100;
    }
}

short fix_sign(short i)
{
    if(i < 0) {
        return (-i) | 0x8000;
    }
    return i;
}

void test_subnormals(F16Comp *impl)
{
    for(short i=-2047;i<2048;i++) {
        for(short j=-2047;j<2048;j++) {
            short si=fix_sign(i),sj=fix_sign(j);
            float16_t a=si,b=sj;
            if(abs(i+j) < 2048)
                test_op(impl,'+',a,b,1);
            if(abs(i-j) < 2048)
                test_op(impl,'-',a,b,1);
            
            test_op(impl,'>',a,b,1);
            test_op(impl,'<',a,b,1);
            test_op(impl,'G',a,b,1);
            test_op(impl,'L',a,b,1);
            test_op(impl,'=',a,b,1);
            test_op(impl,'!',a,b,1);
            test_op(impl,'~',a,b,1);

            b = float32_to_float16(j);

            if(abs(i*j) <= 2047)
                test_op(impl,'*',a,b,1);
            if(j!=0 && abs(i) % abs(j) == 0)
                test_op(impl,'/',a,b,1);
        }
    }
}

void test_integers(F16Comp *impl)
{
    for(short i=-2047;i<2048;i++) {
        float16_t a = float32_to_float16(i);
        test_op(impl,'~',a,0,1);
        for(short j=-2047;j<2048;j++) {
            float16_t b = float32_to_float16(j);
            if(abs(i+j) < 2048)
                test_op(impl,'+',a,b,1);
            if(abs(i-j) < 2048)
                test_op(impl,'-',a,b,1);
            if(abs(i*j) < 2048)
                test_op(impl,'*',a,b,1);
            if(j!=0 && abs(i) % abs(j) == 0)
                test_op(impl,'/',a,b,1);
            test_op(impl,'>',a,b,1);
            test_op(impl,'<',a,b,1);
            test_op(impl,'G',a,b,1);
            test_op(impl,'L',a,b,1);
            test_op(impl,'=',a,b,1);
            test_op(impl,'!',a,b,1);
            test_op(impl,'~',a,b,1);
        }
    }
}

void test_int(F16Comp *impl)
{
    for(unsigned short i=0;i<0x7C00;i++) {
        long act = impl->to_int(i);
        long ref = (long)(float16_to_float32(i));
        if(ref!=act) {
            fprintf(stderr,"Int failed for %x int==%ld got=%ld\n",i,ref,act);
            exit(1);
        }
    }
    for(long v=-32768;v<=32767;v++) {
        unsigned short act = impl->from_int(v);
        unsigned short ref = float32_to_float16(v);
        if(ref!=act) {
            printf("EEEEEEEEEE:%x\n",act);
            fprintf(stderr,"From Int failed for %ld int==(%f)%x got=(%f)%x\n",v,float16_to_float32(ref),ref,float16_to_float32(act),act);
            exit(1);
        }
    }
}

void run_test(F16Comp *impl)
{
    printf("Casting to/from integer\n");
    test_int(impl);
    printf("Infs/NaNs\n");
    invalids_test(impl);
    printf("General\n");
    sanity_test_run(impl);
    printf("Integers\n");
    test_integers(impl);
    printf("Subnormals\n");
    test_subnormals(impl);
 }
