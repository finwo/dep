#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "command/command.h"
#include "common/net-utils.h"
#include "emmanuel-marty/em_inflate.h"
#include "erkkah/naett.h"
#include "rxi/microtar.h"
#include "tidwall/json.h"

/* Forward declarations */
static char *query_github_default_branch(const char *full_name);
static char *query_github_matching_ref(const char *full_name, const char *ref);
static int   install_dependency(const char *name, const char *spec);

static int dir_exists(const char *path) {
  struct stat st;
  return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

static int mkdir_recursive(const char *path) {
  char   tmp[PATH_MAX];
  char  *p = NULL;
  size_t len;

  snprintf(tmp, sizeof(tmp), "%s", path);
  len = strlen(tmp);
  if (tmp[len - 1] == '/') {
    tmp[len - 1] = '\0';
  }

  for (p = tmp + 1; *p; p++) {
    if (*p == '/') {
      *p = '\0';
      mkdir(tmp, 0755);
      *p = '/';
    }
  }
  return mkdir(tmp, 0755);
}

static char *trim_whitespace(char *str) {
  while (*str == ' ' || *str == '\t' || *str == '\n' || *str == '\r') str++;
  if (*str == '\0') return str;
  char *end = str + strlen(str) - 1;
  while (end > str && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) {
    *end = '\0';
    end--;
  }
  return str;
}

static char *spec_to_url(const char *name, const char *spec) {
  if (strlen(spec) > 0 && is_url(spec)) {
    return strdup(spec);
  }

  char *full_ref = NULL;

  if (strlen(spec) > 0) {
    full_ref = query_github_matching_ref(name, spec);
    if (!full_ref) {
      fprintf(stderr, "Error: ref '%s' not found for %s\n", spec, name);
      return NULL;
    }
  } else {
    char *branch = query_github_default_branch(name);
    if (!branch) {
      fprintf(stderr, "Warning: could not determine default branch for %s, using 'main'\n", name);
      branch = strdup("main");
    }
    full_ref = malloc(256);
    if (full_ref) {
      snprintf(full_ref, 256, "refs/heads/%s", branch);
    }
    free(branch);
  }

  if (!full_ref) {
    fprintf(stderr, "Error: could not determine ref for %s\n", name);
    return NULL;
  }

  char *url = malloc(2048);
  if (!url) {
    free(full_ref);
    return NULL;
  }
  snprintf(url, 2048, "https://github.com/%s/archive/%s.tar.gz", name, full_ref);
  free(full_ref);
  return url;
}

static int process_dep_file_in_dir(const char *dep_dir) {
  char dep_path[PATH_MAX];
  snprintf(dep_path, sizeof(dep_path), "%s/.dep", dep_dir);
  FILE *f = fopen(dep_path, "r");
  if (!f) {
    // No .dep file is not an error
    return 0;
  }

  char line[LINE_MAX];
  while (fgets(line, sizeof(line), f)) {
    char *comment = strchr(line, '#');
    if (comment) *comment = '\0';

    char *trimmed = trim_whitespace(line);
    if (strlen(trimmed) == 0) continue;

    char name[256]  = {0};
    char spec[1024] = {0};

    char *at_pos = strchr(trimmed, '@');
    if (at_pos) {
      size_t name_len = at_pos - trimmed;
      strncpy(name, trimmed, name_len);
      name[name_len] = '\0';
      strcpy(spec, trim_whitespace(at_pos + 1));
    } else {
      strncpy(name, trimmed, sizeof(name) - 1);
      name[sizeof(name) - 1] = '\0';
    }

    name[strcspn(name, " \t")] = '\0';

    if (strlen(name) == 0) continue;

    install_dependency(name, spec);
  }

  fclose(f);
  return 0;
}

static char *query_github_default_branch(const char *full_name) {
  char url[512];
  snprintf(url, sizeof(url), "https://api.github.com/repos/%s", full_name);

  size_t size;
  char  *response = download_url(url, &size);
  if (!response) return NULL;

  struct json root           = json_parse(response);
  struct json default_branch = json_object_get(root, "default_branch");

  char *branch = NULL;
  if (json_exists(default_branch) && json_type(default_branch) == JSON_STRING) {
    size_t len = json_string_length(default_branch);
    branch     = malloc(len + 1);
    if (branch) {
      json_string_copy(default_branch, branch, len + 1);
    }
  }

  free(response);
  return branch;
}

static char *query_github_ref(const char *full_name, const char *ref_type, const char *ref) {
  char url[512];
  snprintf(url, sizeof(url), "https://api.github.com/repos/%s/git/ref/%s/%s", full_name, ref_type, ref);

  size_t size;
  char  *response = download_url(url, &size);
  if (!response) return NULL;

  struct json root    = json_parse(response);
  struct json ref_obj = json_object_get(root, "ref");

  char *full_ref = NULL;
  if (json_exists(ref_obj) && json_type(ref_obj) == JSON_STRING) {
    size_t len = json_string_length(ref_obj);
    full_ref   = malloc(len + 1);
    if (full_ref) {
      json_string_copy(ref_obj, full_ref, len + 1);
    }
  }

  free(response);
  return full_ref;
}

static char *query_github_matching_ref(const char *full_name, const char *ref) {
  char *full_ref = query_github_ref(full_name, "tags", ref);
  if (full_ref) return full_ref;

  return query_github_ref(full_name, "heads", ref);
}

static int install_dependency(const char *name, const char *spec) {
  char lib_path[PATH_MAX];
  snprintf(lib_path, sizeof(lib_path), "lib/%s", name);

  if (dir_exists(lib_path)) {
    printf("Skipping %s (already installed)\n", name);
    return 0;
  }

  char url[2048] = {0};

  if (strlen(spec) > 0 && is_url(spec)) {
    strncpy(url, spec, sizeof(url) - 1);
  } else {
    char *full_ref = NULL;

    if (strlen(spec) > 0) {
      full_ref = query_github_matching_ref(name, spec);
      if (!full_ref) {
        fprintf(stderr, "Error: ref '%s' not found for %s\n", spec, name);
        return -1;
      }
    } else {
      char *branch = query_github_default_branch(name);
      if (!branch) {
        fprintf(stderr, "Warning: could not determine default branch for %s, using 'main'\n", name);
        branch = strdup("main");
      }
      full_ref = malloc(256);
      if (full_ref) {
        snprintf(full_ref, 256, "refs/heads/%s", branch);
      }
      free(branch);
    }

    if (!full_ref) {
      fprintf(stderr, "Error: could not determine ref for %s\n", name);
      return -1;
    }

    snprintf(url, sizeof(url), "https://github.com/%s/archive/%s.tar.gz", name, full_ref);
    free(full_ref);
  }

  printf("Installing %s from %s\n", name, url);

  mkdir_recursive(lib_path);

  if (download_and_extract(url, lib_path) != 0) {
    fprintf(stderr, "Error: failed to install %s\n", name);
    return -1;
  }

  // Process .dep.chain recursively
  char dep_chain_path[PATH_MAX];
  while (1) {
    // Build .dep.chain path
    snprintf(dep_chain_path, sizeof(dep_chain_path), "%s/.dep.chain", lib_path);

    // Check if .dep.chain exists
    FILE *chain_file = fopen(dep_chain_path, "r");
    if (!chain_file) {
      break;  // No more chaining
    }

    // Read spec (single line)
    char spec[1024] = {0};
    if (!fgets(spec, sizeof(spec), chain_file)) {
      fclose(chain_file);
      fprintf(stderr, "Error: failed to read .dep.chain\n");
      return -1;
    }
    fclose(chain_file);

    // Delete .dep.chain file
    if (remove(dep_chain_path) != 0) {
      fprintf(stderr, "Warning: failed to remove .dep.chain\n");
      // Continue anyway - spec was read
    }

    // Trim whitespace/newline from spec
    char *trimmed = trim_whitespace(spec);
    if (strlen(trimmed) == 0) {
      fprintf(stderr, "Warning: empty spec in .dep.chain\n");
      continue;
    }

    printf("Found .dep.chain, chaining to: %s\n", trimmed);

    // Resolve spec to URL
    char *overlay_url = spec_to_url(name, trimmed);
    if (!overlay_url) {
      fprintf(stderr, "Error: failed to resolve chained spec '%s'\n", trimmed);
      return -1;
    }

    // Overlay extract (directly over existing files)
    printf("Overlaying %s from %s\n", name, overlay_url);
    if (download_and_extract(overlay_url, lib_path) != 0) {
      fprintf(stderr, "Error: failed to overlay chained dependency\n");
      free(overlay_url);
      return -1;
    }
    free(overlay_url);
    // Loop continues to check for new .dep.chain
  }

  // Process .dep file in the dependency's directory
  if (process_dep_file_in_dir(lib_path) != 0) {
    fprintf(stderr, "Warning: failed to process .dep file for %s\n", name);
    // Not returning error because the dependency itself installed successfully.
  }

  // Handle config.mk: append dependency's config.mk to lib/.deb/config.mk
  char deb_dir[PATH_MAX];
  snprintf(deb_dir, sizeof(deb_dir), "lib/.deb");
  mkdir_recursive(deb_dir);

  char dep_config_path[PATH_MAX];
  snprintf(dep_config_path, sizeof(dep_config_path), "%s/config.mk", lib_path);

  char deb_config_path[PATH_MAX];
  snprintf(deb_config_path, sizeof(deb_config_path), "lib/.deb/config.mk");

  FILE *dep_config = fopen(dep_config_path, "r");
  if (dep_config) {
    FILE *deb_config = fopen(deb_config_path, "a");
    if (deb_config) {
      char line[LINE_MAX];
      while (fgets(line, sizeof(line), dep_config)) {
        // Replace __DIRNAME and {__DIRNAME__} with the dependency's path (lib_path)
        char  modified[LINE_MAX * 2];  // enough space for replacements
        char *src = line;
        char *dst = modified;
        while (*src) {
          if (strncmp(src, "__DIRNAME", 9) == 0) {
            strcpy(dst, lib_path);
            dst += strlen(lib_path);
            src += 9;
          } else if (strncmp(src, "{__DIRNAME__}", 13) == 0) {
            strcpy(dst, lib_path);
            dst += strlen(lib_path);
            src += 13;
          } else {
            *dst++ = *src++;
          }
        }
        *dst = '\0';
        fputs(modified, deb_config);
      }
      // Ensure a newline at end of appended content if not already ending with newline
      // (optional, but we can add a newline to separate entries)
      fputc('\n', deb_config);
      fclose(deb_config);
    } else {
      fprintf(stderr, "Warning: could not open %s for appending\n", deb_config_path);
    }
    fclose(dep_config);
  }

  printf("Installed %s\n", name);
  return 0;
}

static int cmd_install(int argc, const char **argv) {
  (void)argc;
  (void)argv;

  const char *dep_path = ".dep";
  FILE       *f        = fopen(dep_path, "r");
  if (!f) {
    fprintf(stderr, "Error: .dep file not found. Run 'dep init' first.\n");
    return 1;
  }

  if (!dir_exists("lib")) {
    if (mkdir("lib", 0755) != 0) {
      fprintf(stderr, "Error: could not create lib directory\n");
      fclose(f);
      return 1;
    }
  }

  char line[LINE_MAX];
  int  has_deps = 0;

  while (fgets(line, sizeof(line), f)) {
    char *comment = strchr(line, '#');
    if (comment) *comment = '\0';

    char *trimmed = trim_whitespace(line);
    if (strlen(trimmed) == 0) continue;

    has_deps = 1;

    char name[256]  = {0};
    char spec[1024] = {0};

    char *at_pos = strchr(trimmed, '@');
    if (at_pos) {
      size_t name_len = at_pos - trimmed;
      strncpy(name, trimmed, name_len);
      name[name_len] = '\0';
      strcpy(spec, trim_whitespace(at_pos + 1));
    } else {
      strncpy(name, trimmed, sizeof(name) - 1);
      name[sizeof(name) - 1] = '\0';
    }

    name[strcspn(name, " \t")] = '\0';

    if (strlen(name) == 0) continue;

    install_dependency(name, spec);
  }

  fclose(f);

  if (!has_deps) {
    printf("No dependencies to install\n");
  }

  return 0;
}

void __attribute__((constructor)) cmd_install_setup(void) {
  struct cmd_struct *cmd = calloc(1, sizeof(struct cmd_struct));
  if (!cmd) {
    fprintf(stderr, "Failed to allocate memory for install command\n");
    return;
  }
  cmd->next                          = commands;
  cmd->fn                            = cmd_install;
  static const char *install_names[] = {"install", "i", NULL};
  cmd->name                          = install_names;
  commands                           = cmd;
}
