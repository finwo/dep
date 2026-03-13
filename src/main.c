#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cofyc/argparse.h"
#include "command/command.h"
#include "erkkah/naett.h"
#include "rxi/microtar.h"

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

struct cmd_struct *commands = NULL;

static const char *const usages[] = {
    "subcommands [options] [cmd] [args]",
    NULL,
};

int cmd_foo(int argc, const char **argv) {
  printf("executing subcommand foo\n");
  printf("argc: %d\n", argc);
  for (int i = 0; i < argc; i++) {
    printf("argv[%d]: %s\n", i, *(argv + i));
  }
  int                    force     = 0;
  int                    test      = 0;
  const char            *path      = NULL;
  struct argparse_option options[] = {
      OPT_HELP(),
      OPT_BOOLEAN('f', "force", &force, "force to do", NULL, 0, 0),
      OPT_BOOLEAN('t', "test", &test, "test only", NULL, 0, 0),
      OPT_STRING('p', "path", &path, "path to read", NULL, 0, 0),
      OPT_END(),
  };
  struct argparse argparse;
  argparse_init(&argparse, options, usages, 0);
  argc = argparse_parse(&argparse, argc, argv);
  printf("after argparse_parse:\n");
  printf("argc: %d\n", argc);
  for (int i = 0; i < argc; i++) {
    printf("argv[%d]: %s\n", i, *(argv + i));
  }
  return 0;
}

int cmd_bar(int argc, const char **argv) {
  printf("executing subcommand bar\n");
  for (int i = 0; i < argc; i++) {
    printf("argv[%d]: %s\n", i, *(argv + i));
  }
  return 0;
}

int main(int argc, const char **argv) {
  struct argparse        argparse;
  struct argparse_option options[] = {
      OPT_HELP(),
      OPT_END(),
  };
  argparse_init(&argparse, options, usages, ARGPARSE_STOP_AT_NON_OPTION);
  argc = argparse_parse(&argparse, argc, argv);
  if (argc < 1) {
    argparse_usage(&argparse);
    return -1;
  }

   /* Try to run command with args provided. */
   struct cmd_struct *cmd = commands;
   while (cmd) {
     const char **name = cmd->name;
     while (*name) {
       if (!strcmp(*name, argv[0])) {
         goto found;
       }
       name++;
     }
     cmd = cmd->next;
   }
   cmd = NULL;
found:
   if (cmd) {
     return cmd->fn(argc, argv);
   }
  return 0;
}
