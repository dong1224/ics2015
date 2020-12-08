
#include "monitor/monitor.h"
#include "monitor/expr.h"
#include "monitor/watchpoint.h"
#include "nemu.h"
#include "cpu/helper.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

void cpu_exec(uint32_t);

/* We use the ``readline'' library to provide more flexibility to read from stdin. */
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

static int cmd_si(char *args) {
  char *arg = strtok(NULL, " ");

  if(NULL == arg){
    cpu_exec(1);
    return 0;
  }else{
    int i = atoi(arg);
    if(i > 0){
      cpu_exec(i);
    }
  }
	return 0;
}

static int cmd_q(char *args) {
	return -1;
}

static int cmd_help(char *args);

static int cmd_info(char *args);

static int cmd_x(char *args);


static int cmd_p(char *args) {
  //char *arg = strtok(NULL, " ");
  bool success = 0;

  if(NULL == args){
    //cpu_exec(1);
    printf("no args, please check again!\n");
    return 0;
  }else{
    //printf("args %s \n",args);
    printf("the answer is %d\n",expr(args,&success));
  }
	return 0;
}


static struct {

	char *name;
	char *description;
	int (*handler) (char *);
} cmd_table [] = {
	{ "help", "Display informations about all supported commands", cmd_help },
	{ "c", "Continue the execution of the program", cmd_c },
  { "si", "Next step [N]", cmd_si },
	{ "q", "Exit NEMU", cmd_q },
  { "info", "Info register", cmd_info },
  { "x", "x address ", cmd_x },
  { "p", "p EXPR ",cmd_p},


	/* TODO: Add more commands */

};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))
#define NUM_REGISTER 9

static int cmd_help(char *args) {
	/* extract the first argument */
	char *arg = strtok(NULL, " ");
	int i;

	if(arg == NULL) {
		/* no argument given */
		for(i = 0; i < NR_CMD; i ++) {
			printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
		}
	}
	else {
		for(i = 0; i < NR_CMD; i ++) {
			if(strcmp(arg, cmd_table[i].name) == 0) {
				printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
				return 0;
			}
		}
		printf("Unknown command '%s'\n", arg);
	}
	return 0;
}

static int cmd_info(char *args){
	/* extract the first argument */
	char *arg = strtok(NULL, " ");
	//int i;

	if(arg == NULL) {
		/* no argument given */
    return 0;
	}

  if(strcmp(arg,"r") == 0){
    printf("eax:    0x%x        %d\n", cpu.eax,cpu.eax);
    printf("edx:    0x%x        %d\n", cpu.edx,cpu.edx);
    printf("ecx:    0x%x        %d\n", cpu.ecx,cpu.ecx);
    printf("ebx:    0x%x        %d\n", cpu.ebx,cpu.ebx);
    printf("ebp:    0x%x        %d\n", cpu.ebp,cpu.ebp);
    printf("esi:    0x%x        %d\n", cpu.esi,cpu.esi);
    printf("edi:    0x%x        %d\n", cpu.edi,cpu.edi);
    printf("esp:    0x%x        %d\n", cpu.esp,cpu.esp);
    printf("eip:    0x%x        %d\n", cpu.eip,cpu.eip);
    return 0;
	}

  return 0;
}

static int cmd_x(char *args){
  char *arg = strtok(NULL, " ");

  if(NULL == arg){
    return 0;
  }
  uint32_t lines = atoi(arg);
  if(lines > 100){
    printf("lines %d\n",lines);
    return 0;
  }

  arg = strtok(NULL, " ");

  if(NULL == arg){
    return 0;
  }
  uint32_t addr;
  sscanf(arg,"%x",&addr);
  printf("addr %x\n",addr);

  uint32_t i = 0;

  for(i = 0 ; i < lines ; i++ )
    printf("%02x \n",instr_fetch(addr + i,1));

  return 0;
}

void ui_mainloop() {
	while(1) {
		char *str = rl_gets();
		char *str_end = str + strlen(str);

		/* extract the first token as the command */
		char *cmd = strtok(str, " ");
		if(cmd == NULL) { continue; }

		/* treat the remaining string as the arguments,
		 * which may need further parsing
		 */
		char *args = cmd + strlen(cmd) + 1;
		if(args >= str_end) {
			args = NULL;
		}

#ifdef HAS_DEVICE
		extern void sdl_clear_event_queue(void);
		sdl_clear_event_queue();
#endif

		int i;
		for(i = 0; i < NR_CMD; i ++) {
			if(strcmp(cmd, cmd_table[i].name) == 0) {
				if(cmd_table[i].handler(args) < 0) { return; }
				break;
			}
		}

		if(i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
	}
}
