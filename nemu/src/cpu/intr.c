#include "cpu/exec.h"
#include "memory/mmu.h"

void raise_intr(uint8_t NO, vaddr_t ret_addr) {
  Log("raise_intr: NO = 0x%02x, ret_addr = 0x%08x", NO, ret_addr);
  Log("raise_intr: idtr.base = 0x%08x, idtr.limit = 0x%04x",
      cpu.idtr.base, cpu.idtr.limit);

  vaddr_t gate_addr = cpu.idtr.base + NO * 8;
  Log("raise_intr: gate_addr = 0x%08x", gate_addr);

  uint32_t gate_low = vaddr_read(gate_addr, 4);
  uint32_t gate_high = vaddr_read(gate_addr + 4, 4);

  Log("raise_intr: gate_low = 0x%08x, gate_high = 0x%08x",
      gate_low, gate_high);

  vaddr_t target = (gate_low & 0xffff) | (gate_high & 0xffff0000);
  Log("raise_intr: target = 0x%08x, esp = 0x%08x", target, cpu.esp);

  cpu.esp -= 4;
  vaddr_write(cpu.esp, 4, cpu.eflags);

  cpu.esp -= 4;
  vaddr_write(cpu.esp, 4, cpu.cs);

  cpu.esp -= 4;
  vaddr_write(cpu.esp, 4, ret_addr);

  cpu.eip = target;
}

void dev_raise_intr() {
}