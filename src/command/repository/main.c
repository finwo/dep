#include <dirent.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../command.h"
#include "cofyc/argparse.h"
#include "common/fs-utils.h"

static int cmd_repository_list(int argc, const char **argv);
static int cmd_repository_add(int argc, const char **argv);
static int cmd_repository_remove(int argc, const char **argv);
static int cmd_repository_clean_cache(int argc, const char **argv);

static const char *const usages[] = {
    "repository <subcommand> [options]",
    NULL,
};

static int cmd_repository(int argc, const char **argv) {
  struct argparse_option options[] = {
      OPT_HELP(),
      OPT_END(),
  };
  struct argparse argparse;
  argparse_init(&argparse, options, usages, 0);
  argc = argparse_parse(&argparse, argc, argv);

  // If no subcommand provided, show available subcommands
  if (argc < 1) {
    printf("Available subcommands:\n");
    printf("  list          List the names of the repositories\n");
    printf("  add           Add a repository: add <name> <url>\n");
    printf("  remove        Remove a repository: remove <name>\n");
    printf("  clean-cache   Remove all cached manifest files\n");
    return 0;
  }

  // Dispatch to the appropriate subcommand handler
  if (!strcmp(argv[0], "list")) {
    return cmd_repository_list(argc - 1, argv + 1);
  } else if (!strcmp(argv[0], "add")) {
    return cmd_repository_add(argc - 1, argv + 1);
  } else if (!strcmp(argv[0], "remove")) {
    return cmd_repository_remove(argc - 1, argv + 1);
  } else if (!strcmp(argv[0], "clean-cache")) {
    return cmd_repository_clean_cache(argc - 1, argv + 1);
  } else {
    fprintf(stderr, "Error: unknown subcommand '%s'\n", argv[0]);
    return 1;
  }
}

// List all repository names from files in the repository directory
// Files are parsed line by line, with '#' starting comments
static int cmd_repository_list(int argc, const char **argv) {
  (void)argc;
  (void)argv;

  char *repo_dir = get_repo_dir();
  if (!repo_dir) {
    return 1;
  }
  DIR *dir = opendir(repo_dir);
  free(repo_dir);
  if (!dir) {
    fprintf(stderr, "Error: could not open repository directory\n");
    return 1;
  }

  struct dirent *entry;
  // Iterate through all files in the repository directory
  while ((entry = readdir(dir)) != NULL) {
    // Skip . and .. entries
    if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) continue;

    struct stat st;
    char       *repo_dir2 = get_repo_dir();
    if (!repo_dir2) {
      closedir(dir);
      return 1;
    }
    char filepath[PATH_MAX];
    snprintf(filepath, sizeof(filepath), "%s%s", repo_dir2, entry->d_name);
    free(repo_dir2);
    // Skip non-regular files
    if (stat(filepath, &st) < 0 || !S_ISREG(st.st_mode)) continue;

    FILE *f = fopen(filepath, "r");
    if (!f) continue;

    // Parse each line in the file
    char line[LINE_MAX];
    while (fgets(line, sizeof(line), f)) {
      // Remove trailing newline
      line[strcspn(line, "\n")] = '\0';
      // Strip comments (everything after '#')
      char *comment = strchr(line, '#');
      if (comment) *comment = '\0';
      // Extract the first word as the repository name
      char *name = strtok(line, " \t");
      if (name && name[0] != '\0') {
        printf("%s\n", name);
      }
    }
    fclose(f);
  }

  closedir(dir);
  return 0;
}

// Add a repository entry to the 00-managed file
// Format: <name> <url>
static int cmd_repository_add(int argc, const char **argv) {
  if (argc < 2) {
    fprintf(stderr, "Error: add requires <name> and <url>\n");
    fprintf(stderr, "Usage: repository add <name> <url>\n");
    return 1;
  }

  const char *name = argv[0];
  const char *url  = argv[1];

  // Build the file path: ~/.config/finwo/dep/repositories.d/00-managed
  char *repo_dir = get_repo_dir();
  if (!repo_dir) {
    return 1;
  }
  char filepath[PATH_MAX];
  snprintf(filepath, sizeof(filepath), "%s00-managed", repo_dir);
  free(repo_dir);

  // Open the file in append mode
  FILE *f = fopen(filepath, "a");
  if (!f) {
    fprintf(stderr, "Error: could not open file '%s' for writing\n", filepath);
    return 1;
  }

  // Write the repository entry: name url
  fprintf(f, "%s %s\n", name, url);
  fclose(f);

  printf("Repository '%s' added.\n", name);
  return 0;
}

// Remove a repository entry by name from all files in the repository directory
// If a file becomes empty after removal, it is deleted
static int cmd_repository_remove(int argc, const char **argv) {
  if (argc < 1) {
    fprintf(stderr, "Error: remove requires <name>\n");
    fprintf(stderr, "Usage: repository remove <name>\n");
    return 1;
  }

  const char *name  = argv[0];
  int         found = 0;

  char *repo_dir = get_repo_dir();
  if (!repo_dir) {
    return 1;
  }
  DIR *dir = opendir(repo_dir);
  free(repo_dir);
  if (!dir) {
    fprintf(stderr, "Error: could not open repository directory\n");
    return 1;
  }

  struct dirent *entry;
  // Iterate through all files in the repository directory
  while ((entry = readdir(dir)) != NULL) {
    // Skip . and .. entries
    if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) continue;

    char *repo_dir2 = get_repo_dir();
    if (!repo_dir2) {
      closedir(dir);
      return 1;
    }
    char filepath[PATH_MAX];
    snprintf(filepath, sizeof(filepath), "%s%s", repo_dir2, entry->d_name);
    free(repo_dir2);

    struct stat st;
    // Skip non-regular files
    if (stat(filepath, &st) < 0 || !S_ISREG(st.st_mode)) continue;

    // Create a temporary file for writing the modified content
    char temp_path[PATH_MAX];
    snprintf(temp_path, sizeof(temp_path), "%s.tmp", filepath);

    FILE *in  = fopen(filepath, "r");
    FILE *out = fopen(temp_path, "w");
    if (!in || !out) {
      fprintf(stderr, "Error: could not open files for processing '%s'\n", filepath);
      if (in) fclose(in);
      if (out) fclose(out);
      closedir(dir);
      return 1;
    }

    // Process each line, removing lines that match the repository name
    char line[LINE_MAX];
    while (fgets(line, sizeof(line), in)) {
      // Make a copy for parsing (preserving original line for output)
      char line_copy[LINE_MAX];
      strncpy(line_copy, line, sizeof(line_copy) - 1);
      line_copy[sizeof(line_copy) - 1] = '\0';

      // Strip comments before matching
      line_copy[strcspn(line_copy, "\n")] = '\0';
      char *comment                       = strchr(line_copy, '#');
      if (comment) *comment = '\0';
      char *line_name = strtok(line_copy, " \t");

      // Skip lines that match the repository name
      if (line_name && !strcmp(line_name, name)) {
        found = 1;
        continue;
      }
      // Write the original line back to the temp file
      fputs(line, out);
    }

    fclose(in);
    fclose(out);

    // Replace original file with the temporary file
    if (rename(temp_path, filepath) < 0) {
      fprintf(stderr, "Error: could not replace file '%s'\n", filepath);
      closedir(dir);
      return 1;
    }

    // If the file is now empty, remove it
    if (stat(filepath, &st) == 0 && st.st_size == 0) {
      if (remove(filepath) < 0) {
        fprintf(stderr, "Error: could not remove empty file '%s'\n", filepath);
        closedir(dir);
        return 1;
      }
    }
  }

  closedir(dir);

  if (!found) {
    fprintf(stderr, "Warning: repository '%s' not found\n", name);
  } else {
    printf("Repository '%s' removed.\n", name);
  }

  return 0;
}

static int cmd_repository_clean_cache(int argc, const char **argv) {
  (void)argc;
  (void)argv;

  char *cache_dir = get_cache_dir();
  if (!cache_dir) {
    return 1;
  }

  DIR *dir = opendir(cache_dir);
  if (!dir) {
    free(cache_dir);
    return 0;
  }

  struct dirent *entry;
  int            removed = 0;

  while ((entry = readdir(dir)) != NULL) {
    if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) continue;

    char filepath[PATH_MAX];
    snprintf(filepath, sizeof(filepath), "%s%s", cache_dir, entry->d_name);

    struct stat st;
    if (stat(filepath, &st) < 0 || !S_ISREG(st.st_mode)) continue;

    if (remove(filepath) == 0) {
      removed++;
    }
  }

  closedir(dir);
  free(cache_dir);

  printf("Removed %d cached manifest file%s.\n", removed, removed == 1 ? "" : "s");
  return 0;
}

// Register the repository command with the command system
void __attribute__((constructor)) cmd_repository_setup(void) {
  struct cmd_struct *cmd = calloc(1, sizeof(struct cmd_struct));
  if (!cmd) {
    fprintf(stderr, "Failed to allocate memory for repository command\n");
    return;
  }
  cmd->next                             = commands;
  cmd->fn                               = cmd_repository;
  static const char *repository_names[] = {"repository", "repo", "r", NULL};
  cmd->name                             = repository_names;
  commands                              = cmd;
}
