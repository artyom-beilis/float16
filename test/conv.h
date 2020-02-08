#ifndef F1632_CONV_H
#define F1632_CONV_H
#include <emmintrin.h>
#include <immintrin.h>
#include <math.h>
#include <string.h>
typedef struct Float32 {
    union {
        unsigned value;
        float fvalue;
        struct {
            unsigned fraction: 23;
            unsigned exponent: 8;
            unsigned sign: 1;
        };
    } m;
} Float32;
typedef unsigned short float16_t;

float16_t float32_to_float16(float v)
{
    if(isnan(v))
        return 0x7FFF;
    if(isinf(v))
        return v > 0 ? 0x7C00 : 0xFC00;
    Float32 tmp;
    tmp.m.fvalue = v;
    tmp.m.value &= ~(unsigned)((1<<13)-1);
    int new_exp = tmp.m.exponent - 127 + 15;
    if(new_exp >=31)
        return tmp.m.sign ? 0xFC00 : 0x7C00;
    __m128 iv;
    memset(&iv,0,sizeof(iv));
    memcpy(&iv,&v,4);
    __m128i resv = _mm_cvtps_ph(iv,_MM_FROUND_TO_ZERO  |_MM_FROUND_NO_EXC);
    float16_t res;
    memcpy(&res,&resv,2);
    return res;
}


float float16_to_float32(float16_t v)
{
    if((v & 0x7C00) == 0x7C00) {
        if((v & 1023) == 0) {
            return v & 0x8000 ? -INFINITY : INFINITY;
        }
        else
            return NAN;
    }
    __m128i iv;
    memset(&iv,0,sizeof(iv));
    memcpy(&iv,&v,2);
    __m128 resv = _mm_cvtph_ps(iv);
    float res;
    memcpy(&res,&resv,4);
    return res;
}

#endif
