#ifndef COMP_H
#define COMP_H
#include <stdint.h>

typedef struct F16Comp {
    short (*add)(short,short);
    short (*sub)(short,short);
    short (*mul)(short,short);
    short (*div)(short,short);
    short (*neg)(short);
    int32_t  (*to_int)(short);
    short (*from_int)(int32_t);
    int   (*gte)(short,short);
    int   (*lte)(short,short);
    int   (*gt)(short,short);
    int   (*lt)(short,short);
    int   (*eq)(short,short);
    int   (*neq)(short,short);
} F16Comp;
void run_test(F16Comp *imp);
#endif
