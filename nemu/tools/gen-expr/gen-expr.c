#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>

// this should be enough
static char buf[65536];
static int state = 0;

static inline int choose(int n) {
	return rand() % n;
}


static inline void gen_num() {
	uint32_t num = (uint32_t)rand() % 100 + 1;
	switch (choose(2)) {
		case 0:
			sprintf(buf + strlen(buf), "%u", num);
			break;
		case 1:
			sprintf(buf + strlen(buf), "%#x", num);
			break;
		default:
			break;
	}
}


static inline void gen(char c) {
	size_t i = strlen(buf);
	if (i > 65536) {
		state = 1;
		return;
	}
	buf[i] = ' ';
	buf[i+1] = c;
	buf[i+2] = '\0';
}


static inline void gen_rand_op() {
	switch (choose(4)) {
		case 0:
			gen('+');
			break;
		case 1:
			gen('-');
			break;
		case 2:
			gen('*');
			break;
		case 3:
			gen('/');
			break;
	}
}


static inline void gen_rand_expr() {
	switch (choose(3)) {
		case 0:
			gen_num();
			break;
		case 1:
			gen('(');
			gen_rand_expr();
			gen(')');
			break;
		case 2:
			gen_rand_expr();
			gen_rand_op();
			gen_rand_expr();
			break;
	}	
} 


static char code_buf[65536];
static char *code_format =
"#include <stdio.h>\n"
"int main() { "
"  unsigned result = %s; "
"  printf(\"%%u\", result); "
"  return 0; "
"}";

int main(int argc, char *argv[]) {
	int seed = time(0);
	srand(seed);
	int loop = 1;
	if (argc > 1) {
		sscanf(argv[1], "%d", &loop);
	}
	int i;
	for (i = 0; i < loop; i ++) {

		do {
			memset(buf, 0, 65536);
			state = 0;
			gen_rand_expr();
		} 
		while (state);

		sprintf(code_buf, code_format, buf);

		FILE *fp = fopen(".code.c", "w");
		assert(fp != NULL);
		fputs(code_buf, fp);
		fclose(fp);

		int ret = system("gcc .code.c -o .expr");
		if (ret != 0) continue;

		fp = popen("./.expr", "r");
		assert(fp != NULL);

		int result;
		fscanf(fp, "%d", &result);
		pclose(fp);

		printf("%u %s\n", result, buf);
	}
	return 0;
}
