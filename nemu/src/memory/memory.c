#include "nemu.h"
#include "device/mmio.h"
#include "memory/mmu.h"

#define PMEM_SIZE (128 * 1024 * 1024)

#define pmem_rw(addr, type) *(type *)({\
    Assert(addr < PMEM_SIZE, "physical address(0x%08x) is out of bound", addr); \
    guest_to_host(addr); \
    })

uint8_t pmem[PMEM_SIZE];

/* Memory accessing interfaces */

uint32_t paddr_read(paddr_t addr, int len) {
  int map_NO = is_mmio(addr);
  if (map_NO != -1) {
    return mmio_read(addr, len, map_NO);
  }
  return pmem_rw(addr, uint32_t) & (~0u >> ((4 - len) << 3));
}

void paddr_write(paddr_t addr, int len, uint32_t data) {
  int map_NO = is_mmio(addr);
  if (map_NO != -1) {
    mmio_write(addr, len, data, map_NO);
    return;
  }
  memcpy(guest_to_host(addr), &data, len);
}

static paddr_t page_translate(vaddr_t addr, bool is_write) {
  paddr_t pdir_base = cpu.cr3 & ~PAGE_MASK;
  uint32_t pdir_idx = (addr >> 22) & 0x3ff;
  uint32_t ptab_idx = (addr >> 12) & 0x3ff;
  uint32_t offset = addr & PAGE_MASK;

  paddr_t pde_addr = pdir_base + pdir_idx * 4;
  uint32_t pde = paddr_read(pde_addr, 4);
  assert(pde & 0x1);
  pde |= 0x20;
  paddr_write(pde_addr, 4, pde);

  paddr_t pte_addr = (pde & ~PAGE_MASK) + ptab_idx * 4;
  uint32_t pte = paddr_read(pte_addr, 4);
  assert(pte & 0x1);
  pte |= 0x20;
  if (is_write) {
    pte |= 0x40;
  }
  paddr_write(pte_addr, 4, pte);

  return (pte & ~PAGE_MASK) + offset;
}

static inline bool paging_enabled(void) {
  return (cpu.cr0 & 0x80000000) != 0;
}

uint32_t vaddr_read(vaddr_t addr, int len) {
  if (!paging_enabled()) {
    return paddr_read(addr, len);
  }

  uint32_t data = 0;
  for (int i = 0; i < len; i++) {
    data |= paddr_read(page_translate(addr + i, false), 1) << (i * 8);
  }
  return data;
}

void vaddr_write(vaddr_t addr, int len, uint32_t data) {
  if (!paging_enabled()) {
    paddr_write(addr, len, data);
    return;
  }

  for (int i = 0; i < len; i++) {
    paddr_write(page_translate(addr + i, true), 1, data >> (i * 8));
  }
}
