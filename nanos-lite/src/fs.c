#include "fs.h"

typedef size_t (*ReadFn)(void *buf, off_t offset, size_t len);
typedef size_t (*WriteFn)(const void *buf, off_t offset, size_t len);

typedef struct {
  char *name;
  size_t size;
  off_t disk_offset;
  off_t open_offset;
  ReadFn read;
  WriteFn write;
} Finfo;

enum {
  FD_STDIN,
  FD_STDOUT,
  FD_STDERR,
  FD_EVENTS,
  FD_FB,
  FD_FBSYNC,
  FD_DISPINFO,
  FD_NORMAL
};

static size_t invalid_read(void *buf, off_t offset, size_t len) {
  panic("invalid_read: should not reach here");
  return 0;
}

static size_t invalid_write(const void *buf, off_t offset, size_t len) {
  panic("invalid_write: should not reach here");
  return 0;
}

void ramdisk_read(void *buf, off_t offset, size_t len);
void ramdisk_write(const void *buf, off_t offset, size_t len);

size_t serial_write(const void *buf, off_t offset, size_t len);
size_t events_read(void *buf, off_t offset, size_t len);
size_t dispinfo_read(void *buf, off_t offset, size_t len);
size_t fb_write(const void *buf, off_t offset, size_t len);
size_t fbsync_write(const void *buf, off_t offset, size_t len);

static Finfo file_table[] __attribute__((used)) = {
  {"stdin", 0, 0, 0, invalid_read, invalid_write},
  {"stdout", 0, 0, 0, invalid_read, serial_write},
  {"stderr", 0, 0, 0, invalid_read, serial_write},
  {"/dev/events", 0, 0, 0, events_read, invalid_write},
  {"/dev/fb", 0, 0, 0, invalid_read, fb_write},
  {"/dev/fbsync", 0, 0, 0, invalid_read, fbsync_write},
  {"/proc/dispinfo", 128, 0, 0, dispinfo_read, invalid_write},
#include "files.h"
};

#define NR_FILES (sizeof(file_table) / sizeof(file_table[0]))

void init_fs() {
  file_table[FD_FB].size = _screen.width * _screen.height * sizeof(uint32_t);
}

size_t fs_filesz(int fd) {
  assert(fd >= 0 && fd < NR_FILES);
  return file_table[fd].size;
}

int fs_open(const char *pathname, int flags, int mode) {
  for (int i = 0; i < NR_FILES; i++) {
    if (strcmp(file_table[i].name, pathname) == 0) {
      file_table[i].open_offset = 0;
      return i;
    }
  }

  panic("fs_open: file not found: %s", pathname);
  return -1;
}

size_t fs_read(int fd, void *buf, size_t len) {
  assert(fd >= 0 && fd < NR_FILES);

  size_t true_len = len;

  if (fd != FD_EVENTS) {
    if (file_table[fd].open_offset >= file_table[fd].size) {
      true_len = 0;
    } else if (file_table[fd].open_offset + len > file_table[fd].size) {
      true_len = file_table[fd].size - file_table[fd].open_offset;
    }
  }

  size_t ret;

  if (file_table[fd].read != NULL) {
    ret = file_table[fd].read(buf, file_table[fd].open_offset, true_len);
  } else {
    ramdisk_read(buf, file_table[fd].disk_offset + file_table[fd].open_offset, true_len);
    ret = true_len;
  }

  if (fd != FD_EVENTS) {
    file_table[fd].open_offset += ret;
  }

  return ret;
}

size_t fs_write(int fd, const void *buf, size_t len) {
  assert(fd >= 0 && fd < NR_FILES);

  size_t true_len = len;
  size_t ret;

  if (file_table[fd].write != NULL) {
    ret = file_table[fd].write(buf, file_table[fd].open_offset, true_len);
  } else {
    if (file_table[fd].open_offset >= file_table[fd].size) {
      true_len = 0;
    } else if (file_table[fd].open_offset + len > file_table[fd].size) {
      true_len = file_table[fd].size - file_table[fd].open_offset;
    }

    ramdisk_write(buf, file_table[fd].disk_offset + file_table[fd].open_offset, true_len);
    ret = true_len;
  }

  file_table[fd].open_offset += ret;
  return ret;
}

off_t fs_lseek(int fd, off_t offset, int whence) {
  assert(fd >= 0 && fd < NR_FILES);

  off_t new_offset = 0;

  switch (whence) {
    case SEEK_SET:
      new_offset = offset;
      break;
    case SEEK_CUR:
      new_offset = file_table[fd].open_offset + offset;
      break;
    case SEEK_END:
      new_offset = file_table[fd].size + offset;
      break;
    default:
      panic("fs_lseek: invalid whence = %d", whence);
  }

  if (new_offset < 0) {
    new_offset = 0;
  }

  if ((size_t)new_offset > file_table[fd].size) {
    new_offset = file_table[fd].size;
  }

  file_table[fd].open_offset = new_offset;
  return new_offset;
}

int fs_close(int fd) {
  assert(fd >= 0 && fd < NR_FILES);
  return 0;
}
