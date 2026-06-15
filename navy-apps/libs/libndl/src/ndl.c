#include <ndl.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

static int has_nwm = 0;
static uint32_t *canvas = NULL;
static FILE *fbdev = NULL;
static int evtfd = -1;

static void get_display_info();
static int canvas_w, canvas_h, screen_w, screen_h, pad_x, pad_y;
static int dirty_valid, dirty_x1, dirty_y1, dirty_x2, dirty_y2;
static uint32_t *padded_rows = NULL;
static int padded_capacity = 0;

static void mark_dirty(int x, int y, int w, int h) {
  if (w <= 0 || h <= 0) {
    return;
  }

  int x1 = x;
  int y1 = y;
  int x2 = x + w;
  int y2 = y + h;

  if (x1 < 0) x1 = 0;
  if (y1 < 0) y1 = 0;
  if (x2 > canvas_w) x2 = canvas_w;
  if (y2 > canvas_h) y2 = canvas_h;

  if (x1 >= x2 || y1 >= y2) {
    return;
  }

  if (!dirty_valid) {
    dirty_x1 = x1;
    dirty_y1 = y1;
    dirty_x2 = x2;
    dirty_y2 = y2;
    dirty_valid = 1;
  } else {
    if (x1 < dirty_x1) dirty_x1 = x1;
    if (y1 < dirty_y1) dirty_y1 = y1;
    if (x2 > dirty_x2) dirty_x2 = x2;
    if (y2 > dirty_y2) dirty_y2 = y2;
  }
}

int NDL_OpenDisplay(int w, int h) {
  if (canvas) {
    NDL_CloseDisplay();
  }

  canvas_w = w;
  canvas_h = h;
  canvas = malloc(sizeof(uint32_t) * w * h);
  assert(canvas);
  dirty_valid = 0;

#ifdef __ISA_NATIVE__
  has_nwm = getenv("NWM_APP") ? 1 : 0;
#else
  has_nwm = 0;
#endif

  if (has_nwm) {
    printf("\033[X%d;%ds", w, h);
    fflush(stdout);
    evtfd = 0;
  } else {
    get_display_info();
    assert(screen_w >= canvas_w);
    assert(screen_h >= canvas_h);

    pad_x = (screen_w - canvas_w) / 2;
    pad_y = (screen_h - canvas_h) / 2;

    fbdev = fopen("/dev/fb", "w");
    assert(fbdev);

    evtfd = open("/dev/events", 0);
    assert(evtfd >= 0);
  }

  return 0;
}

int NDL_CloseDisplay() {
  if (canvas) {
    free(canvas);
    canvas = NULL;
  }
  if (padded_rows) {
    free(padded_rows);
    padded_rows = NULL;
    padded_capacity = 0;
  }

  if (fbdev) {
    fclose(fbdev);
    fbdev = NULL;
  }

  if (evtfd >= 0 && evtfd != 0) {
    close(evtfd);
  }
  evtfd = -1;

  return 0;
}

int NDL_DrawRect(uint32_t *pixels, int x, int y, int w, int h) {
  if (has_nwm) {
    for (int i = 0; i < h; i ++) {
      printf("\033[X%d;%d", x, y + i);
      for (int j = 0; j < w; j ++) {
        putchar(';');
        fwrite(&pixels[i * w + j], 1, 4, stdout);
      }
      printf("d\n");
    }
  } else {
    int x1 = x;
    int y1 = y;
    int x2 = x + w;
    int y2 = y + h;

    if (x1 < 0) x1 = 0;
    if (y1 < 0) y1 = 0;
    if (x2 > canvas_w) x2 = canvas_w;
    if (y2 > canvas_h) y2 = canvas_h;

    for (int row = y1; row < y2; row ++) {
      memcpy(&canvas[row * canvas_w + x1],
          &pixels[(row - y) * w + (x1 - x)],
          (x2 - x1) * sizeof(uint32_t));
    }
    mark_dirty(x1, y1, x2 - x1, y2 - y1);
  }

  return 0;
}

int NDL_Render() {
  if (has_nwm) {
    fflush(stdout);
  } else {
    assert(fbdev);

    if (!dirty_valid) {
      return 0;
    }

    int dirty_w = dirty_x2 - dirty_x1;
    int dirty_h = dirty_y2 - dirty_y1;

    if (dirty_w == canvas_w) {
      if (canvas_w == screen_w && pad_x == 0) {
        fseek(fbdev, (dirty_y1 + pad_y) * screen_w * sizeof(uint32_t), SEEK_SET);
        fwrite(&canvas[dirty_y1 * canvas_w], sizeof(uint32_t), canvas_w * dirty_h, fbdev);
      } else {
        int nr_pixels = screen_w * dirty_h;

        if (padded_capacity < nr_pixels) {
          padded_rows = realloc(padded_rows, nr_pixels * sizeof(uint32_t));
          assert(padded_rows);
          padded_capacity = nr_pixels;
        }

        for (int row = 0; row < dirty_h; row ++) {
          uint32_t *dst = &padded_rows[row * screen_w];
          memset(dst, 0, screen_w * sizeof(uint32_t));
          memcpy(&dst[pad_x], &canvas[(dirty_y1 + row) * canvas_w],
              canvas_w * sizeof(uint32_t));
        }

        fseek(fbdev, (dirty_y1 + pad_y) * screen_w * sizeof(uint32_t), SEEK_SET);
        fwrite(padded_rows, sizeof(uint32_t), nr_pixels, fbdev);
      }
    } else {
      for (int i = dirty_y1; i < dirty_y2; i ++) {
        fseek(fbdev, ((i + pad_y) * screen_w + pad_x + dirty_x1) * sizeof(uint32_t), SEEK_SET);
        fwrite(&canvas[i * canvas_w + dirty_x1], sizeof(uint32_t), dirty_w, fbdev);
      }
    }

    fflush(fbdev);
    dirty_valid = 0;
  }

  return 0;
}

#define keyname(k) #k,

static const char *keys[] = {
  "NONE",
  _KEYS(keyname)
};

#define numkeys (sizeof(keys) / sizeof(keys[0]))

int NDL_WaitEvent(NDL_Event *event) {
  char buf[256];

  assert(event != NULL);
  assert(evtfd >= 0);

  while (1) {
    int n = read(evtfd, buf, sizeof(buf) - 1);

    if (n <= 0) {
      continue;
    }

    buf[n] = '\0';

    if (buf[0] == 'k') {
      char kname[32];

      event->type = buf[1] == 'd' ? NDL_EVENT_KEYDOWN : NDL_EVENT_KEYUP;
      event->data = NDL_SCANCODE_NONE;

      sscanf(buf + 3, "%s", kname);

      for (int i = 0; i < numkeys; i ++) {
        if (strcmp(keys[i], kname) == 0) {
          event->data = i;
          break;
        }
      }

      return 0;
    }

    if (buf[0] == 't') {
      int tsc = 0;

      sscanf(buf + 2, "%d", &tsc);

      event->type = NDL_EVENT_TIMER;
      event->data = tsc;
      return 0;
    }
  }

  assert(0);
  return -1;
}

static void get_display_info() {
  FILE *dispinfo = fopen("/proc/dispinfo", "r");
  assert(dispinfo);

  screen_w = screen_h = 0;

  char buf[128], key[128], value[128], *delim;

  while (fgets(buf, sizeof(buf), dispinfo)) {
    delim = strchr(buf, ':');
    if (delim == NULL) {
      continue;
    }

    *delim = '\0';

    sscanf(buf, "%s", key);
    sscanf(delim + 1, "%s", value);

    if (strcmp(key, "WIDTH") == 0) {
      sscanf(value, "%d", &screen_w);
    }

    if (strcmp(key, "HEIGHT") == 0) {
      sscanf(value, "%d", &screen_h);
    }
  }

  fclose(dispinfo);

  assert(screen_w > 0 && screen_h > 0);
}
