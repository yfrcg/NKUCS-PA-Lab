#include <klib.h>

float fabsf(float x) { return x < 0 ? -x : x; }
float floorf(float x) { int i = (int)x; return (x < 0 && x != i) ? i - 1 : i; }
float ceilf(float x) { int i = (int)x; return (x > 0 && x != i) ? i + 1 : i; }
float sqrtf(float x) {
  if (x <= 0) return 0;
  float y = x;
  for (int i = 0; i < 16; i++) {
    y = 0.5f * (y + x / y);
  }
  return y;
}
float fmodf(float x, float y) { return x - (int)(x / y) * y; }
float sinf(float x) { (void)x; return 0; }
float cosf(float x) { (void)x; return 1; }
float powf(float x, float y) {
  int n = (int)y;
  float r = 1;
  while (n-- > 0) r *= x;
  return r;
}
