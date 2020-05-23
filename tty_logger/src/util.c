#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>


int eprintf(
    FILE *strm,
    const char *fn,
    const char *param) {
  if(fn == NULL && param == NULL) {
    return EOF;
  }

  return (param != NULL) 
    ? fprintf(strm, "%s: %s %s\n", fn, param, strerror(errno))
    : fprintf(strm, "%s: %s\n", fn, strerror(errno));
}
