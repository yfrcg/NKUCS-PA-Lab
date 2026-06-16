#include <klib.h>

static void out_char(char **out, size_t *left, int *cnt, char ch) {
  if (*left > 1) {
    **out = ch;
    (*out)++;
    (*left)--;
  }
  (*cnt)++;
}

static void out_str(char **out, size_t *left, int *cnt, const char *s) {
  if (s == NULL) {
    s = "(null)";
  }
  while (*s) {
    out_char(out, left, cnt, *s++);
  }
}

static void out_unsigned(char **out, size_t *left, int *cnt, unsigned int val, int base) {
  char buf[16];
  int i = 0;
  const char *digits = "0123456789abcdef";

  if (val == 0) {
    out_char(out, left, cnt, '0');
    return;
  }

  while (val) {
    buf[i++] = digits[val % base];
    val /= base;
  }
  while (i--) {
    out_char(out, left, cnt, buf[i]);
  }
}

int vsnprintf(char *str, size_t size, const char *format, va_list ap) {
  char *out = str;
  size_t left = size;
  int cnt = 0;

  while (*format) {
    if (*format != '%') {
      out_char(&out, &left, &cnt, *format++);
      continue;
    }

    format++;
    if (*format == '0') {
      format++;
    }
    while (*format >= '0' && *format <= '9') {
      format++;
    }

    switch (*format) {
      case 's':
        out_str(&out, &left, &cnt, va_arg(ap, const char *));
        break;
      case 'c':
        out_char(&out, &left, &cnt, va_arg(ap, int));
        break;
      case 'd': {
        int val = va_arg(ap, int);
        if (val < 0) {
          out_char(&out, &left, &cnt, '-');
          val = -val;
        }
        out_unsigned(&out, &left, &cnt, val, 10);
        break;
      }
      case 'u':
        out_unsigned(&out, &left, &cnt, va_arg(ap, unsigned int), 10);
        break;
      case 'x':
        out_unsigned(&out, &left, &cnt, va_arg(ap, unsigned int), 16);
        break;
      case 'p':
        out_str(&out, &left, &cnt, "0x");
        out_unsigned(&out, &left, &cnt, (uintptr_t)va_arg(ap, void *), 16);
        break;
      case '%':
        out_char(&out, &left, &cnt, '%');
        break;
      default:
        out_char(&out, &left, &cnt, '%');
        if (*format) {
          out_char(&out, &left, &cnt, *format);
        }
        break;
    }
    if (*format) {
      format++;
    }
  }

  if (size > 0) {
    *out = '\0';
  }
  return cnt;
}

int snprintf(char* s, size_t n, const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  int ret = vsnprintf(s, n, format, ap);
  va_end(ap);
  return ret;
}

int vsprintf(char *str, const char *format, va_list ap) {
  return vsnprintf(str, (size_t)-1, format, ap);
}

int sprintf(char* out, const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  int ret = vsprintf(out, format, ap);
  va_end(ap);
  return ret;
}

int printf(const char* fmt, ...) {
  char buf[1024];
  va_list ap;
  va_start(ap, fmt);
  int ret = vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);

  for (char *p = buf; *p; p++) {
    _putc(*p);
  }
  return ret;
}

int sscanf(const char *str, const char *format, ...) {
  (void)str;
  (void)format;
  return 0;
}
