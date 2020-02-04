#ifndef FLOAT16_H
#define FLOAT16_H

#include <stdint.h>

#define f16_one 15360
#define f16_half 14336

short f16_add(short a,short b);
short f16_sub(short a,short b);
short f16_mul(short a,short b);
short f16_div(short a,short b);
short f16_neg(short a);
short f16_from_int(int32_t v);
int32_t f16_int(short v);

int f16_gte(short a,short b);
int f16_gt(short a,short b);
int f16_eq(short a,short b);
int f16_lte(short a,short b);
int f16_lt(short a,short b);
int f16_neq(short a,short b);


#endif
