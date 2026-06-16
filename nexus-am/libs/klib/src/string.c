#include <klib.h>

void* memset(void* v, int c, size_t n) {
  unsigned char *p = v;
  while (n--) {
    *p++ = (unsigned char)c;
  }
  return v;
}

void* memcpy(void* dst, const void* src, size_t n) {
  unsigned char *d = dst;
  const unsigned char *s = src;
  while (n--) {
    *d++ = *s++;
  }
  return dst;
}

void* memmove(void* dst, const void* src, size_t n) {
  unsigned char *d = dst;
  const unsigned char *s = src;
  if (d < s) {
    while (n--) {
      *d++ = *s++;
    }
  } else if (d > s) {
    d += n;
    s += n;
    while (n--) {
      *--d = *--s;
    }
  }
  return dst;
}

int memcmp(const void* s1, const void* s2, size_t n) {
  const unsigned char *p1 = s1;
  const unsigned char *p2 = s2;
  while (n--) {
    if (*p1 != *p2) {
      return *p1 - *p2;
    }
    p1++;
    p2++;
  }
  return 0;
}

size_t strlen(const char* s) {
  const char *p = s;
  while (*p) {
    p++;
  }
  return p - s;
}

char* strcpy(char* dst, const char* src) {
  char *ret = dst;
  while ((*dst++ = *src++) != '\0');
  return ret;
}

char* strncpy(char* dst, const char* src, size_t n) {
  char *ret = dst;
  while (n && *src) {
    *dst++ = *src++;
    n--;
  }
  while (n--) {
    *dst++ = '\0';
  }
  return ret;
}

char* strcat(char* dst, const char* src) {
  strcpy(dst + strlen(dst), src);
  return dst;
}

int strcmp(const char* s1, const char* s2) {
  while (*s1 && *s1 == *s2) {
    s1++;
    s2++;
  }
  return (unsigned char)*s1 - (unsigned char)*s2;
}

int strncmp(const char* s1, const char* s2, size_t n) {
  while (n && *s1 && *s1 == *s2) {
    s1++;
    s2++;
    n--;
  }
  if (n == 0) {
    return 0;
  }
  return (unsigned char)*s1 - (unsigned char)*s2;
}

const char *strchr(const char *s, int c) {
  while (*s) {
    if (*s == (char)c) {
      return s;
    }
    s++;
  }
  return c == 0 ? s : NULL;
}

char *strstr(const char *haystack, const char *needle) {
  if (*needle == '\0') {
    return (char *)haystack;
  }
  for (; *haystack; haystack++) {
    const char *h = haystack;
    const char *n = needle;
    while (*h && *n && *h == *n) {
      h++;
      n++;
    }
    if (*n == '\0') {
      return (char *)haystack;
    }
  }
  return NULL;
}

char* strtok(char* s, const char* delim) {
  static char *save;
  if (s == NULL) {
    s = save;
  }
  if (s == NULL) {
    return NULL;
  }
  while (*s && strchr(delim, *s)) {
    s++;
  }
  if (*s == '\0') {
    save = NULL;
    return NULL;
  }
  char *ret = s;
  while (*s && !strchr(delim, *s)) {
    s++;
  }
  if (*s) {
    *s++ = '\0';
  }
  save = s;
  return ret;
}
