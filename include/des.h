#ifndef DES_H
#define DES_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "des_structs.h"

#ifdef __cplusplus
extern "C" {
#endif

// Function prototypes
int DES_INIT(
    DES *des,
    void (*func)(const double, const double[], double[]),
    const size_t num_eqs,
    const double init_cond[],
    const DES_CONFIG config
);

int DES_SOLVE(DES *des, const double t0, const double tf, const int samples, DES_METHOD method);

void DES_FREE(DES *des);

#ifdef __cplusplus
}
#endif

#endif // DES_H

