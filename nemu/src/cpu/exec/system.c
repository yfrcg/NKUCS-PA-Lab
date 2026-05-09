#include "cpu/exec.h"

void raise_intr(uint8_t NO, vaddr_t ret_addr);
void diff_test_skip_qemu();
void diff_test_skip_nemu();

make_EHelper(lidt) {
  assert(id_dest->type == OP_TYPE_MEM);

  cpu.idtr.limit = vaddr_read(id_dest->addr, 2);
  cpu.idtr.base = vaddr_read(id_dest->addr + 2, 4);

  print_asm("lidt %s", id_dest->str);
}

make_EHelper(mov_r2cr) {
  TODO();

  print_asm("movl %%%s,%%cr%d", reg_name(id_src->reg, 4), id_dest->reg);
}

make_EHelper(mov_cr2r) {
  TODO();

  print_asm("movl %%cr%d,%%%s", id_src->reg, reg_name(id_dest->reg, 4));

#ifdef DIFF_TEST
  diff_test_skip_qemu();
#endif
}

make_EHelper(int) {
  vaddr_t old_eip = cpu.eip;

  raise_intr(id_dest->val, *eip);

  decoding.is_jmp = 1;
  decoding.jmp_eip = cpu.eip;

  /* exec_wrapper() 在 update_eip() 之前还会用 cpu.eip 计算当前指令长度，
   * 所以这里必须先恢复 cpu.eip，真正跳转交给 update_eip() 完成。
   */
  cpu.eip = old_eip;

  print_asm("int %s", id_dest->str);

#ifdef DIFF_TEST
  diff_test_skip_nemu();
#endif
}

make_EHelper(iret) {
  rtlreg_t ret_eip, ret_cs, ret_eflags;

  rtl_pop(&ret_eip);
  rtl_pop(&ret_cs);
  rtl_pop(&ret_eflags);

  cpu.cs = ret_cs;
  cpu.eflags = ret_eflags;

  decoding.is_jmp = 1;
  decoding.jmp_eip = ret_eip;

  print_asm("iret");
}

uint32_t pio_read(ioaddr_t, int);
void pio_write(ioaddr_t, int, uint32_t);

make_EHelper(in) {
  if (decoding.opcode == 0xec || decoding.opcode == 0xed) {
    id_src->val = cpu.edx & 0xffff; 
    id_dest->type = OP_TYPE_REG;
    id_dest->reg = R_EAX; 
    id_dest->width = (decoding.opcode == 0xec) ? 1 : (decoding.is_operand_size_16 ? 2 : 4);
  }

  t0 = pio_read(id_src->val, id_dest->width);
  operand_write(id_dest, &t0);

  print_asm_template2(in);

#ifdef DIFF_TEST
  diff_test_skip_qemu();
#endif
}

make_EHelper(out) {

  if (decoding.opcode == 0xee || decoding.opcode == 0xef) {
    id_src->val = cpu.edx & 0xffff;
    id_dest->width = (decoding.opcode == 0xee) ? 1 : (decoding.is_operand_size_16 ? 2 : 4);
    
   
    if (id_dest->width == 1) {
      id_dest->val = reg_b(R_AL);
    } else if (id_dest->width == 2) {
      id_dest->val = reg_w(R_AX);
    } else {
      id_dest->val = reg_l(R_EAX);
    }
  }

  pio_write(id_src->val, id_dest->width, id_dest->val);
  print_asm_template2(out);

#ifdef DIFF_TEST
  diff_test_skip_qemu();
#endif
}
