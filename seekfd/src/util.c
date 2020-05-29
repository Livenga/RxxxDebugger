#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>


int eprintf(
    FILE       *strm,
    const char *fn,
    const char *param) {
  if(fn == NULL && param == NULL) {
    return EOF;
  }

  return (param != NULL)
    ? fprintf(stderr, "%s: %s %s(%d)\n", fn, param, strerror(errno), errno)
    : fprintf(stderr, "%s: %s(%d)\n", fn, strerror(errno), errno);
}
