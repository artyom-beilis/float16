#include "float16.h"
#include "comp.h"

int main()
{
    F16Comp cmp = {
        .add = f16_add,
        .sub = f16_sub,
        .mul = f16_mul,
        .div = f16_div,
        .neg = f16_neg,
        .to_int = f16_int,
        .from_int = f16_from_int,
        .gte = f16_gte,
        .lte = f16_lte,
        .gt = f16_gt,
        .lt = f16_lt,
        .eq = f16_eq,
        .neq = f16_neq
    };
    run_test(&cmp);
}

