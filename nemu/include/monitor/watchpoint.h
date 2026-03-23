#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include "common.h"

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;

  /* TODO: Add more members if necessary */
  char expr[128];
  word_t old_val;


} WP;

void init_wp_pool(void);

WP* new_wp(void);
void free_wp(WP *wp);

void display_wp(void);
WP* add_wp(char *e);
bool delete_wp(int n);
bool check_wp(void);

#endif
