#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include "common.h"

typedef struct watchpoint {
	int NO;
	struct watchpoint *next;
	char exp[256];
	uint32_t val;
} WP;

WP* new_wp(char *);

int del_wp(int);

int free_wp(WP*);

WP* get_head();

WP* get_free();

int check_watchpoints();

int print_watchpoints();

#endif
