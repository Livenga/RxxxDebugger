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


size_t seekfd_dump_writev(
    int               fd,
    pid_t             pid,
    unsigned long int iov_addr,
    unsigned long int iovcnt) {
#if defined(__DEBUG__)
  fprintf(stderr, "- writev(0x%lx, %lu)\n", iov_addr, iovcnt);
#endif

  size_t _alc_size = sizeof(struct iovec) * iovcnt;
  _alc_size += ((_alc_size % sizeof(long)) > 0) ? sizeof(long) : 0;

  void *_alc = malloc(_alc_size);

  int i, j;
  for(i = 0; i < _alc_size; i += sizeof(long)) {
    long data = ptrace(PTRACE_PEEKDATA, pid, iov_addr + i, NULL);
    memcpy(_alc + i, (const void *)&data, sizeof(long));
  }

  int32_t iov_size = 0;
  for(i = 0; i < iovcnt; ++i) {
    iov_size += ((struct iovec *)_alc + i)->iov_len;
  }
  write(fd, (const void *)&iov_size, sizeof(int32_t));


  for(i = 0; i < iovcnt; ++i) {
    struct iovec *iov = ((struct iovec *)_alc + i);

    int32_t iov_alc_size = iov->iov_len;
    iov_alc_size += (iov_alc_size % sizeof(long)) > 0 ? sizeof(long) : 0;

    void *iov_data = malloc(iov_alc_size);
    memset((void *)iov_data, 0, iov_alc_size);

    for(j = 0; j < iov->iov_len; j += sizeof(long)) {
      long data = ptrace(PTRACE_PEEKDATA, pid, iov->iov_base + j, NULL);
      memcpy(iov_data + j, (const void *)&data, sizeof(long));
    }
    write(fd, (const void *)iov_data, iov->iov_len);

    free(iov_data);
  }

  free(_alc);
  _alc = NULL;


  return 0;
}
