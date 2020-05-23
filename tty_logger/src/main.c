#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <termios.h>
#include <signal.h>
#include <fcntl.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "../include/util.h"




extern int signal_init(void);
extern int tty_capture(const char *path);


uint8_t f_verbose = 0;
uint8_t f_stopped = 0;


#define GET_OPTARG(argv, optarg, optind) \
  ((optarg != NULL) ? optarg : argv[optind])

int main(
    int argc,
    char *argv[]) {
  const struct option opts[] = {
    {"device",  required_argument, 0, 'D'},
    {"verbose", no_argument,       0, 'v'},
    {0, 0, 0, 0},
  };


  char device_path[1024];
  memset((void *)device_path, '\0', sizeof(device_path));

  int opt, longopt;
  while((opt = getopt_long(
          /* argc      = */argc,
          /* argv      = */argv,
          /* optstring = */"D:v",
          /* longopts  = */opts,
          /* longindex = */&longopt)) > 0) {
    char *_optarg = NULL;

    switch(opt) {
      case 'D':
        _optarg = GET_OPTARG(argv, optarg, optind);
        strncpy((char *)device_path, (const char *)_optarg, 1023);
        break;

      case 'v':
        f_verbose = 1;
        break;
    }
  }

  if(strlen(device_path) == 0) {
    return EOF;
  }

  if(signal_init() != 0) {
    eprintf(stderr, "sigaction(2)", NULL);

    return EOF;
  }


  if(f_verbose) {
    fprintf(stderr, "-- Target device path: %s\n", device_path);
  }

  tty_capture(device_path);

  return 0;
}
