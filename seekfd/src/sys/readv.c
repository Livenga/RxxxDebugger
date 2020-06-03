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


size_t seekfd_dump_readv(
    int               fd,
    pid_t             pid,
    unsigned long int iov_addr,
    unsigned long int iovcnt) {
  int i;

  // struct iovec *
  size_t _iov_size = sizeof(struct iovec) * iovcnt;
  _iov_size += (_iov_size % sizeof(long)) > 0 ? sizeof(long) : 0;

  void *_p_iov = malloc(_iov_size);
  if(_p_iov == NULL) {
    eprintf(stderr, "malloc(3)", NULL);
    return EOF;
  }

#if defined(__DEBUG__)
  fprintf(stderr, "- readv(0x%lx, %lu)\n", iov_addr, iovcnt);
#endif

  for(i = 0; i < iovcnt; i += sizeof(long)) {
    long data = ptrace(PTRACE_PEEKDATA, pid, iov_addr + i, NULL);
    memcpy(_p_iov + i, &data, sizeof(long));
  }

  struct iovec *p_iov = _p_iov;
  for(i = 0; i < iovcnt; ++i) {
    struct iovec *iov = (p_iov + i);

    fprintf(stderr, "iov[%d]: %p, %ld\n",
        i, iov->iov_base, (long)iov->iov_len);
  }


  // TODO:
  int32_t zero = 0;
  write(fd, (const void *)&zero, sizeof(int32_t));

  free((void *)_p_iov);

  return 0;
}
