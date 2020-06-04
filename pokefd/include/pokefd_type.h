#ifndef _POKEFD_TYPE_H
#define _POKEFD_TYPE_H

#include <stdint.h>
#include <unistd.h>
#include <pthread.h>


struct pokefd_thread_args_t {
  pthread_t thread;
  pthread_mutex_t *mutex;

  pid_t     target_pid;
  int       target_fd;
};

#endif
