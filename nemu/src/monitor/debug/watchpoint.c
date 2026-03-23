#include "monitor/watchpoint.h"
#include "monitor/expr.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#define NR_WP 32

static WP wp_pool[NR_WP];
static WP *head, *free_;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = &wp_pool[i + 1];
  }
  wp_pool[NR_WP - 1].next = NULL;

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */
WP* new_wp() {
  assert(free_ != NULL);

  WP *wp = free_;
  free_ = free_->next;

  wp->next = NULL;

  if (head == NULL) {
    head = wp;
  } else {
    WP *p = head;
    while (p->next != NULL) {
      p = p->next;
    }
    p->next = wp;
  }

  return wp;
}
void free_wp(WP *wp) {
  assert(wp != NULL);

  if (head == wp) {
    head = wp->next;
  } else {
    WP *p = head;
    while (p != NULL && p->next != wp) {
      p = p->next;
    }
    assert(p != NULL);
    p->next = wp->next;
  }

  wp->next = free_;
  free_ = wp;
}
static WP* find_wp(int n) {
  WP *p = head;
  while (p != NULL) {
    if (p->NO == n) {
      return p;
    }
    p = p->next;
  }
  return NULL;
}
bool delete_wp(int n) {
  WP *wp = find_wp(n);
  if (wp == NULL) {
    return false;
  }
  free_wp(wp);
  return true;
}
void display_wp(void) {
  WP *p = head;

  if (p == NULL) {
    printf("No watchpoints.\n");
    return;
  }

  while (p != NULL) {
    printf("Num: %d, Expr: %s, Val: %u (0x%x)\n",
        p->NO, p->expr, p->old_val, p->old_val);
    p = p->next;
  }
}
WP* add_wp(char *e) {
  bool success = true;
  word_t val = expr(e, &success);
  if (!success) {
    return NULL;
  }

  WP *wp = new_wp();
  strcpy(wp->expr, e);
  wp->old_val = val;

  return wp;
}
bool check_wp(void) {
  WP *p = head;
  bool hit = false;

  while (p != NULL) {
    bool success = true;
    word_t new_val = expr(p->expr, &success);

    if (success && new_val != p->old_val) {
      printf("Watchpoint %d triggered: %s\n", p->NO, p->expr);
      printf("Old value = %u (0x%x)\n", p->old_val, p->old_val);
      printf("New value = %u (0x%x)\n", new_val, new_val);

      p->old_val = new_val;
      hit = true;
    }

    p = p->next;
  }

  return hit;
}

