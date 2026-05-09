#include "common.h"

unsigned long _uptime();

size_t serial_write(const void *buf, off_t offset, size_t len) {
  const char *s = (const char *)buf;

  for (size_t i = 0; i < len; i++) {
    _putc(s[i]);
  }

  return len;
}

#define NAME(key) \
  [_KEY_##key] = #key,

static const char *keyname[256] __attribute__((used)) = {
  [_KEY_NONE] = "NONE",
  _KEYS(NAME)
};

static char event_line[64];
static size_t event_pos = 0;
static size_t event_len = 0;
static unsigned int last_time = 0;

static void make_event_line() {
  int key = _read_key();

  if (key != _KEY_NONE) {
    const char *type = (key & 0x8000) ? "kd" : "ku";
    int code = key & ~0x8000;

    if (code >= 0 && code < 256 && keyname[code] != NULL) {
      sprintf(event_line, "%s %s\n", type, keyname[code]);
      event_len = strlen(event_line);
      event_pos = 0;
      return;
    }
  }

  unsigned int now = (unsigned int)_uptime();

  if (now <= last_time) {
    now = last_time + 1;
  }
  last_time = now;

  sprintf(event_line, "t %u\n", now);
  event_len = strlen(event_line);
  event_pos = 0;
}

size_t events_read(void *buf, off_t offset, size_t len) {
  if (len == 0) {
    return 0;
  }

  char *out = (char *)buf;
  size_t total = 0;

  while (total < len) {
    if (event_pos >= event_len) {
      make_event_line();

      if (event_len == 0) {
        break;
      }
    }

    size_t rest = event_len - event_pos;
    size_t room = len - total;
    size_t n = rest < room ? rest : room;

    memcpy(out + total, event_line + event_pos, n);
    event_pos += n;
    total += n;

    if (total > 0 && event_pos >= event_len) {
      break;
    }
  }

  return total;
}

static char dispinfo[128] __attribute__((used));

size_t dispinfo_read(void *buf, off_t offset, size_t len) {
  size_t size = strlen(dispinfo);

  if ((size_t)offset >= size) {
    return 0;
  }

  if ((size_t)offset + len > size) {
    len = size - offset;
  }

  memcpy(buf, dispinfo + offset, len);
  return len;
}

size_t fb_write(const void *buf, off_t offset, size_t len) {
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

  return len;
}

size_t fbsync_write(const void *buf, off_t offset, size_t len) {
  _draw_sync();
  return len;
}

void init_device() {
  _ioe_init();

  memset(dispinfo, 0, sizeof(dispinfo));
  sprintf(dispinfo, "WIDTH:%d\nHEIGHT:%d\n", _screen.width, _screen.height);
}
