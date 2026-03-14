#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "command/command.h"

static void print_global_help(void) {
  printf("Usage: dep [global options] <command> [command options]\n");
  printf("\n");
  printf("Global options:\n");
  printf("  n/a\n");
  printf("\n");
  printf("Commands:\n");

  struct cmd_struct *cmd = commands;
  while (cmd) {
    printf("  %-16s %s\n", cmd->display ? cmd->display : cmd->name[0], cmd->description ? cmd->description : "");
    cmd = cmd->next;
  }

  printf("\n");
  printf("Help topics:\n");
  printf("  global          This help text\n");

  cmd = commands;
  while (cmd) {
    printf("  %-16s More detailed explanation on the %s command\n", cmd->name[0], cmd->name[0]);
    cmd = cmd->next;
  }
}

static int cmd_help(int argc, const char **argv) {
  if (argc < 1) {
    print_global_help();
    return 0;
  }

  if (argc == 1 && (!strcmp(argv[0], "help") || !strcmp(argv[0], "h") || !strcmp(argv[0], "global"))) {
    print_global_help();
    return 0;
  }

  const char *topic = (argc > 1) ? argv[1] : argv[0];

  if (!strcmp(topic, "global")) {
    print_global_help();
    return 0;
  }

  struct cmd_struct *cmd = commands;
  while (cmd) {
    if (!strcmp(topic, cmd->name[0])) {
      if (cmd->help_text) {
        printf("%s\n", cmd->help_text);
      } else {
        printf("dep %s - %s\n\n", cmd->name[0], cmd->description);
        printf("  %s\n", cmd->display);
      }
      return 0;
    }
    cmd = cmd->next;
  }

  fprintf(stderr, "Error: unknown help topic '%s'\n", topic);
  fprintf(stderr, "Run 'dep help' for available topics.\n");
  return 1;
}

void __attribute__((constructor)) cmd_help_setup(void) {
  struct cmd_struct *cmd = calloc(1, sizeof(struct cmd_struct));
  if (!cmd) {
    fprintf(stderr, "Failed to allocate memory for help command\n");
    return;
  }
  cmd->next                       = commands;
  cmd->fn                         = cmd_help;
  static const char *help_names[] = {"help", "h", NULL};
  cmd->name                       = help_names;
  cmd->display                    = "help [topic]";
  cmd->description                = "Show this help or the top-level info about a command";
  cmd->help_text =
      "dep help - Show this help or the top-level info about a command\n"
      "\n"
      "Usage:\n"
      "  dep help\n"
      "  dep help <command>\n"
      "\n"
      "Description:\n"
      "  Show general help or detailed help for a specific command.\n"
      "\n"
      "Examples:\n"
      "  dep help              # Show general help\n"
      "  dep help add          # Show help for add command\n"
      "  dep help install     # Show help for install command\n";
  commands = cmd;
}
