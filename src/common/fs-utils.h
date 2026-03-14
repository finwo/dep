#ifndef FS_UTILS_H
#define FS_UTILS_H

#include <stddef.h>

#define REPO_DIR_DEFAULT  "/.config/finwo/dep/repositories.d/"
#define CACHE_DIR_DEFAULT "/.config/finwo/dep/repositories.cache/"

char *get_repo_dir(void);
char *get_cache_dir(void);
int   mkdir_recursive(const char *path);
char *trim_whitespace(char *str);

#endif  // FS_UTILS_H
