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

    int src_x = 0;
    int dst_x = x;
    int copy_w = w;

    if (dst_x < 0) {
      src_x = -dst_x;
      copy_w -= src_x;
      dst_x = 0;
    }
    if (dst_x + copy_w > _screen.width) {
      copy_w = _screen.width - dst_x;
    }
    if (copy_w <= 0) {
      continue;
    }

    memcpy(&fb[dst_y * _screen.width + dst_x],
        &pixels[row * w + src_x],
        copy_w * sizeof(uint32_t));
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
