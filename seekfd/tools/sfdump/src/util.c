#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>


/**
 */
int eprintf(
    FILE *strm,
    const char *fn,
    const char *param) {
  if(fn == NULL && param == NULL) {
    return EOF;
  }

  return (param != NULL)
    ? fprintf(strm, "%s: %s %s(%d)\n", fn, param, strerror(errno), errno)
    : fprintf(strm, "%s: %s(%d)\n", fn, strerror(errno), errno);
}
