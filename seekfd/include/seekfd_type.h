#ifndef _SEEKFD_TYPE_H
#define _SEEKFD_TYPE_H

#include <stdio.h>
#include <stdint.h>
#include <pthread.h>


struct thread_seekfd_arg_t {
  pid_t pid;
  pthread_mutex_t *mutex;

  int32_t target_fd;
  int output_fd;
};

#endif
