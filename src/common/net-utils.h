#ifndef NET_UTILS_H
#define NET_UTILS_H

#include <stddef.h>

int   is_url(const char *str);
char *download_url(const char *url, size_t *out_size);
int   download_and_extract(const char *url, const char *dest_dir);

#endif  // NET_UTILS_H
