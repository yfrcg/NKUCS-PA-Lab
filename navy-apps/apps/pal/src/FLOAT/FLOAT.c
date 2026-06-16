#include "FLOAT.h"
#include <stdint.h>
#include <assert.h>

#define FLOAT_EPSILON 6

FLOAT F_mul_F(FLOAT a, FLOAT b) {
  return (FLOAT)(((int64_t)a * b) >> FLOAT_FBITS);
}

FLOAT F_div_F(FLOAT a, FLOAT b) {
  assert(b != 0);
  return (FLOAT)(((int64_t)a << FLOAT_FBITS) / b);
}

FLOAT f2F(float a) {
  /* You should figure out how to convert `a' into FLOAT without
   * introducing x87 floating point instructions. Else you can
   * not run this code in NEMU before implementing x87 floating
   * point instructions, which is contrary to our expectation.
   *
   * Hint: The bit representation of `a' is already on the
   * stack. How do you retrieve it to another variable without
   * performing arithmetic operations on it directly?
   */

  uint32_t bits = *(uint32_t *)&a;
  uint32_t sign = bits >> 31;
  uint32_t raw_exp = (bits >> 23) & 0xff;
  uint32_t frac = bits & 0x7fffff;

  if (raw_exp == 0 && frac == 0) {
    return 0;
  }
  assert(raw_exp != 0xff);

  int exp;
  uint32_t mantissa;
  if (raw_exp == 0) {
    exp = -126;
    mantissa = frac;
  }
  else {
    exp = (int)raw_exp - 127;
    mantissa = (1 << 23) | frac;
  }

  int shift = exp - (23 - FLOAT_FBITS);
  int64_t value = (shift >= 0) ? ((int64_t)mantissa << shift)
                               : ((int64_t)mantissa >> -shift);
  return (FLOAT)(sign ? -value : value);
}

FLOAT Fabs(FLOAT a) {
  return a < 0 ? -a : a;
}

/* Functions below are already implemented */

FLOAT Fsqrt(FLOAT x) {
  FLOAT dt, t = int2F(2);

  do {
    dt = F_div_int((F_div_F(x, t) - t), 2);
    t += dt;
  } while(Fabs(dt) > FLOAT_EPSILON);

  return t;
}

FLOAT Fpow(FLOAT x, FLOAT y) {
  /* we only compute x^0.333 */
  FLOAT t2, dt, t = int2F(2);

  do {
    t2 = F_mul_F(t, t);
    dt = (F_div_F(x, t2) - t) / 3;
    t += dt;
  } while(Fabs(dt) > FLOAT_EPSILON);

  return t;
}
