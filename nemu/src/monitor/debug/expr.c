#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>
#include <stdlib.h>

enum {
  TK_NOTYPE = 256, 

  TK_EQ,TK_INT,TK_HEX,TK_REG,TK_NEQ,TK_AND,TK_OR,TK_NOT,TK_DEREF,TK_NEGATIVE,

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
  {"0x[1-9A-Fa-f][0-9A-Fa-f]*",TK_HEX},
  {"0|[1-9][0-9]*", TK_INT},//integar

  {"\\$(eax|edx|ecx|ebx|ebp|esi|edi|esp|ax|dx|cx|bx|pc|bp|si|di|sp|al|dl|cl|bl|ah|dh|ch|bh|eip)",TK_REG},

  {"\\+", '+'},   // plus
  {"\\-", '-'}, 	// sub
  {"\\*", '*'}, 	// multiple
  {"\\/", '/'}, 	// divide
  {"\\)", ')'},		// left bracket
  {"\\(", '('}, 	// right bracket

  {"==", TK_EQ},  // equal
  {"!=",TK_NEQ},
  {"&&",TK_AND},
  {"\\|\\|",TK_OR},
  {"!",TK_NOT},
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
        
        if(substr_len>32){
          printf("Error: The token's length is out of range, at position %d with len %d.",position-substr_len,substr_len);
          return false;
        }

        /* TODO: Now a new token is recognized with rules[i]. Add codes
              * to record the token in the array `tokens'. For certain types
              * of tokens, some extra actions should be performed.
              */	
	
        if(rules[i].token_type==TK_NOTYPE)
        {
          break;
        }        
        else
        {
          tokens[nr_token].type=rules[i].token_type;
          switch (rules[i].token_type) {
            case TK_INT:
            {
              strncpy(tokens[nr_token].str,substr_start,substr_len);
              tokens[nr_token].str[substr_len]='\0';
              break;
            }
            case TK_HEX:
            {
              strncpy(tokens[nr_token].str,substr_start+2,substr_len-2);
              tokens[nr_token].str[substr_len-2]='\0';
              break;
            }
            case TK_REG:
            {
              strncpy(tokens[nr_token].str,substr_start+1,substr_len-1);
              tokens[nr_token].str[substr_len-1]='\0';
              break;
            }
          }
          printf("Success: Correctly identified:nr_token=%d,type=%d,str=%s\n",nr_token,tokens[nr_token].type,tokens[nr_token].str);
          nr_token+=1;
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
}//test

bool check_parentheses(int p,int q){
  if(p >= q)
  {
    printf("Error: p>q in check_parentheses in p=%d, q=%d.\n",p,q);
    return false;
  }
  int count_flag = 0;
  for(int i = p;i <= q;i++)
  {
    if(count_flag < 0)
    {
      printf("Error:'('and')' not matched in check_parentheses");
      assert(0);
    }
    if(tokens[i].type == '(')
    {
      count_flag+=1;
    }
    else if(tokens[i].type == ')')
    {
      count_flag-=1;
    }  
  }
  if(count_flag > 0)
  {
    printf("Error:'('and')' not matched in check_parentheses");
    assert(0);
  }
  if(tokens[p].type != '(' || tokens[q].type != ')')
  {
    return false;
  }
  else
  {
    return true;
  }
}

int find_dominant_operator(int p,int q){
  int priority[6] = {-1,-1,-1,-1,-1,-1};
  int count_flag = 0;
  for(int i = p;i <= q;i++)
  {
    if(count_flag < 0)
    {
      printf("Error:'('and')' not matched in check_parentheses");
      assert(0);
    }
    if(tokens[i].type == '(')
    {
      count_flag+=1;
      continue;
    }
    else if(tokens[i].type == ')')
    {
      count_flag-=1;
      continue;
    }
    if(count_flag == 0)
    {
      switch(tokens[i].type)
      {
        case TK_DEREF: case TK_NEGATIVE: case TK_NOT:
        {
          priority[5] =  i;
          break;
        }
        case '*': case '/':
        {
          priority[4] = i;
          break;
        }
        case '+': case '-':
        {
          priority[3] = i;
          break;
        }
        case TK_EQ: case TK_NEQ:
        {
          priority[2] = i;
          break;
        }
        case TK_AND:
        {
          priority[1] = i;
          break;
        }
        case TK_OR:
        {
          priority[0] = i;
          break;
        }
      }
    }
  }
  for(int i = 0;i < 6;i++)
  {
    if(priority[i] != -1)
    {
      return priority[i];
    }
  }
  printf("Error:Can't find a dominant operator!");
  assert(0);
}

uint32_t eval(int p,int q){
  if(p>q){
    printf("Error: p>q in eval() in p=%d, q=%d.\n",p,q);
    assert(0); 
  }
  else if(p==q){
    if(tokens[q].type==TK_INT){
      char *ptr;
      int r=strtoul(tokens[q].str,&ptr,10);
      return r;
    }
    else if(tokens[q].type==TK_HEX){
      char *ptr;
      int r=strtoul(tokens[q].str,&ptr,16);
      return r;
    }
    else if(tokens[q].type==TK_REG){
      for(int i=0;i<8;i++){
        if(strcmp(tokens[p].str,regsl[i])==0)
   	      return reg_l(i);
 	      if(strcmp(tokens[p].str,regsw[i])==0)
 	        return reg_w(i);
 	      if(strcmp(tokens[p].str,regsb[i])==0)
   	      return reg_b(i);
      }
 
      if(strcmp(tokens[p].str,"eip")==0)
        return cpu.eip;
      else{
        printf("Error:Not Match in TK_REG in p=%d, q=%d.\n",p,q);
        assert(0);
      }
    }
    else{
      printf("Error:Not Match in single token in p=%d,q=%d.\n",p,q);
      assert(0);
    }
  }
  else if (check_parentheses(p, q) == true) {
    return eval(p + 1, q - 1);
  }
  else
  {
    int op = find_dominant_operator(p,q);
    uint32_t val2 = eval(op + 1,q);
    switch(tokens[op].type)
    {
      case TK_NEGATIVE:
      {
        return -val2;
      }
      case TK_DEREF:
      {
        return vaddr_read(val2,4);
      }
      case TK_NOT:
      {
        return !val2;
      }
    }
    uint32_t val1 = eval(p,op - 1);
    switch(tokens[op].type)
    {
      case '+':
      {
        return val1 + val2;
      }
      case '-':
      {
        return val1 - val2;
      }
      case '*':
      {
        return val1 * val2;
      }
      case '/':
      {
        return val1 / val2;
      }
      case TK_EQ:
      {
        return val1 == val2;
      }
      case TK_NEQ:
      {
        return val1 != val2;
      }
      case TK_AND:
      {
        return val1 && val2;
      }
      case TK_OR:
      {
        return val1 || val2;
      }
      default:
      {
        printf("Error:Not Match in eval!");
        assert(0);
      }
    }
  }
}

uint32_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  for(int i = 0;i < nr_token;i++)
  {
    if(tokens[i].type == '-')
    {
      if(i == 0)
      {
        tokens[i].type = TK_NEGATIVE;
      }
      else if((tokens[i - 1].type != TK_INT) && (tokens[i - 1].type != TK_HEX) && (tokens[i - 1].type != TK_REG) && (tokens[i - 1].type != ')'))
      {
        tokens[i].type = TK_NEGATIVE;
      }
    }
    if(tokens[i].type == '*')
    {
      if(i == 0)
      {
        tokens[i].type = TK_DEREF;
      }
      else if((tokens[i - 1].type != TK_INT) && (tokens[i - 1].type != TK_HEX) && (tokens[i - 1].type != TK_REG) && (tokens[i - 1].type != ')'))
      {
        tokens[i].type = TK_DEREF;
      }
    }
  }

  return eval(0, nr_token - 1);
}
