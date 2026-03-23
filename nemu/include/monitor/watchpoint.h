#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include "common.h"

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;


  char expr[128];
  uint32_t old_value;
  /* TODO: Add more members if necessary */

} WP;
WP* get_head(void);
void print_wp(void);
bool check_wp(void);
WP* new_wp(void);
void free_wp(WP *wp);
void init_wp_pool(void);
#endif
