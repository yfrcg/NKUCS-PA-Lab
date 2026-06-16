#include <x86.h>

#define PG_ALIGN __attribute((aligned(PGSIZE)))

static PDE kpdirs[NR_PDE] PG_ALIGN;
static PTE kptabs[PMEM_SIZE / PGSIZE] PG_ALIGN;
static void* (*palloc_f)();
static void (*pfree_f)(void*);

_Area segments[] = {      // Kernel memory mappings
  {.start = (void*)0,          .end = (void*)PMEM_SIZE}
};

#define NR_KSEG_MAP (sizeof(segments) / sizeof(segments[0]))

void _pte_init(void* (*palloc)(), void (*pfree)(void*)) {
  palloc_f = palloc;
  pfree_f = pfree;

  int i;

  // make all PDEs invalid
  for (i = 0; i < NR_PDE; i ++) {
    kpdirs[i] = 0;
  }

  PTE *ptab = kptabs;
  for (i = 0; i < NR_KSEG_MAP; i ++) {
    uint32_t pdir_idx = (uintptr_t)segments[i].start / (PGSIZE * NR_PTE);
    uint32_t pdir_idx_end = (uintptr_t)segments[i].end / (PGSIZE * NR_PTE);
    for (; pdir_idx < pdir_idx_end; pdir_idx ++) {
      // fill PDE
      kpdirs[pdir_idx] = (uintptr_t)ptab | PTE_P;

      // fill PTE
      PTE pte = PGADDR(pdir_idx, 0, 0) | PTE_P;
      PTE pte_end = PGADDR(pdir_idx + 1, 0, 0) | PTE_P;
      for (; pte < pte_end; pte += PGSIZE) {
        *ptab = pte;
        ptab ++;
      }
    }
  }

  set_cr3(kpdirs);
  set_cr0(get_cr0() | CR0_PG);
}

void _protect(_Protect *p) {
  PDE *updir = (PDE*)(palloc_f());
  p->ptr = updir;
  // map kernel space
  for (int i = 0; i < NR_PDE; i ++) {
    updir[i] = kpdirs[i];
  }

  p->area.start = (void*)0x8000000;
  p->area.end = (void*)0xc0000000;
}

void _release(_Protect *p) {
}

void _switch(_Protect *p) {
  set_cr3(p->ptr);
}

void _map(_Protect *p, void *va, void *pa) {
  PDE *pdir = (PDE *)p->ptr;
  uintptr_t vaddr = (uintptr_t)va;
  uintptr_t paddr = (uintptr_t)pa;
  uint32_t pdir_idx = PDX(vaddr);
  uint32_t ptab_idx = PTX(vaddr);

  if ((pdir[pdir_idx] & PTE_P) == 0) {
    PTE *ptab = (PTE *)palloc_f();
    for (int i = 0; i < NR_PTE; i++) {
      ptab[i] = 0;
    }
    pdir[pdir_idx] = (uintptr_t)ptab | PTE_P | PTE_W | PTE_U;
  }

  PTE *ptab = (PTE *)PTE_ADDR(pdir[pdir_idx]);
  ptab[ptab_idx] = (paddr & ~(PGSIZE - 1)) | PTE_P | PTE_W | PTE_U;
}

void _unmap(_Protect *p, void *va) {
}

_RegSet *_umake(_Protect *p, _Area ustack, _Area kstack, void *entry, char *const argv[], char *const envp[]) {
  (void)p;
  (void)kstack;
  (void)argv;
  (void)envp;

  uintptr_t *sp = (uintptr_t *)ustack.end;
  *--sp = 0;
  *--sp = 0;
  *--sp = 0;
  *--sp = 0;

  _RegSet *tf = (_RegSet *)sp - 1;
  for (uintptr_t *p = (uintptr_t *)tf; p < (uintptr_t *)(tf + 1); p++) {
    *p = 0;
  }

  tf->eip = (uintptr_t)entry;
  tf->cs = KSEL(SEG_KCODE);
  tf->eflags = FL_IF | 0x2;
  tf->esp = (uintptr_t)sp;
  return tf;
}
