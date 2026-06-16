#include <klib.h>

static unsigned long seed = 1;

int atoi(const char* nptr) {
  int sign = 1;
  int val = 0;
  while (*nptr == ' ' || *nptr == '\t' || *nptr == '\n') {
    nptr++;
  }
  if (*nptr == '-') {
    sign = -1;
    nptr++;
  }
  while (*nptr >= '0' && *nptr <= '9') {
    val = val * 10 + *nptr - '0';
    nptr++;
  }
  return sign * val;
}

int abs(int x) {
  return x < 0 ? -x : x;
}

unsigned long time() {
  return _uptime();
}

void srand(unsigned int s) {
  seed = s;
}

int rand() {
  seed = seed * 1103515245 + 12345;
  return (seed >> 16) & 0x7fff;
}

void qsort(void *base, size_t nmemb, size_t size, int (*compar)(const void *, const void *)) {
  char *arr = base;
  char tmp[64];
  assert(size <= sizeof(tmp));
  for (size_t i = 0; i < nmemb; i++) {
    for (size_t j = i + 1; j < nmemb; j++) {
      char *a = arr + i * size;
      char *b = arr + j * size;
      if (compar(a, b) > 0) {
        memcpy(tmp, a, size);
        memcpy(a, b, size);
        memcpy(b, tmp, size);
      }
    }
  }
}

void *kalloc(size_t size) {
  (void)size;
  return NULL;
}

void kfree(void *ptr) {
  (void)ptr;
}
