#include <am.h>
#include <x86.h>

#define RTC_PORT 0x48   // Note that this is not standard
#define I8042_DATA_PORT 0x60
#define I8042_STATUS_PORT 0x64
#define I8042_STATUS_HASKEY_MASK 0x1

static unsigned long boot_time;

void _ioe_init() {
  boot_time = inl(RTC_PORT);
}

unsigned long _uptime() {
  return inl(RTC_PORT) - boot_time;
}

uint32_t* const fb = (uint32_t *)0x40000;

_Screen _screen = {
  .width  = 400,
  .height = 300,
};

extern void* memcpy(void *, const void *, int);

void _draw_rect(const uint32_t *pixels, int x, int y, int w, int h) {
    for (int row = 0; row < h; row ++) {
    int dst_y = y + row;
    if (dst_y < 0 || dst_y >= _screen.height) {
      continue;
    }

    for (int col = 0; col < w; col ++) {
      int dst_x = x + col;
      if (dst_x < 0 || dst_x >= _screen.width) {
        continue;
      }
      fb[dst_y * _screen.width + dst_x] = pixels[row * w + col];
    }
  }
}

void _draw_sync() {
}

int _read_key() {
  if ((inb(I8042_STATUS_PORT) & I8042_STATUS_HASKEY_MASK) == 0) {
    return _KEY_NONE;
  }
  return inl(I8042_DATA_PORT);
}
