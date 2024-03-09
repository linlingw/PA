#include "monitor/monitor.h"
#include "monitor/expr.h"
#include "monitor/watchpoint.h"
#include "nemu.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

void cpu_exec(uint64_t);

/* We use the `readline' library to provide more flexibility to read from stdin. */
char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}

static int cmd_q(char *args) {
  return -1;
}

static int cmd_help(char *args);

static int cmd_si(char *args);

static int cmd_info(char *args);

static int cmd_x(char *args);

static int cmd_p(char *args);

static int cmd_w(char *args);

static int cmd_d(char *args);

static struct {
  char *name;
  char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display informations about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  { "si", "Execute [N](1 as default) instructions step by step", cmd_si},
  { "info", "Args:r/w; Eg:info r; Erint information about registers or watchpoint", cmd_info},
  { "x", "Scan Your Memory", cmd_x},
  { "p", "Caluculate the value of the expression", cmd_p },
  { "w", "Set a watching point", cmd_w },
  { "d", "Delete a watching point", cmd_d },

  /* TODO: Add more commands */

};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

static int cmd_si(char *args){  
    char *arg = strtok(NULL," ");  
    int steps = 0;  
    if(arg == NULL){  
        cpu_exec(1);  
        return 0;  
    }  
    int i = sscanf(arg, "%d", &steps);  
    if(i<-1){  
        printf("Error，N is an integer greater than or equal to -1\n");  
        return 0;  
    }   
    cpu_exec(steps);
    return 0;
}

static int cmd_info(char *args)  {  
    char *arg=strtok(NULL," ");  
    if(strcmp(arg,"r") == 0){  
        for(int i=0;i<8;i++)  
          printf("%s \t0x%x\n",regsl[i],reg_l(i));  
        printf("eip \t0x%x\n", cpu.eip); 
        for(int i=0;i<8;i++)  
          printf("%s \t0x%x\n",regsw[i],reg_w(i));
        for(int i=0;i<8;i++)  
          printf("%s \t0x%x\n",regsb[i],reg_b(i));

        printf("eflags:CF=%d,ZF=%d,SF=%d,IF=%d,OF=%d\n", cpu.eflags.CF, cpu.eflags.ZF, cpu.eflags.SF, cpu.eflags.IF, cpu.eflags.OF);

        printf("cr0=0x%x,cr3=0x%x\n", cpu.cr0, cpu.cr3);
    }
    else if(strcmp(arg, "w") == 0)
    {
      print_wp();
    }
    else
    {
      printf("Invalid arguments\n");
    }
    return 0;  
} 

static int cmd_x(char *args){  
    char *N = strtok(NULL," ");  
    char *EXPR = strtok(NULL," ");  
    int len;  
    vaddr_t address;  
      
    sscanf(N, "%d", &len);  
    sscanf(EXPR, "%x", &address);  
      
    printf("0x%x:",address);  
    int i;
    for(i = 0; i < len; i ++){  
        printf("%08x ",vaddr_read(address,4));  
        address += 4;  
    }  
    printf("\n");  
    return 0;  
}

static int cmd_p(char *args){
  char *arg = strtok(NULL, "\n");
  bool success = true;
  uint32_t result = expr(arg, &success);
  if(success){
    printf("The value of the expression is %u\n", result);
  }
  else{
    printf("Error in cmd_p!\n");
  }
  return 0;
}

static int cmd_w(char *args) {
  new_wp(args);
  return 0;
}//test

static int cmd_d(char *args) {
  char* arg = strtok(NULL, "\n");
  int num = -1;
  sscanf(arg, "%d", &num);
  WP* t_wp = find_point(num);
  if(num <= 0)
  {
    printf("Error: Args error in cmd_x\n");
    return 0;
  }
  if(t_wp)
  {
    free_wp(t_wp);
  }
  else
  {
    printf("No watchpoint with id %d!\n", num);
  }
  return 0;
}

void ui_mainloop(int is_batch_mode) {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  while (1) {
    char *str = rl_gets();
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef HAS_IOE
    extern void sdl_clear_event_queue(void);
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}
