#include "proc.h"

#define MAX_NR_PROC 4

static PCB pcb[MAX_NR_PROC];
static int nr_proc = 0;
static int current_game = 0;
PCB *current = NULL;

uintptr_t loader(_Protect *as, const char *filename);

void load_prog(const char *filename) {
  int i = nr_proc ++;
  assert(i < MAX_NR_PROC);
  _protect(&pcb[i].as);

  uintptr_t entry = loader(&pcb[i].as, filename);

  _Area stack;
  stack.start = pcb[i].stack;
  stack.end = stack.start + sizeof(pcb[i].stack);

  pcb[i].tf = _umake(&pcb[i].as, stack, stack, (void *)entry, NULL, NULL);
  pcb[i].cur_brk = 0;
  pcb[i].max_brk = 0;
}

_RegSet* schedule(_RegSet *prev) {
  if (current != NULL) {
    current->tf = prev;
  }

  if (current == NULL || current == &pcb[1]) {
    current = &pcb[current_game];
  } else {
    current = &pcb[1];
  }

  _switch(&current->as);
  return current->tf;
}

void switch_game(void) {
  current_game = (current_game == 0) ? 2 : 0;
}
