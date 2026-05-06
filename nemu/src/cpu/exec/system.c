#include "cpu/exec.h"

void diff_test_skip_qemu();
void diff_test_skip_nemu();

make_EHelper(lidt) {
  TODO();

  print_asm_template1(lidt);
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
  TODO();

  print_asm("int %s", id_dest->str);

#ifdef DIFF_TEST
  diff_test_skip_nemu();
#endif
}

make_EHelper(iret) {
  TODO();

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
