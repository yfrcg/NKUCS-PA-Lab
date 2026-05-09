#include "common.h"

#define NAME(key) \
  [_KEY_##key] = #key,

static const char *keyname[256] __attribute__((used)) = {
  [_KEY_NONE] = "NONE",
  _KEYS(NAME)
};

size_t events_read(void *buf, size_t len) {
  int key = _read_key();

  if (key == _KEY_NONE) {
    return 0;
  }

  const char *type = (key & 0x8000) ? "kd" : "ku";
  int code = key & ~0x8000;

  if (code < 0 || code >= 256 || keyname[code] == NULL) {
    return 0;
  }

  int n = snprintf((char *)buf, len, "%s %s\n", type, keyname[code]);
  return n < 0 ? 0 : n;
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