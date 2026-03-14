#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "emmanuel-marty/em_inflate.h"
#include "erkkah/naett.h"
#include "rxi/microtar.h"
#include "tidwall/json.h"

/* Forward declarations for helpers */
static int mem_read(mtar_t *tar, void *data, unsigned size);
static int mem_seek(mtar_t *tar, unsigned pos);
static int mem_close(mtar_t *tar);

/* Membuffer structure used by the tar memory stream */
typedef struct {
  char  *data;
  size_t size;
  size_t pos;
} membuffer_t;

/* Check if a string looks like a URL */
int is_url(const char *str) {
  return strncmp(str, "http://", 7) == 0 || strncmp(str, "https://", 8) == 0;
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

/* Static helper for downloading with retries */
static char *download_url_with_retry(const char *url, size_t *out_size, int retries) {
  static int naett_initialized = 0;

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

/* Public download URL function */
char *download_url(const char *url, size_t *out_size) {
  return download_url_with_retry(url, out_size, 3);
}

/* Memory tar callbacks */
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

/* Download and extract a tar.gz URL into a directory */
int download_and_extract(const char *url, const char *dest_dir) {
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

  size_t max_tar_size = gzip_size * 15;
  char  *tar_data     = malloc(max_tar_size);
  if (!tar_data) {
    free(gzip_data);
    return -1;
  }

  size_t tar_size = em_inflate(gzip_data, gzip_size, (unsigned char *)tar_data, max_tar_size);
  free(gzip_data);

  if (tar_size == (size_t)-1) {
    free(tar_data);
    fprintf(stderr, "Error: decompression failed (invalid or corrupted gzip data)\n");
    return -1;
  }
  if (tar_size == 0) {
    free(tar_data);
    fprintf(stderr, "Error: decompressed to empty (likely wrong format or truncated data)\n");
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

      // Skip if file already exists
      if (access(full_path, F_OK) == 0) {
        mtar_next(&tar);
        continue;
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
