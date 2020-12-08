#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>
#include <stdlib.h>
#include <string.h>
#include "cpu/helper.h"
enum {
      NOTYPE = 256, NOT, NEGATIVE,DEREF, REG_EAX,REG_EDX,REG_ECX,REG_EBX,REG_EBP,REG_ESI,REG_EDI,REG_ESP,REG_EIP,NUM,SPACES,ADDR,MINUS,PLUS,DIVIDE,MULTIPLY,EQ,NOT_EQ,OR,AND,

	/* TODO: Add more token types */

};

static struct rule {
	char *regex;
	int token_type;
} rules[] = {

	/* TODO: Add more rules.
	 * Pay attention to the precedence level of different rules.
	 */

	{" ",	NOTYPE},				// spaces
	{"\\+", PLUS},					// plus
	{"==", EQ},						// equal
  {"\\-", MINUS},         //minus
  {"\\*", MULTIPLY},         //multiply
  {"\\/", DIVIDE},         //divide
  {"!=", NOT_EQ},
  {"\\&&",AND},
  {"\\|\\|",OR},
  {"!",NOT},
  {"\\(", '('},         //
  {"\\)", ')'},         //
  {"\\$eax", REG_EAX},    //eax
  {"\\$edx", REG_EDX},    //edx
  {"\\$ecx", REG_ECX},    //ecx
  {"\\$ebx", REG_EBX},    //ebx
  {"\\$ebp", REG_EBP},    //ebp
  {"\\$esi", REG_ESI},    //esi
  {"\\$edi", REG_EDI},    //edi
  {"\\$esp", REG_ESP},    //esp
  {"\\$eip", REG_EIP},    //eip
  {"0x", ADDR},          //ADDR
  {"[0-9]*", NUM},            //number
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

	for(i = 0; i < NR_REGEX; i ++) {
		ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
		if(ret != 0) {
			regerror(ret, &re[i], error_msg, 128);
			Assert(ret == 0, "regex compilation failed: %s\n%s", error_msg, rules[i].regex);
		}
	}
}

typedef struct token {
	int type;
	char str[32];
} Token;

Token tokens[32];
int nr_token;

bool is_num(uint32_t index){
  switch(tokens[index].type){
  case NUM:
    return true;
  default:
    return false;
  }
}
bool is_binary(uint32_t index){
  switch(tokens[index].type){
  case PLUS:
  case MINUS:
  case MULTIPLY:
  case DIVIDE:
  case OR:
  case NOT_EQ:
  case AND:
  case NEGATIVE:
    return true;
  default:
    return false;

  }
}
static bool make_token(char *e) {
	int position = 0;
	int i;
	regmatch_t pmatch;

	nr_token = 0;

	while(e[position] != '\0') {
		/* Try all rules one by one. */
		for(i = 0; i < NR_REGEX; i ++) {
			if(regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
				char *substr_start = e + position;
				int substr_len = pmatch.rm_eo;

        //				Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s", i, rules[i].regex, position, substr_len, substr_len, substr_start);
				position += substr_len;

				/* TODO: Now a new token is recognized with rules[i]. Add codes
				 * to record the token in the array ``tokens''. For certain
				 * types of tokens, some extra actions should be performed.
				 */

				switch(rules[i].token_type) {
          case NOTYPE:
            break;
        default:

            tokens[nr_token].type = rules[i].token_type;
            strncpy(tokens[nr_token].str,substr_start,substr_len<32?substr_len:32);
            //  Log("get i = %d  %s",i,tokens[nr_token].str);
            nr_token++;
            break;
				}

				break;
			}
		}

		if(i == NR_REGEX) {
			printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
			return false;
		}
	}

  for(i = 0; i < nr_token; i ++) {
    if(tokens[i].type == MULTIPLY && (i == 0 ||  is_binary(i - 1)) ) {
      tokens[i].type = DEREF;
    }
    if(tokens[i].type == MINUS && (i == 0 || is_binary(i - 1)) ) {
      tokens[i].type = NEGATIVE;
    }
  }
  /*for(i = 0; i < nr_token; i++){
    Log("tokens[%d]:%s type=%d",i,tokens[i].str,tokens[i].type);
    }*/

	return true; 
}


bool is_eva(uint32_t index){
  switch(tokens[index].type){
  case PLUS:
  case MINUS:
  case MULTIPLY:
  case DIVIDE:
    return true;
  default:
    return false;
  }
}
bool is_addorsub(uint32_t index){
  return (tokens[index].type == '+') || (tokens[index].type == '-');
}

bool is_mulordiv(uint32_t index){
  return (tokens[index].type == '*') || (tokens[index].type == '/');
}
bool is_left(uint32_t index){
  if(tokens[index].type == '(')
    return true;
  return false;
}
bool is_right(uint32_t index){
  if(tokens[index].type == ')')
    return true;
  return false;
}
void clear_tokens(){
  uint32_t i;
  for(i = 0; i < nr_token; i++){
    memset(tokens[i].str,'\0',strlen(tokens[i].str));
  }
}
bool check_parentheses(uint32_t p,uint32_t q){
  int count = 0;
  if(!is_left(p) || !is_right(q))
    return false;
  while(p <= q){
    switch(tokens[p].type){
    case '(':
      count++;
      break;
    case ')':
      count--;
      if(count == 0 && p != q)
        return false;
      else if(count == 0 && p == q)
        return true;
      break;
    default:
      break;
    }
    p++;
  }
  return false;
}


uint32_t find_op(uint32_t p, uint32_t q){
  uint32_t current = p;
  uint32_t in_parentheses = 0;
  uint32_t i = 0;
  for( i = p; i <= q; i++){
    printf("i = %d",i);
    switch(tokens[i].type) {
    case '(':
      in_parentheses ++;
      break;
    case ')':
      in_parentheses --;
      break;

    default:
      if(!is_num(i) && in_parentheses == 0 && tokens[i].type > tokens[current].type)
      current = i;
      break;
    }
  }
  printf("current = %d \n", current);
  return current;
}

uint32_t eval(uint32_t p,uint32_t q){
  if(p > q) {
    /* Bad expression */
    panic("bad expression p = %d, q = %d", p, q);
  }
  else if(p == q) {
    /* Single token.
     * For now this token should be a number.
     * Return the value of the number.
     */
  switch(tokens[p].type){
  case NUM:
    return atoi(tokens[p].str);
  case REG_EAX:
    return cpu.eax;
  case REG_EBX:
    return cpu.ebx;
  case REG_EDX:
    return cpu.edx;
  case REG_ECX:
    return cpu.ecx;
  case REG_EBP:
    return cpu.ebp;
  case REG_ESI:
    return cpu.esi;
  case REG_ESP:
    return cpu.esp;
  case REG_EIP:
    return cpu.eip;
  default:return 0;
  }

  }
  else if(check_parentheses(p, q) == true) {
    /* The expression is surrounded by a matched pair of parentheses.

     * If that is the case, just throw away the parentheses.
     */
    return eval(p + 1, q - 1);
  }
  else {
    uint32_t op = find_op(p, q);

    uint32_t val2 = 0;

    uint32_t addr;

    //    printf("token type =  %d",tokens[op].type);
    switch(tokens[op].type) {
    case OR: return eval(p, op - 1) || eval(op + 1, q);
    case AND: return eval(p, op -1) && eval(op + 1, q);
    case EQ: return eval(p, op - 1 ) == eval(op + 1, q);
    case PLUS: return eval(p, op - 1) + eval(op + 1, q);
    case MINUS:return eval(p, op - 1) - eval(op + 1, q);
    case MULTIPLY: return eval(p, op - 1) * eval(op + 1, q);
    case DIVIDE:
      val2 = eval(op + 1, q);
      if(val2 == 0){
        printf("error, val2 == 0 \n");
        return 0;
      }
      return eval(p, op - 1) / val2;
    case NEGATIVE:
      return -eval(op + 1,q);
    case DEREF:
      return swaddr_read(eval(op + 1,q),1);
    case ADDR:
      sscanf(tokens[p + 1].str,"%x",&addr);
      return instr_fetch(addr,1);
    default: assert(0);
    }
  }
  return 0;
}

uint32_t expr(char *e, bool *success) {
	if(!make_token(e)) {
		*success = false;
		return 0;
	}

  uint32_t result;
	/* TODO: Insert codes to evaluate the expression. */
	//panic("please implement me");
  //printf("evaluate = %d",eval(0,nr_token - 1));
  result = eval(0,nr_token - 1);

  clear_tokens();
	return result;
}

