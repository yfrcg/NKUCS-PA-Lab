#include "common.h"

#include "memory.h"

#define DEFAULT_ENTRY ((void *)0x8048000)

int fs_open(const char *pathname, int flags, int mode);
size_t fs_read(int fd, void *buf, size_t len);
size_t fs_filesz(int fd);
int fs_close(int fd);

uintptr_t loader(_Protect *as, const char *filename) {
  int fd = fs_open(filename, 0, 0);
  size_t size = fs_filesz(fd);

  if (as == NULL) {
    fs_read(fd, DEFAULT_ENTRY, size);
  } else {
    uintptr_t va = (uintptr_t)DEFAULT_ENTRY;
    size_t left = size;

    while (left > 0) {
      void *pa = new_page();
      size_t len = left < PGSIZE ? left : PGSIZE;

      memset(pa, 0, PGSIZE);
      _map(as, (void *)va, pa);
      fs_read(fd, pa, len);

      va += PGSIZE;
      left -= len;
    }
  }

  fs_close(fd);

  return (uintptr_t)DEFAULT_ENTRY;
}
