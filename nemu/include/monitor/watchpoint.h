#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include "common.h"

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;

  /* TODO: Add more members if necessary */
  int value;
  char expr[32];
  int count;

} WP;

WP* new_wp(char * args);
void free_wp(WP* wp);
void print_wp();
bool check_wp();
WP* find_point(int num);

#endif
