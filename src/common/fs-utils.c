#include "fs-utils.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

char *get_repo_dir(void) {
  const char *home = getenv("HOME");
  if (!home) {
    fprintf(stderr, "Error: HOME environment variable not set\n");
    return NULL;
  }
  size_t len  = strlen(home) + strlen(REPO_DIR_DEFAULT) + 1;
  char  *path = malloc(len);
  if (!path) {
    fprintf(stderr, "Error: out of memory\n");
    return NULL;
  }
  snprintf(path, len, "%s%s", home, REPO_DIR_DEFAULT);
  return path;
}

char *get_cache_dir(void) {
  const char *home = getenv("HOME");
  if (!home) {
    fprintf(stderr, "Error: HOME environment variable not set\n");
    return NULL;
  }
  size_t len  = strlen(home) + strlen(CACHE_DIR_DEFAULT) + 1;
  char  *path = malloc(len);
  if (!path) {
    fprintf(stderr, "Error: out of memory\n");
    return NULL;
  }
  snprintf(path, len, "%s%s", home, CACHE_DIR_DEFAULT);
  return path;
}

int mkdir_recursive(const char *path) {
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

char *trim_whitespace(char *str) {
  while (*str == ' ' || *str == '\t' || *str == '\n' || *str == '\r') str++;
  if (*str == '\0') return str;
  char *end = str + strlen(str) - 1;
  while (end > str && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) {
    *end = '\0';
    end--;
  }
  return str;
}
