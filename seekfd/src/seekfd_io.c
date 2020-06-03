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
#include <sys/uio.h>

#include "../include/seekfd_type.h"
#include "../include/util.h"
#include "../libtask/include/libtask.h"


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
  if(fd < 0) {
    return;
  }

  const struct iovec iov[9] = {
    {&sys, sizeof(unsigned long int)},
    {&ip,  sizeof(unsigned long int)},
    {&ret, sizeof(unsigned long int)},
    {&r0,  sizeof(unsigned long int)},
    {&r1,  sizeof(unsigned long int)},
    {&r2,  sizeof(unsigned long int)},
    {&r3,  sizeof(unsigned long int)},
    {&r4,  sizeof(unsigned long int)},
    {&r5,  sizeof(unsigned long int)},
  };

  writev(fd, iov, 9);
}
