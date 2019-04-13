#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>

enum {
	TK_NOTYPE = 256, TK_EQ,
	TK_NEQ, TK_AND, TK_OR, TK_GE, TK_LE, TK_NEG, 
	TK_DEC, TK_HEX, TK_POINTER, TK_REG,  
};

static struct rule {
	char *regex;
	int token_type;
	int priority;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */
	{" +", TK_NOTYPE, 0},			// spaces
	{"\\t+", TK_NOTYPE, 0},			// spaces
	{"\\+", '+', 4},				// plus
	{"==", TK_EQ, 3},				// equal
	{"!=", TK_NEQ, 3},				// not epual
    {"&&", TK_AND, 2},				// and
	{"\\|\\|", TK_OR, 1},			// or
	{"-", '-', 4},					// minus
	{">", '>', 3},					// greater
	{">=", TK_GE, 3},				// greater or equal
	{"<", '<', 3},					// less
	{"<=", TK_LE, 3},				// less or equal
	{"\\*", '*', 5},				// mul
	{"/", '/', 5},					// divide
	{"\\(", '(', 7},				// left bracket
	{"\\)", ')', 7},				// right bracket
	{"\\b[0-9]+\\b", TK_DEC, 0},    // decimal number
	{"\\b0[xX][0-9a-fA-F]+\\b", TK_HEX, 0},   // hex number
	{"\\$(eax|EAX|ebx|EBX|ecx|ECX|edx|EDX|ebp|EBP|esp|ESP|esi|ESI|edi|EDI|eip|EIP)", TK_REG, 0},  // register
};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]) )

static regex_t re[NR_REGEX];

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
	int i;
	char error_msg[128];
	int ret;

	for (i = 0; i < NR_REGEX; i ++) {
		ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
		if (ret != 0) {
			regerror(ret, &re[i], error_msg, 128);
			panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
		}
	}
}

typedef struct token {
	int type;
	char str[32];
	int priority;
} Token;

Token tokens[1000];
int nr_token;

static bool make_token(char *e) {
	int position = 0;
	int i;
	regmatch_t pmatch;

	nr_token = 0;

	while (e[position] != '\0') {
    /* Try all rules one by one. */
		for (i = 0; i < NR_REGEX; i ++) {
			if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
				char *substr_start = e + position;
				int substr_len = pmatch.rm_eo;

				Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
					i, rules[i].regex, position, substr_len, substr_len, substr_start);
				position += substr_len;

				/* TODO: Now a new token is recognized with rules[i]. Add codes
				 * to record the token in the array `tokens'. For certain types
                 * of tokens, some extra actions should be performed.
				 */

				switch (rules[i].token_type) {
					case TK_NOTYPE: 
						break;

					case TK_REG:
						tokens[nr_token].type = rules[i].token_type;
						strncpy(tokens[nr_token].str, substr_start+1, substr_len-1);
						tokens[nr_token].str[substr_len-1] = '\0';
						tokens[nr_token].priority = rules[i].priority;
						nr_token++;
						break;

					case TK_DEC: 
					case TK_HEX:			 
						tokens[nr_token].type = rules[i].token_type;
						strncpy(tokens[nr_token].str, substr_start, substr_len);
						tokens[nr_token].str[substr_len] = '\0';
						tokens[nr_token].priority = rules[i].priority;
						nr_token++;
						break;

					default: 
						tokens[nr_token].type = rules[i].token_type;
						tokens[nr_token].priority = rules[i].priority;
						nr_token++;
				}
				break;
			}
		}

		if (i == NR_REGEX) {
			printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
			return false;
		}
	}

	return true;
}

/* check the brackets is right */  
static bool check_parentheses(int l, int r) {

	if (tokens[l].type == '(' && tokens[r].type == ')') {
		int l_count = 0, r_count = 0;
		for (int i = l + 1; i < r; i++) {
			if (tokens[i].type == '(') l_count++;
			if (tokens[i].type == ')') r_count++;
			if (r_count > l_count) return false;
		}
		
		return l_count == r_count;
	}

	return false;
}

// load data in register
static uint32_t read_reg(char *addr) {
	return 0;
}

// find dominant operator in experssion
static int dominant(int l, int r) {
	
	int op = l, pri = 8, count = 0;
	for (int i = l; i <= r; i++) {
		if (tokens[i].type == '(') 
			count++;
		else if (tokens[i].type == ')') 
			count--;
		if (count > 0)
			continue;
		if (tokens[i].type == '+' || tokens[i].type == '-' || \
			tokens[i].type == '*' || tokens[i].type == '/') {
			if (tokens[i].priority <= pri) {
				op = i;
				pri = tokens[i].priority;
			}
		}
	}
	printf("location: %d %d %d\n", l, r, op);	
	return op;
}

static uint32_t eval(int l, int r, bool *success) {
	if (!*success) {
		return -1;
	}

	if (l > r) {
		Assert(l > r, "Wrong expression\n");
		*success = false;
		printf("Wrong 1!\n");
		return -1;
	}

	if (l == r) {
		int num = 0;
		switch (tokens[l].type) {
			case TK_DEC: 
				sscanf(tokens[l].str, "%d", &num);
				return num;
			case TK_HEX:
				sscanf(tokens[l].str, "%x", &num);
				return num;
			case TK_REG:
				return read_reg(tokens[l].str);
		}
	}

	else if (check_parentheses(l, r) == true) {
		return eval(l + 1, r - 1, success);
	}

	else {
		int op = dominant(l, r);
		//if (token[i].type == TK_NEG)
		//	return -eval(l + 1, r, *success);
		uint32_t val1 = eval(l, op - 1, success);
		uint32_t val2 = eval(op + 1, r, success);
		
		switch(tokens[op].type) {
			case '+' : return val1 + val2;
			case '-' : return val1 - val2;
			case '*' : return val1 * val2;
			case '/' :
				if (val2 == 0) {
					*success = false;
					printf("Wrong!\n");
					return -1;
				}
				return val1 / val2;
			case TK_EQ: return val1 == val2;
			case TK_AND: return val1 && val2;
			case TK_NEQ: return val1 != val2;
			case TK_OR: return val1 || val2;
			case '<': return val1 < val2;
			case '>': return val1 > val2;
			case TK_GE: return val1 >= val2;
			case TK_LE: return val1 <= val2;
		
			default: return -1;
		}
	}

	return 0;
}

uint32_t expr(char *e, bool *success) {
	if (!make_token(e)) {
		*success = false;
		return 0;
	}

	/* check poniter and negative number */
	for (int i = 0; i < nr_token; i++) {

		if (tokens[i].type == '-' && (i == 0 || tokens[i-1].type == '+' || \
			tokens[i-1].type == '(' || tokens[i-1].type == '-')) {
			tokens[i].type = TK_NEG;
			tokens[i].priority = 6;
		}
	}

	*success = true;
	return eval(0 , nr_token - 1, success);
}
