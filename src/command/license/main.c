#include <stdio.h>
#include <stdlib.h>

#include "command/command.h"
#include "license.h"

int cmd_license(int argc, const char **argv) {
  (void)argc;
  (void)argv;
  const unsigned char *start = _binary_LICENSE_md_start;
  const unsigned char *end   = _binary_LICENSE_md_end;
  size_t               len   = end - start;
  fwrite(start, 1, len, stdout);
  return 0;
}

void __attribute__((constructor)) cmd_license_setup() {
  struct cmd_struct *cmd = calloc(1, sizeof(struct cmd_struct));
  cmd->next              = commands;
  cmd->fn                = cmd_license;
  cmd->cmd               = "license";
  commands               = cmd;
}
