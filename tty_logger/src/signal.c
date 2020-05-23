#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <unistd.h>

#include "../include/util.h"


// SIGINT Handler
static void _sigint_handler(
    int       sig,
    siginfo_t *info,
    void      *ctx) {
  extern int f_stopped;

  if(! f_stopped) {
    f_stopped = 1;
  }
}


/** シグナルの初期化
 */
int signal_init(void) {
  struct sigaction sa_sigint;
  memset((void *)&sa_sigint, '\0', sizeof(struct sigaction));


  sigemptyset(&sa_sigint.sa_mask);
  sigaddset(&sa_sigint.sa_mask, SIGINT);

  sa_sigint.sa_sigaction = _sigint_handler;
  sa_sigint.sa_flags     = SA_SIGINFO;

  return  sigaction(
      /* signum = */SIGINT,
      /* act    = */(const struct sigaction *)&sa_sigint,
      /* oldact = */NULL);
}
