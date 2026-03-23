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
  {"si","si [N]",cmd_si},
  {"info","info r",cmd_info},
  {"x","x N EXPR",cmd_x},
  { "p", "p EXPR - evaluate expression", cmd_p },
  { "w", "w EXPR - set watchpoint", cmd_w },
  { "d", "d N - delete watchpoint N", cmd_d },

  /* TODO: Add more commands */

};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))
static int cmd_d(char *args) {
  if (args == NULL) {
    printf("Usage: d N\n");
    return 0;
  }

  int no = atoi(args);
  if (no < 0) {
    printf("Invalid watchpoint number\n");
    return 0;
  }

  WP *p = get_head();
  while (p != NULL) {
    if (p->NO == no) {
      free_wp(p);
      printf("Deleted watchpoint %d\n", no);
      return 0;
    }
    p = p->next;
  }

  printf("No watchpoint %d\n", no);
  return 0;
}
static int cmd_w(char *args) {
  if (args == NULL) {
    printf("Usage: w EXPR\n");
    return 0;
  }

  bool success = true;
  uint32_t val = expr(args, &success);
  if (!success) {
    printf("Bad expression: %s\n", args);
    return 0;
  }

  WP *wp = new_wp();
  strcpy(wp->expr, args);
  wp->old_value = val;

  printf("Watchpoint %d: %s = %u (0x%08x)\n",
         wp->NO, wp->expr, val, val);
  return 0;
}
static int cmd_x(char *args) {
  if (args == NULL) {
    printf("Usage: x N EXPR\n");
    return 0;
  }

  char *n_str = strtok(args, " ");
  char *expr_str = strtok(NULL, "");

  if (n_str == NULL || expr_str == NULL) {
    printf("Usage: x N EXPR\n");
    return 0;
  }

  while (*expr_str == ' ') expr_str++;

  int n = atoi(n_str);
  if (n <= 0) {
    printf("x: N should be positive\n");
    return 0;
  }

  bool success = true;
  uint32_t start_addr = expr(expr_str, &success);
  if (!success) {
    printf("Bad expression: %s\n", expr_str);
    return 0;
  }

  for (int i = 0; i < n; i++) {
    uint32_t addr = start_addr + i * 4;
    uint32_t data = vaddr_read(addr, 4);
    printf("0x%08x: 0x%08x\n", addr, data);
  }

  return 0;
}
static int cmd_p(char *args) {
  if (args == NULL) {
    printf("Usage: p EXPR\n");
    return 0;
  }

  bool success = true;
  uint32_t val = expr(args, &success);

  if (!success) {
    printf("Bad expression: %s\n", args);
    return 0;
  }

  printf("%u (0x%08x)\n", val, val);
  return 0;
}
static int cmd_info(char *args) {
  if (args == NULL) {
    printf("Usage: info r | info w\n");
    return 0;
  }

  if (strcmp(args, "r") == 0) {
    printf("eax            0x%08x %u\n", cpu.eax, cpu.eax);
    printf("ecx            0x%08x %u\n", cpu.ecx, cpu.ecx);
    printf("edx            0x%08x %u\n", cpu.edx, cpu.edx);
    printf("ebx            0x%08x %u\n", cpu.ebx, cpu.ebx);
    printf("esp            0x%08x %u\n", cpu.esp, cpu.esp);
    printf("ebp            0x%08x %u\n", cpu.ebp, cpu.ebp);
    printf("esi            0x%08x %u\n", cpu.esi, cpu.esi);
    printf("edi            0x%08x %u\n", cpu.edi, cpu.edi);
    printf("eip            0x%08x %u\n", cpu.eip, cpu.eip);
    return 0;
  }
  else if (strcmp(args, "w") == 0) {
    print_wp();
    return 0;
  }

  printf("Usage: info r | info w\n");
  return 0;
}
static int cmd_si(char *args){
	int step=1;
	if(args==NULL||args[0]=='\0')
	{
	cpu_exec(step);
	return 0;
	}
	step=atoi(args);
	cpu_exec(step);
	return 0;
	}

static int cmd_help(char *args) {
  char *arg = args;
  int i;

  if (arg == NULL) {
    for (i = 0; i < NR_CMD; i++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
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
