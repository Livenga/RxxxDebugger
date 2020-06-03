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
#include <sys/user.h>
#include <sys/syscall.h>
#include <sys/uio.h>

#include "../../include/util.h"


size_t seekfd_dump_write(
    int               fd,
    pid_t             pid,
    unsigned long int buf_addr,
    unsigned long int count) {
  int32_t buf_size = count;
  buf_size += ((buf_size % sizeof(long)) > 0) ? sizeof(long) : 0;

#if defined(__DEBUG__)
  fprintf(stderr, "- write(0x%lx, %lu)\n", buf_addr, (unsigned long)count);
#endif

  void *buf = malloc(buf_size);
  if(buf == NULL) {
    eprintf(stderr, "malloc(3)", NULL);
  }

  memset((void *)buf, '\0', buf_size);

  int i;
  for(i = 0; i < count; i += sizeof(long)) {
    long data = ptrace(
        /* request = */PTRACE_PEEKDATA,
        /* pid     = */pid,
        /* addr    = */(void *)buf_addr + i,
        /* data    = */NULL);
    memcpy(buf + i, (const void *)&data, sizeof(long));
  }

  //
  write(fd, (const void *)&buf_size, sizeof(int32_t));
  write(fd, (const void *)buf, buf_size);

  free(buf);
  return 0;
}
