#include <dirent.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "cofyc/argparse.h"
#include "command/command.h"
#include "common/fs-utils.h"
#include "common/github-utils.h"
#include "common/net-utils.h"

#define CACHE_MAX_AGE_SECONDS (7 * 24 * 60 * 60)

static unsigned long hash_string(const char *str) {
  unsigned long hash = 5381;
  int           c;
  while ((c = *str++)) {
    hash = ((hash << 5) + hash) + c;
  }
  return hash;
}

static void url_to_cache_filename(const char *url, char *filename, size_t size) {
  unsigned long hash = hash_string(url);
  snprintf(filename, size, "%lu", hash);
}

static int is_cache_outdated(const char *cache_path) {
  struct stat st;
  if (stat(cache_path, &st) != 0) {
    return 1;
  }
  time_t now      = time(NULL);
  time_t file_age = now - st.st_mtime;
  return file_age > CACHE_MAX_AGE_SECONDS;
}

static int extract_version_from_depname(const char *depname_with_version, char *depname, char *version,
                                        size_t depname_size, size_t version_size) {
  const char *at_pos = strchr(depname_with_version, '@');
  if (!at_pos) {
    strncpy(depname, depname_with_version, depname_size - 1);
    depname[depname_size - 1] = '\0';
    version[0]                = '\0';
    return 0;
  }

  size_t depname_len = at_pos - depname_with_version;
  if (depname_len >= depname_size) {
    depname_len = depname_size - 1;
  }
  strncpy(depname, depname_with_version, depname_len);
  depname[depname_len] = '\0';

  strncpy(version, at_pos + 1, version_size - 1);
  version[version_size - 1] = '\0';
  return 0;
}

static int parse_manifest_line(const char *line, char *depname, char *version, char *url, size_t depname_size,
                               size_t version_size, size_t url_size) {
  char *line_copy = strdup(line);
  if (!line_copy) return -1;

  char *comment = strchr(line_copy, '#');
  if (comment) *comment = '\0';

  char *trimmed = trim_whitespace(line_copy);
  if (strlen(trimmed) == 0) {
    free(line_copy);
    return -1;
  }

  char *space_pos = strchr(trimmed, ' ');
  char *tab_pos   = strchr(trimmed, '\t');
  char *first_ws  = NULL;

  if (space_pos && tab_pos) {
    first_ws = (space_pos < tab_pos) ? space_pos : tab_pos;
  } else if (space_pos) {
    first_ws = space_pos;
  } else if (tab_pos) {
    first_ws = tab_pos;
  }

  int result = 0;

  if (first_ws) {
    size_t name_len = first_ws - trimmed;
    if (name_len >= depname_size) {
      name_len = depname_size - 1;
    }
    strncpy(depname, trimmed, name_len);
    depname[name_len] = '\0';

    char *url_start = trim_whitespace(first_ws + 1);
    strncpy(url, url_start, url_size - 1);
    url[url_size - 1] = '\0';
  } else {
    strncpy(depname, trimmed, depname_size - 1);
    depname[depname_size - 1] = '\0';
    url[0]                    = '\0';
  }

  char version_from_depname[256];
  extract_version_from_depname(depname, depname, version_from_depname, depname_size, sizeof(version_from_depname));

  if (version_from_depname[0] != '\0') {
    strncpy(version, version_from_depname, version_size - 1);
    version[version_size - 1] = '\0';
  } else {
    version[0] = '\0';
  }

  free(line_copy);
  return result;
}

static int version_matches(const char *requested, const char *available) {
  if (!requested || requested[0] == '\0') {
    return 1;
  }
  if (!available || available[0] == '\0') {
    return 0;
  }
  return strcmp(requested, available) == 0;
}

static int append_to_dep_file(const char *name, const char *spec) {
  const char *dep_path = ".dep";
  FILE       *f        = fopen(dep_path, "a");
  if (!f) {
    fprintf(stderr, "Error: could not open .dep file for writing\n");
    return -1;
  }

  fseek(f, 0, SEEK_END);
  long pos = ftell(f);
  if (pos > 0) {
    fputc('\n', f);
  }

  if (spec && spec[0] != '\0') {
    fprintf(f, "%s %s\n", name, spec);
  } else {
    fprintf(f, "%s\n", name);
  }

  fclose(f);
  return 0;
}

static int add_from_url(const char *name, const char *url) {
  int result = append_to_dep_file(name, url);
  if (result == 0) {
    printf("Added %s to .dep\n", name);
  }
  return result;
}

static int add_from_repository(const char *name, const char *requested_version) {
  char *repo_dir = get_repo_dir();
  if (!repo_dir) {
    return -1;
  }

  char *cache_dir = get_cache_dir();
  if (!cache_dir) {
    free(repo_dir);
    return -1;
  }

  mkdir_recursive(cache_dir);

  DIR *dir = opendir(repo_dir);
  if (!dir) {
    fprintf(stderr, "Error: could not open repository directory\n");
    free(repo_dir);
    free(cache_dir);
    return -1;
  }

  struct dirent *entry;
  int            found = 0;

  while (!found && (entry = readdir(dir)) != NULL) {
    if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) continue;

    char *repo_dir2 = get_repo_dir();
    if (!repo_dir2) continue;

    char filepath[PATH_MAX];
    snprintf(filepath, sizeof(filepath), "%s%s", repo_dir2, entry->d_name);
    free(repo_dir2);

    struct stat st;
    if (stat(filepath, &st) < 0 || !S_ISREG(st.st_mode)) continue;

    FILE *repo_file = fopen(filepath, "r");
    if (!repo_file) continue;

    char line[LINE_MAX];
    while (!found && fgets(line, sizeof(line), repo_file)) {
      char *comment = strchr(line, '#');
      if (comment) *comment = '\0';

      char *trimmed = trim_whitespace(line);
      if (strlen(trimmed) == 0) continue;

      char repo_name[256]     = {0};
      char manifest_url[1024] = {0};

      char *space_pos = strchr(trimmed, ' ');
      char *tab_pos   = strchr(trimmed, '\t');
      char *first_ws  = NULL;

      if (space_pos && tab_pos) {
        first_ws = (space_pos < tab_pos) ? space_pos : tab_pos;
      } else if (space_pos) {
        first_ws = space_pos;
      } else if (tab_pos) {
        first_ws = tab_pos;
      }

      if (!first_ws) continue;

      size_t name_len = first_ws - trimmed;
      if (name_len >= sizeof(repo_name)) {
        name_len = sizeof(repo_name) - 1;
      }
      strncpy(repo_name, trimmed, name_len);
      repo_name[name_len] = '\0';

      char *url_start = trim_whitespace(first_ws + 1);
      strncpy(manifest_url, url_start, sizeof(manifest_url) - 1);
      manifest_url[sizeof(manifest_url) - 1] = '\0';

      if (strlen(manifest_url) == 0) continue;

      char cache_filename[256];
      url_to_cache_filename(manifest_url, cache_filename, sizeof(cache_filename));

      char cache_path[PATH_MAX];
      snprintf(cache_path, sizeof(cache_path), "%s%s", cache_dir, cache_filename);

      if (is_cache_outdated(cache_path)) {
        printf("Updating cache for repository %s...\n", repo_name);
        size_t size;
        char  *manifest_content = download_url(manifest_url, &size);
        if (manifest_content) {
          FILE *cache_file = fopen(cache_path, "w");
          if (cache_file) {
            fwrite(manifest_content, 1, size, cache_file);
            fclose(cache_file);
          }
          free(manifest_content);
        }
      }

      FILE *cache_file = fopen(cache_path, "r");
      if (!cache_file) continue;

      char manifest_line[LINE_MAX];
      while (!found && fgets(manifest_line, sizeof(manifest_line), cache_file)) {
        char depname[256]      = {0};
        char version[256]      = {0};
        char tarball_url[1024] = {0};

        if (parse_manifest_line(manifest_line, depname, version, tarball_url, sizeof(depname), sizeof(version),
                                sizeof(tarball_url)) != 0) {
          continue;
        }

        if (strcmp(depname, name) != 0) continue;

        if (!version_matches(requested_version, version)) continue;

        if (strlen(tarball_url) == 0) continue;

        printf("Found %s", name);
        if (version[0] != '\0') {
          printf(" version %s", version);
        }
        printf(" in repository %s\n", repo_name);

        fclose(cache_file);
        closedir(dir);
        free(repo_dir);
        free(cache_dir);
        return append_to_dep_file(name, tarball_url);
      }

      fclose(cache_file);
    }

    fclose(repo_file);
  }

  closedir(dir);
  free(repo_dir);
  free(cache_dir);

  return found ? 0 : -1;
}

static int add_from_github(const char *name, const char *requested_version) {
  char *full_ref = NULL;

  if (requested_version && requested_version[0] != '\0') {
    full_ref = github_matching_ref(name, requested_version);
    if (!full_ref) {
      fprintf(stderr, "Error: ref '%s' not found for %s on GitHub\n", requested_version, name);
      return -1;
    }
    free(full_ref);
    printf("Found %s version %s on GitHub\n", name, requested_version);
    fflush(stdout);
    return append_to_dep_file(name, requested_version);
  } else {
    char *branch = github_default_branch(name);
    if (!branch) {
      fprintf(stderr, "Error: could not determine default branch for %s on GitHub\n", name);
      return -1;
    }
    printf("Found %s on GitHub (default branch: %s)\n", name, branch);
    fflush(stdout);
    free(branch);
    return append_to_dep_file(name, "");
  }
}

static int cmd_add(int argc, const char **argv) {
  struct argparse_option options[] = {
      OPT_HELP(),
      OPT_END(),
  };

  struct argparse argparse;
  argparse_init(&argparse, options, NULL, 0);
  argc = argparse_parse(&argparse, argc, argv);

  if (argc < 1) {
    fprintf(stderr, "Error: add requires <name> [version]\n");
    fprintf(stderr, "Usage: dep add <name> [version]\n");
    return 1;
  }

  const char *name    = argv[0];
  const char *version = (argc > 1) ? argv[1] : NULL;

  if (version && is_url(version)) {
    printf("Adding %s as direct URL: %s\n", name, version);
    return add_from_url(name, version);
  }

  printf("Searching repositories for %s", name);
  if (version && strlen(version) > 0) {
    printf(" with version %s", version);
  }
  printf("...\n");
  fflush(stdout);

  int result = add_from_repository(name, version);
  if (result == 0) {
    printf("Added %s to .dep\n", name);
    fflush(stdout);
    return 0;
  }

  printf("Not found in repositories, trying GitHub...\n");
  fflush(stdout);
  result = add_from_github(name, version);
  if (result == 0) {
    printf("Added %s to .dep\n", name);
    return 0;
  }

  fprintf(stderr, "Error: package '%s' not found\n", name);
  return 1;
}

void __attribute__((constructor)) cmd_add_setup(void) {
  struct cmd_struct *cmd = calloc(1, sizeof(struct cmd_struct));
  if (!cmd) {
    fprintf(stderr, "Failed to allocate memory for add command\n");
    return;
  }
  cmd->next                      = commands;
  cmd->fn                        = cmd_add;
  static const char *add_names[] = {"add", "a", NULL};
  cmd->name                      = add_names;
  cmd->display                   = "a(dd)";
  cmd->description               = "Add a new dependency to the project";
  cmd->help_text =
      "dep add - Add a new dependency to the project\n"
      "\n"
      "Usage:\n"
      "  dep add <name>\n"
      "  dep add <name> <version>\n"
      "  dep add <name> <url>\n"
      "\n"
      "Description:\n"
      "  Add a package to the project's .dep file.\n"
      "\n"
      "  If a version is not specified, the latest version from the repository\n"
      "  will be used, or the default branch from GitHub.\n"
      "\n"
      "  You can also add a dependency with a direct URL.\n"
      "\n"
      "Examples:\n"
      "  dep add finwo/palloc           # Latest from repo or GitHub main branch\n"
      "  dep add finwo/palloc edge     # Specific version/branch\n"
      "  dep add finwo/palloc v1.0.0   # Specific tag\n"
      "  dep add mylib https://example.com/mylib.tar.gz\n";
  commands = cmd;
}
