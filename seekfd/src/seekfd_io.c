#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <sys/syscall.h>

#include "../include/seekfd_type.h"
#include "../include/util.h"
#include "../libtask/include/libtask.h"


struct write_reg_arg_t {
  int fd;
  unsigned long int regs[9];
};


static void *_async_write_reg(void *arg);


void seekfd_write_reg(
    int fd,
    unsigned long int sys,
    unsigned long int ip,
    unsigned long int ret,
    unsigned long int r0,
    unsigned long int r1,
    unsigned long int r2,
    unsigned long int r3,
    unsigned long int r4,
    unsigned long int r5) {
  struct write_reg_arg_t arg = {
    fd, {sys, ip, ret, r0, r1, r2, r3, r4, r5},
  };

  struct task_t *p_task = task_new(
      _async_write_reg,
      &arg,
      0);

  task_start(p_task);

  task_join(p_task);

  task_release(p_task);
}


static void *_async_write_reg(void *arg) {
  struct write_reg_arg_t *regs = (struct write_reg_arg_t *)arg;

  fprintf(stderr, "%p\n", arg);

  write(
      regs->fd,
      (const void *)regs->regs,
      sizeof(unsigned long int) * 9);

  return NULL;
}
