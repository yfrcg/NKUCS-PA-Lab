#include "common.h"

unsigned long _uptime();

#define NAME(key) \
  [_KEY_##key] = #key,

static const char *keyname[256] __attribute__((used)) = {
  [_KEY_NONE] = "NONE",
  _KEYS(NAME)
};

size_t events_read(void *buf, size_t len) {
  static char stream[64];
  static size_t pos = 0;
  static size_t size = 0;
  static unsigned int last_time = 0;

  if (len == 0) {
    return 0;
  }

  if (pos >= size) {
    int key = _read_key();

    if (key != _KEY_NONE) {
      const char *type = (key & 0x8000) ? "kd" : "ku";
      int code = key & ~0x8000;

      if (code >= 0 && code < 256 && keyname[code] != NULL) {
        int n = snprintf(stream, sizeof(stream), "%s %s\n", type, keyname[code]);
        size = n > 0 ? (size_t)n : 0;
      } else {
        size = 0;
      }
    }

    if (size == 0) {
      unsigned int now = (unsigned int)_uptime();

      if (now <= last_time) {
        now = last_time + 1;
      }
      last_time = now;

      int n = snprintf(stream, sizeof(stream), "t %u\n", now);
      size = n > 0 ? (size_t)n : 0;
    }

    if (size >= sizeof(stream)) {
      size = sizeof(stream) - 1;
    }

    pos = 0;
  }

  size_t rest = size - pos;
  size_t nread = len < rest ? len : rest;

  memcpy(buf, stream + pos, nread);
  pos += nread;

  return nread;
}

static char dispinfo[128] __attribute__((used));

void dispinfo_read(void *buf, off_t offset, size_t len) {
  size_t size = strlen(dispinfo);

  if (offset >= size) {
    return;
  }

  if (offset + len > size) {
    len = size - offset;
  }

  memcpy(buf, dispinfo + offset, len);
}

void fb_write(const void *buf, off_t offset, size_t len) {
  const uint32_t *pixels = (const uint32_t *)buf;

  int width = _screen.width;
  int height = _screen.height;

  int pixel_offset = offset / 4;
  int nr_pixels = len / 4;

  while (nr_pixels > 0) {
    int x = pixel_offset % width;
    int y = pixel_offset / width;

    if (y >= height) {
      break;
    }

    int n = width - x;
    if (n > nr_pixels) {
      n = nr_pixels;
    }

    _draw_rect(pixels, x, y, n, 1);

    pixels += n;
    pixel_offset += n;
    nr_pixels -= n;
  }
}

void init_device() {
  _ioe_init();

  snprintf(dispinfo, sizeof(dispinfo),
      "WIDTH:%d\nHEIGHT:%d\n", _screen.width, _screen.height);
}