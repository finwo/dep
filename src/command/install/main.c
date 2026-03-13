#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "command/command.h"
#include "emmanuel-marty/em_inflate.h"
#include "erkkah/naett.h"
#include "rxi/microtar.h"
#include "tidwall/json.h"

/* Forward declarations */
static char *query_github_default_branch(const char *full_name);
static char *query_github_matching_ref(const char *full_name, const char *ref);
static int   install_dependency(const char *name, const char *spec);

typedef struct {
  char  *data;
  size_t size;
  size_t pos;
} membuffer_t;

static int is_url(const char *str) {
  return strncmp(str, "http://", 7) == 0 || strncmp(str, "https://", 8) == 0;
}

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

static int mem_read(mtar_t *tar, void *data, unsigned size) {
  membuffer_t *buf = (membuffer_t *)tar->stream;
  if (buf->pos + size > buf->size) {
    return MTAR_EREADFAIL;
  }
  memcpy(data, buf->data + buf->pos, size);
  buf->pos += size;
  return MTAR_ESUCCESS;
}

static int mem_seek(mtar_t *tar, unsigned pos) {
  membuffer_t *buf = (membuffer_t *)tar->stream;
  if (pos > buf->size) {
    return MTAR_ESEEKFAIL;
  }
  buf->pos = pos;
  return MTAR_ESUCCESS;
}

static int mem_close(mtar_t *tar) {
  (void)tar;
  return MTAR_ESUCCESS;
}

static int naett_initialized = 0;

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

static char *download_url(const char *url, size_t *out_size);

static char *download_url_with_retry(const char *url, size_t *out_size, int retries) {
  if (!naett_initialized) {
    naettInit(NULL);
    naett_initialized = 1;
  }
  naettReq *req = naettRequest(url, naettMethod("GET"), naettTimeout(30000));
  if (!req) {
    fprintf(stderr, "Error: failed to create request\n");
    return NULL;
  }

  naettRes *res = naettMake(req);
  if (!res) {
    naettFree(req);
    fprintf(stderr, "Error: failed to make request\n");
    return NULL;
  }

  while (!naettComplete(res)) {
    usleep(10000);
  }

  int         status    = naettGetStatus(res);
  const char *remaining = naettGetHeader(res, "X-RateLimit-Remaining");
  int         body_size = 0;
  const void *body      = naettGetBody(res, &body_size);

  if (status == 403 || status == 429 || status == 404) {
    if (remaining && strcmp(remaining, "0") == 0) {
      if (retries > 0) {
        fprintf(stderr, "Rate limited, waiting 5 seconds before retry...\n");
        naettClose(res);
        naettFree(req);
        sleep(5);
        return download_url_with_retry(url, out_size, retries - 1);
      }
    }
  }

  if (status != 200) {
    fprintf(stderr, "Error: HTTP status %d for %s\n", status, url);
    naettClose(res);
    naettFree(req);
    return NULL;
  }

  if (!body || body_size == 0) {
    if (retries > 0) {
      fprintf(stderr, "Empty response, waiting 5 seconds before retry...\n");
      naettClose(res);
      naettFree(req);
      sleep(5);
      return download_url_with_retry(url, out_size, retries - 1);
    }
    fprintf(stderr, "Error: empty response body\n");
    naettClose(res);
    naettFree(req);
    return NULL;
  }

  char *data = malloc(body_size);
  if (data) {
    memcpy(data, body, body_size);
    *out_size = body_size;
  }

  naettClose(res);
  naettFree(req);

  return data;
}

static char *download_url(const char *url, size_t *out_size) {
  return download_url_with_retry(url, out_size, 3);
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

static int download_and_extract(const char *url, const char *dest_dir) {
  size_t gzip_size;
  char  *gzip_data = download_url(url, &gzip_size);
  if (!gzip_data) {
    return -1;
  }

  if (gzip_size < 10 || (unsigned char)gzip_data[0] != 0x1f || (unsigned char)gzip_data[1] != 0x8b) {
    free(gzip_data);
    fprintf(stderr, "Error: downloaded data is not gzip format\n");
    return -1;
  }

  size_t max_tar_size = gzip_size * 10;
  char  *tar_data     = malloc(max_tar_size);
  if (!tar_data) {
    free(gzip_data);
    return -1;
  }

  size_t tar_size = em_inflate(gzip_data, gzip_size, (unsigned char *)tar_data, max_tar_size);
  free(gzip_data);

  if (tar_size == (size_t)-1 || tar_size == 0) {
    free(tar_data);
    fprintf(stderr, "Error: failed to decompress gzip data\n");
    return -1;
  }

  membuffer_t membuf = {.data = tar_data, .size = tar_size, .pos = 0};

  mtar_t tar;
  memset(&tar, 0, sizeof(tar));
  tar.read   = mem_read;
  tar.seek   = mem_seek;
  tar.close  = mem_close;
  tar.stream = &membuf;

  char first_component[256]  = {0};
  int  first_component_found = 0;

  while (1) {
    mtar_header_t h;
    int           err = mtar_read_header(&tar, &h);
    if (err == MTAR_ENULLRECORD) break;
    if (err != MTAR_ESUCCESS) {
      fprintf(stderr, "Error reading tar header: %s\n", mtar_strerror(err));
      break;
    }

    if (!first_component_found && (h.type == MTAR_TREG || h.type == MTAR_TDIR)) {
      char *slash = strchr(h.name, '/');
      if (slash) {
        size_t len = slash - h.name;
        strncpy(first_component, h.name, len);
        first_component[len]  = '\0';
        first_component_found = 1;
      }
    }

    char  full_path[PATH_MAX];
    char *name_ptr = h.name;

    if (first_component_found) {
      size_t first_len = strlen(first_component);
      if (strncmp(h.name, first_component, first_len) == 0 && h.name[first_len] == '/') {
        name_ptr = h.name + first_len + 1;
      }
    }

    if (strlen(name_ptr) == 0) {
      mtar_next(&tar);
      continue;
    }

    snprintf(full_path, sizeof(full_path), "%s/%s", dest_dir, name_ptr);

    if (h.type == MTAR_TDIR) {
      mkdir_recursive(full_path);
    } else if (h.type == MTAR_TREG) {
      char *last_slash = strrchr(full_path, '/');
      if (last_slash) {
        *last_slash = '\0';
        mkdir_recursive(full_path);
        *last_slash = '/';
      }

      FILE *f = fopen(full_path, "wb");
      if (!f) {
        fprintf(stderr, "Error: could not create file %s\n", full_path);
        mtar_next(&tar);
        continue;
      }

      char     buf[8192];
      unsigned remaining = h.size;
      while (remaining > 0) {
        unsigned to_read  = remaining > sizeof(buf) ? sizeof(buf) : remaining;
        int      read_err = mtar_read_data(&tar, buf, to_read);
        if (read_err != MTAR_ESUCCESS) {
          fprintf(stderr, "Error reading tar data\n");
          break;
        }
        fwrite(buf, 1, to_read, f);
        remaining -= to_read;
      }
      fclose(f);
    }

    mtar_next(&tar);
  }

  free(tar_data);
  return 0;
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
