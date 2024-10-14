#ifndef DES_RK45_H
#define DES_RK45_H

#include "../des_structs.h"

#ifdef __cplusplus
extern "C" {
#endif

double rk45_step(DES *des, const double t, const double dt);

void rk45_error();

void rk45(DES *des, const double tf);

#ifdef __cplusplus
}
#endif

#endif // DES_RK45_H
