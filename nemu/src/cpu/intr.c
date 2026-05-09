#include "cpu/exec.h"
#include "memory/mmu.h"

void raise_intr(uint8_t NO, vaddr_t ret_addr) {
  vaddr_t gate_addr = cpu.idtr.base + NO * 8;

  Assert(cpu.idtr.base != 0, "IDTR is not initialized");
  Assert(NO * 8 + 7 <= cpu.idtr.limit,
      "Interrupt number 0x%x is out of IDT limit 0x%x", NO, cpu.idtr.limit);

  uint32_t gate_low = vaddr_read(gate_addr, 4);
  uint32_t gate_high = vaddr_read(gate_addr + 4, 4);

  vaddr_t target = (gate_low & 0xffff) | (gate_high & 0xffff0000);

  Assert(target != 0, "IDT gate target is 0, NO = 0x%x", NO);

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