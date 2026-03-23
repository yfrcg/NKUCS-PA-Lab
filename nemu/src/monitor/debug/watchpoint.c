#include "monitor/watchpoint.h"
#include "monitor/expr.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

#define NR_WP 32

static WP wp_pool[NR_WP];
static WP *head, *free_;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = &wp_pool[i + 1];
	wp_pool[i].expr[0] = '\0';
    wp_pool[i].old_value = 0;
  }
  wp_pool[NR_WP - 1].next = NULL;

  head = NULL;
  free_ = wp_pool;
}

WP* new_wp(void) {
  if (free_ == NULL) {
    printf("No free watchpoint.\n");
    assert(0);
  }

  WP *wp = free_;
  free_ = free_->next;

  wp->next = head;
  head = wp;

  wp->expr[0] = '\0';
  wp->old_value = 0;

  return wp;
}

void free_wp(WP *wp) {
  if (wp == NULL) {
    return;
  }

  WP *prev = NULL;
  WP *cur = head;

  while (cur != NULL) {
    if (cur == wp) {
      if (prev == NULL) {
        head = cur->next;
      } else {
        prev->next = cur->next;
      }

      cur->next = free_;
      cur->expr[0] = '\0';
      cur->old_value = 0;
      free_ = cur;
      return;
    }

    prev = cur;
    cur = cur->next;
  }

  printf("Watchpoint not found.\n");
}

void print_wp(void) {
  WP *p = head;

  if (p == NULL) {
    printf("No watchpoints.\n");
    return;
  }

  printf("Num\tWhat\t\tValue\n");
  while (p != NULL) {
    printf("%d\t%s\t\t%u (0x%08x)\n",
           p->NO, p->expr, p->old_value, p->old_value);
    p = p->next;
  }
}

bool check_wp(void) {
  WP *p = head;
  bool changed = false;

  while (p != NULL) {
    bool success = true;
    uint32_t new_value = expr(p->expr, &success);

    if (!success) {
      printf("Watchpoint %d: bad expression: %s\n", p->NO, p->expr);
      p = p->next;
      continue;
    }

    if (new_value != p->old_value) {
      printf("Watchpoint %d triggered: %s\n", p->NO, p->expr);
      printf("Old value = %u (0x%08x)\n", p->old_value, p->old_value);
      printf("New value = %u (0x%08x)\n", new_value, new_value);

      p->old_value = new_value;
      changed = true;
    }

    p = p->next;
  }

  return changed;
}

WP* get_head(void) {
  return head;
}

/* TODO: Implement the functionality of watchpoint */


