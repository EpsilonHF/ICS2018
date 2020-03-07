#include "monitor/watchpoint.h"
#include "monitor/expr.h"

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

WP* new_wp(char *args) {
	if (free_ == NULL) {
		printf("No Free WatchPointer!\n");
		return NULL;
	}
	if (args == NULL) {
		printf("Please Input Valid Expression!\n");
		return NULL;
	}

	WP *ptr = free_;
	free_ = free_->next;

	ptr->next = head;
	head = ptr;
	strcpy(head->exp,args);

	bool success;
	int val = expr(args, &success);
	if (!success) {
		printf("Invalid Expression.\n");
		return NULL;
	}
	
	head->val = val;

	return head;
}


int del_wp(int n) {
	if (n >= 0 && n < NR_WP) {
		int result = free_wp(&wp_pool[n]);
		return result;
	}
	else
		return 1;
}


int free_wp(WP *wp) {
	if (wp == NULL) {
		printf("Can't free NULL Pointer!\n");
		return 1;
	}

	if (wp == head) {
		head = wp->next;
		wp->next = free_;
		free_ = wp;
		return 0;
	}

	for (WP *ptr = head; ptr != NULL; ptr = ptr->next) {
		if (ptr->next == wp) {
			ptr->next = ptr->next->next;
			wp->next = free_;
			free_ = wp;
			return 0;
		}	
	}

	return 1;
}


WP* get_head() {
	return head;
}


WP* get_free() {
	return free_;
}

int check_watchpoints() {
	WP *ptr = head;
	bool success;
	int state = 0;
	uint32_t res;
	while (ptr) {
		res = expr(ptr->exp, &success);
		if (res != ptr->val) {
			printf("NO: %d EXPR: %s Old: %#x NEW: %#x\n", ptr->NO, \
				   ptr->exp, ptr->val, res); 	
			ptr->val = res;
			state = 1;
		}
		ptr = ptr->next;
	}
	return state;
}


int print_watchpoints() {
	WP *ptr = head; 
	if (ptr == NULL) {
		printf("No watchpoint!\n");
		return 1;
	}
	printf("NO\tEXPR\t\tValue\n");

	while(ptr != NULL) {
		printf("%-d\t%-16s%#08x  %d\n",ptr->NO, ptr->exp, ptr->val, ptr->val);
		ptr = ptr->next;
	}

	return 0;
}
