#include "FLOAT.h"
#include <stdint.h>
#include <assert.h>

#define FLOAT_EPSILON 6

static uint32_t F_abs_u32(FLOAT a) {
  return a < 0 ? (uint32_t)(-(a + 1)) + 1 : (uint32_t)a;
}

static uint32_t F_div_u48_u32(uint32_t hi, uint32_t lo, uint32_t divisor) {
  uint32_t quotient = 0;
  uint32_t remainder = 0;

  assert(divisor != 0);

  for (int bit = 47; bit >= 0; bit --) {
    uint32_t next = bit >= 32 ? ((hi >> (bit - 32)) & 1)
                              : ((lo >> bit) & 1);

    remainder = (remainder << 1) | next;
    if (remainder >= divisor) {
      remainder -= divisor;
      assert(bit < 32);
      quotient |= 1u << bit;
    }
  }

  return quotient;
}

FLOAT F_mul_F(FLOAT a, FLOAT b) {
  uint32_t ua = F_abs_u32(a);
  uint32_t ub = F_abs_u32(b);

  uint32_t a_int = ua >> FLOAT_FBITS;
  uint32_t a_frac = ua & (FLOAT_SCALE - 1);
  uint32_t b_int = ub >> FLOAT_FBITS;
  uint32_t b_frac = ub & (FLOAT_SCALE - 1);

  uint32_t result = (a_int * b_int << FLOAT_FBITS)
                  + a_int * b_frac
                  + b_int * a_frac
                  + ((a_frac * b_frac) >> FLOAT_FBITS);

  return ((a < 0) ^ (b < 0)) ? -(FLOAT)result : (FLOAT)result;
}

FLOAT F_div_F(FLOAT a, FLOAT b) {
  assert(b != 0);

  uint32_t ua = F_abs_u32(a);
  uint32_t ub = F_abs_u32(b);
  uint32_t quotient = F_div_u48_u32(ua >> FLOAT_FBITS, ua << FLOAT_FBITS, ub);

  return ((a < 0) ^ (b < 0)) ? -(FLOAT)quotient : (FLOAT)quotient;
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
  uint32_t value;
  if (shift >= 0) {
    assert(shift < 8);
    value = mantissa << shift;
  }
  else {
    int rshift = -shift;
    value = rshift >= 32 ? 0 : mantissa >> rshift;
  }
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
