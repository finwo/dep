#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

#include "command/command.h"

static int cmd_init(int argc, const char **argv) {
    const char *target_dir = ".";
    if (argc >= 2) {
        target_dir = argv[1];
    }

    // Check if target directory exists and is accessible
    if (access(target_dir, F_OK | X_OK) != 0) {
        fprintf(stderr, "Error: directory '%s' does not exist or is not accessible\n", target_dir);
        return 1;
    }

    // Build path to .dep file
    char dep_path[PATH_MAX];
    int ret = snprintf(dep_path, sizeof(dep_path), "%s/.dep", target_dir);
    if (ret < 0 || ret >= sizeof(dep_path)) {
        fprintf(stderr, "Error: path too long\n");
        return 1;
    }

    // Check if .dep already exists
    if (access(dep_path, F_OK) == 0) {
        printf("Target directory already initialized\n");
        return 0;
    }

    // Create empty .dep file
    FILE *f = fopen(dep_path, "w");
    if (!f) {
        fprintf(stderr, "Error: could not create .dep file in '%s'\n", target_dir);
        return 1;
    }
    fclose(f);

    printf("Initialized successfully\n");
    return 0;
}

void __attribute__((constructor)) cmd_init_setup(void) {
    struct cmd_struct *cmd = calloc(1, sizeof(struct cmd_struct));
    if (!cmd) {
        fprintf(stderr, "Failed to allocate memory for init command\n");
        return;
    }
    cmd->next = commands;
    cmd->fn = cmd_init;
    cmd->cmd = "init";
    commands = cmd;
}