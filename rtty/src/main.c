#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>


/** src/tty.c */
extern struct termios *tty_get_info(const char *path);
extern void tty_print_info(const struct termios *p_term);

//
static int eprintf(
    FILE *strm,
    const char *fn,
    const char *param);

int main(
    int  argc,
    char *argv[]) {
  if(argc == 1) {
    fprintf(stderr, "Usage: %s [DEVICE]\n", *argv);
    return EOF;
  }

  struct termios *p_termios = tty_get_info(argv[1]);
  if(p_termios == NULL) {
    eprintf(stderr, "tty_get_info(3)", argv[1]);
    return EOF;
  }

  fprintf(stdout, "# Device %s\n", argv[1]);
  tty_print_info(p_termios);

  memset((void *)p_termios, '\0', sizeof(struct termios));
  free((void *)p_termios);
  p_termios = NULL;

  return 0;
}


//
static int eprintf(
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
