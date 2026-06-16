#include <am.h>
#include <x86.h>

static _RegSet* (*H)(_Event, _RegSet*) = NULL;

void vecsys();
void vecnull();
void vectrap();
void vecirq();

_RegSet* irq_handle(_RegSet *tf) {
  _RegSet *next = tf;
  if (H) {
    _Event ev;
    switch (tf->irq) {
      case 0x80: ev.event = _EVENT_SYSCALL; break;
      case 0x81: ev.event = _EVENT_TRAP; break;
      case 32: ev.event = _EVENT_IRQ_TIME; break;
      default: ev.event = _EVENT_ERROR; break;
    }

    next = H(ev, tf);
    if (next == NULL) {
      next = tf;
    }
  }

  return next;
}

static GateDesc idt[NR_IRQ];

void _asye_init(_RegSet*(*h)(_Event, _RegSet*)) {
  // initialize IDT
  for (unsigned int i = 0; i < NR_IRQ; i ++) {
    idt[i] = GATE(STS_TG32, KSEL(SEG_KCODE), vecnull, DPL_KERN);
  }

  // -------------------- system call --------------------------
  idt[0x80] = GATE(STS_TG32, KSEL(SEG_KCODE), vecsys, DPL_USER);
  idt[0x81] = GATE(STS_TG32, KSEL(SEG_KCODE), vectrap, DPL_KERN);
  idt[32] = GATE(STS_TG32, KSEL(SEG_KCODE), vecirq, DPL_KERN);

  set_idt(idt, sizeof(idt));

  // register event handler
  H = h;
}

_RegSet *_make(_Area stack, void *entry, void *arg) {
  uintptr_t *sp = (uintptr_t *)stack.end;
  *--sp = (uintptr_t)arg;
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

void _trap() {
  asm volatile("int $0x81");
}

int _istatus(int enable) {
  uint32_t eflags;
  asm volatile("pushfl; popl %0" : "=r"(eflags));
  if (enable) {
    asm volatile("sti");
  } else {
    asm volatile("cli");
  }
  return (eflags & FL_IF) != 0;
}
