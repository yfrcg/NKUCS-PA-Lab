#include "common.h"

#define DEFAULT_ENTRY ((void *)0x4000000)

int fs_open(const char *pathname, int flags, int mode);
size_t fs_read(int fd, void *buf, size_t len);
size_t fs_filesz(int fd);
int fs_close(int fd);

uintptr_t loader(_Protect *as, const char *filename) {
  (void)as;

  int fd = fs_open(filename, 0, 0);
  size_t size = fs_filesz(fd);

  fs_read(fd, DEFAULT_ENTRY, size);
  fs_close(fd);

  return (uintptr_t)DEFAULT_ENTRY;
}