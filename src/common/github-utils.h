#ifndef GITHUB_UTILS_H
#define GITHUB_UTILS_H

#include <stddef.h>

#include "net-utils.h"

char *github_default_branch(const char *full_name);
char *github_matching_ref(const char *full_name, const char *ref);

#endif  // GITHUB_UTILS_H
