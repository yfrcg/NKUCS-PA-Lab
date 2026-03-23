#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>

enum {
  TK_NOTYPE = 256, 
  TK_EQ,
  TK_NEQ,
  TK_LE,
  TK_GE,
  TK_AND,
  TK_OR,
  TK_NUM,
  TK_REG,
  TK_NEG,
  TK_DEREF,
  /* TODO: Add more token types */

};

static struct rule {
  char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */
  {" +", TK_NOTYPE},    // spaces
  {"\\+", '+'},         // plus
  {"==", TK_EQ},         // equal
  {"^!=",TK_NEQ},
  {"^<=",TK_LE},
  {"^>=",TK_GE},
  {"^&&",TK_AND},
  {"^\\|\\|",TK_OR},
  {"^-",'-'},
  {"^\\*",'*'},
  {"^/",'/'},
  {"^\\(",'('},
  {"^\\)",')'},
 {"^0[xX][0-9a-fA-F]+",TK_NUM},
  {"^[0-9]+",TK_NUM},
  {"^\\$[a-zA-Z_][a-zA-Z0-9_]*",TK_REG},
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
} Token;

Token tokens[32];
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

	switch (rules[i].token_type) {
		  case TK_NOTYPE:
			break;

		  case TK_NUM:
		  case TK_REG:
			tokens[nr_token].type = rules[i].token_type;
			Assert(substr_len < sizeof(tokens[nr_token].str), "token too long");
			strncpy(tokens[nr_token].str, substr_start, substr_len);
			tokens[nr_token].str[substr_len] = '\0';
			nr_token++;
			break;

		  case TK_EQ:
		  case TK_NEQ:
		  case TK_AND:
		  case TK_OR:
		  case TK_LE:
		  case TK_GE:
		  case '+':
		  case '-':
		  case '*':
		  case '/':
		  case '(':
		  case ')':
			tokens[nr_token].type = rules[i].token_type;
			tokens[nr_token].str[0] = '\0';
			nr_token++;
			break;

		  default:
			panic("unknown token type: %d", rules[i].token_type);
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
static bool check_parentheses(int p, int q) {
  if (tokens[p].type != '(' || tokens[q].type != ')') {
    return false;
  }

  int balance = 0;
  for (int i = p; i <= q; i++) {
    if (tokens[i].type == '(') balance++;
    else if (tokens[i].type == ')') balance--;

    if (balance < 0) return false;
    if (balance == 0 && i < q) return false;
  }

  return balance == 0;
}
static bool check_balanced(int p, int q) {
  int balance = 0;
  for (int i = p; i <= q; i++) {
    if (tokens[i].type == '(') balance++;
    else if (tokens[i].type == ')') balance--;

    if (balance < 0) return false;
  }
  return balance == 0;
}
static bool is_operator(int type) {
  switch (type) {
    case '+':
    case '-':
    case '*':
    case '/':
    case TK_EQ:
    case TK_NEQ:
    case TK_AND:
    case TK_OR:
    case TK_NEG:
    case TK_DEREF:
	case TK_LE:
	case TK_GE:
      return true;
    default:
      return false;
  }
}
static int precedence(int type) {
  switch (type) {
    case TK_OR:               return 1;
    case TK_AND:              return 2;
    case TK_EQ:
    case TK_NEQ:
	case TK_LE:
	case TK_GE:              return 3;
    case '+':
    case '-':                 return 4;
    case '*':
    case '/':                 return 5;
    case TK_NEG:
    case TK_DEREF:            return 6;
    default:                  return 0;
  }
}
static int find_main_op(int p, int q) {
  int pos = -1;
  int min_pri = 100;
  int balance = 0;

  for (int i = p; i <= q; i++) {
    int type = tokens[i].type;

    if (type == '(') {
      balance++;
      continue;
    }
    if (type == ')') {
      balance--;
      continue;
    }

    if (balance > 0) continue;
    if (!is_operator(type)) continue;

    int pri = precedence(type);
	if (pri > 0) {
	  if (pri < min_pri) {
		min_pri = pri;
		pos = i;
	  }
	  else if (pri == min_pri) {
		if (type == TK_NEG || type == TK_DEREF) {

				} else {
		  pos = i;
		}
	  }
	}

    }

  return pos;
}
static uint32_t eval(int p, int q, bool *success) {
  if (p > q) {
    *success = false;
    return 0;
  }

  if (!check_balanced(p, q)) {
    *success = false;
    return 0;
  }

  if (p == q) {
    if (tokens[p].type == TK_NUM) {
      return strtoul(tokens[p].str, NULL, 0);
    }

    if (tokens[p].type == TK_REG) {
      bool reg_success = true;
      uint32_t val = isa_reg_str2val(tokens[p].str + 1, &reg_success);
      if (!reg_success) {
        *success = false;
        return 0;
      }
      return val;
    }

    *success = false;
    return 0;
  }

  if (check_parentheses(p, q)) {
    return eval(p + 1, q - 1, success);
  }

  int op = find_main_op(p, q);
  if (op < 0) {
    *success = false;
    return 0;
  }

  if (tokens[op].type == TK_NEG) {
    uint32_t val = eval(op + 1, q, success);
    if (!*success) return 0;
    return -val;
  }

  if (tokens[op].type == TK_DEREF) {
    uint32_t addr = eval(op + 1, q, success);
    if (!*success) return 0;
    return vaddr_read(addr, 4);
  }

  uint32_t val1 = eval(p, op - 1, success);
  if (!*success) return 0;

  uint32_t val2 = eval(op + 1, q, success);
  if (!*success) return 0;

  switch (tokens[op].type) {
    case '+': return val1 + val2;
    case '-': return val1 - val2;
    case '*': return val1 * val2;
    case '/':
      if (val2 == 0) {
        *success = false;
        return 0;
      }
      return val1 / val2;

    case TK_EQ:  return val1 == val2;
    case TK_NEQ: return val1 != val2;
    case TK_AND: return val1 && val2;
    case TK_OR:  return val1 || val2;
	case TK_LE:  return val1 <= val2;
	case TK_GE:  return val1 >= val2;

    default:
      *success = false;
      return 0;
  }
}

uint32_t expr(char *e, bool *success) {
	*success = true;
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  for (int i = 0; i < nr_token; i++) {
    if (tokens[i].type == '*') {
      if (i == 0 ||
          !(tokens[i - 1].type == TK_NUM ||
            tokens[i - 1].type == TK_REG ||
            tokens[i - 1].type == ')')) {
        tokens[i].type = TK_DEREF;
      }
    }
    else if (tokens[i].type == '-') {
      if (i == 0 ||
          !(tokens[i - 1].type == TK_NUM ||
            tokens[i - 1].type == TK_REG ||
            tokens[i - 1].type == ')')) {
        tokens[i].type = TK_NEG;
      }
    }
  }

  return eval(0, nr_token - 1, success);
}
