#include "common.h"

_RegSet* do_syscall(_RegSet *r);

static _RegSet* do_event(_Event e, _RegSet* r) {
  switch (e.event) {
    case 3:   // 当前日志里 _EVENT_SYSCALL 的实际编号就是 3
      return do_syscall(r);

    default:
      panic("Unhandled event ID = %d", e.event);
  }

  return r;
}

void init_irq(void) {
  _asye_init(do_event);
}