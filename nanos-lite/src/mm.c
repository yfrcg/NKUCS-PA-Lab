#include "proc.h"
#include "memory.h"

static void *pf = NULL;
static uintptr_t fallback_brk = 0;
static uintptr_t fallback_max_brk = 0;

void* new_page(void) {
  assert((uintptr_t)pf + PGSIZE <= (uintptr_t)_heap.end);
  void *p = pf;
  pf += PGSIZE;
  return p;
}

void free_page(void *p) {
  panic("not implement yet");
}

/* The brk() system call handler. */
int mm_brk(uint32_t new_brk) {
  uintptr_t brk = new_brk;

  if (brk == 0) {
    return 0;
  }

  if (brk > (uintptr_t)_heap.end) {
    return -1;
  }

  if (current == NULL) {
    if (fallback_brk == 0) {
      fallback_brk = brk;
      fallback_max_brk = PGROUNDUP(brk);
    } else if (brk > fallback_max_brk) {
      fallback_max_brk = PGROUNDUP(brk);
    }

    fallback_brk = brk;
    return 0;
  }

  if (current->cur_brk == 0) {
    current->cur_brk = brk;
    current->max_brk = PGROUNDUP(brk);
    return 0;
  }

  if (brk > current->max_brk) {
    uintptr_t start = PGROUNDUP(current->max_brk);
    uintptr_t end = PGROUNDUP(brk);

    for (uintptr_t va = start; va < end; va += PGSIZE) {
      _map(&current->as, (void *)va, new_page());
    }

    current->max_brk = end;
  }

  current->cur_brk = brk;
  return 0;
}

void init_mm() {
  pf = (void *)PGROUNDUP((uintptr_t)_heap.start);
  Log("free physical pages starting from %p", pf);

  _pte_init(new_page, free_page);
}
