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

#include "../../include/util.h"


size_t seekfd_dump_read(
    int               fd,
    pid_t             pid,
    unsigned long int addr,
    unsigned long int size) {
  int32_t allocate_size = size
    + (((size % sizeof(long)) > 0) ? sizeof(long) : 0);

  void *ptr = malloc(allocate_size);
  if(ptr == NULL) {
    eprintf(stderr, "malloc(3)", NULL);
    return -1;
  }
  memset((void *)ptr, 0, allocate_size);

#if defined(__DEBUG__)
  fprintf(stderr, "- read(0x%lx, %lu)\n", addr, (unsigned long)size);
#endif

  int i;
  for(i = 0; i < size; i += sizeof(long)) {
    long data = ptrace(PTRACE_PEEKDATA, pid, addr + i, NULL);

    if(data >= 0) {
      memcpy(ptr + i, (const void *)&data, sizeof(long));
    }
  }


  //
  write(fd, (const void *)&allocate_size, sizeof(int32_t));
  //
  write(fd, (const void *)ptr, allocate_size);

  free((void *)ptr);
  ptr = NULL;

  return size;
}
