#include "github-utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "net-utils.h"
#include "tidwall/json.h"

static char *github_ref(const char *full_name, const char *ref_type, const char *ref) {
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

char *github_default_branch(const char *full_name) {
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

char *github_matching_ref(const char *full_name, const char *ref) {
  char *full_ref = github_ref(full_name, "tags", ref);
  if (full_ref) return full_ref;

  return github_ref(full_name, "heads", ref);
}
