#ifndef __FLOAT_H__
#define __FLOAT_H__

#include "assert.h"
#include <stdint.h>

typedef int FLOAT;

#define FLOAT_FBITS 16
#define FLOAT_SCALE (1 << FLOAT_FBITS)

static inline int F2int(FLOAT a) {
  return a / FLOAT_SCALE;
}

static inline FLOAT int2F(int a) {
  return (FLOAT)(a * FLOAT_SCALE);
}

static inline FLOAT F_mul_int(FLOAT a, int b) {
  return a * b;
}

static inline FLOAT F_div_int(FLOAT a, int b) {
  assert(b != 0);
  return a / b;
}

FLOAT f2F(float);
FLOAT F_mul_F(FLOAT, FLOAT);
FLOAT F_div_F(FLOAT, FLOAT);
FLOAT Fabs(FLOAT);
FLOAT Fsqrt(FLOAT);
FLOAT Fpow(FLOAT, FLOAT);

#endif
