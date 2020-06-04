#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/syscall.h>

#include "../../include/util.h"


#define ADJUST_SIZE(size) \
  (size % sizeof(long)) > 0 \
  ? ((size / sizeof(long)) * sizeof(long)) + sizeof(long) \
  : size


/**
 */
static void *_seekfd_peekdatas(
    pid_t   pid,
    int32_t addr,
    int32_t *size);

/**
 */
static size_t _seekfd_write(
    int        fd,
    const void *buf,
    size_t     size);


/**
 */
size_t seekfd_write_peekdata(
    int               fd,
    pid_t             pid,
    unsigned long int addr,
    unsigned long int size) {
  int32_t _size = size;

  void *buf = _seekfd_peekdatas(pid, addr, (int32_t *)&_size);
  if(buf == NULL) {
    _size = 0;

    write(fd, (const void *)&_size, sizeof(int32_t));
    return EOF;
  }
  write(fd, (const void *)&_size, sizeof(int32_t));
  _seekfd_write(fd, (const void *)buf, _size);

  memset(buf, 0, _size);
  free(buf);
  buf = NULL;

  return size;
}


static void *_seekfd_peekdatas(
    pid_t   pid,
    int32_t addr,
    int32_t *size) {
  void *buf = NULL;

  if(size == NULL) {
    fprintf(stderr, "- size 引数に NULL が入力されました.\n");
    return NULL;
  }


  *size = ADJUST_SIZE((*size));
  buf   = malloc(*size);
  if(buf == NULL) {
    eprintf(stderr, "malloc(3)", NULL);
    return NULL;
  }
  memset(buf, 0, *size);


  int i;
  for(i = 0; i < *size; i += sizeof(long)) {
    long data = ptrace(
        /* request = */PTRACE_PEEKDATA,
        /* pid     = */pid,
        /* addr    = */(addr + i),
        /* data    = */NULL);

    memcpy(
        /* dest = */buf + i,
        /* src  = */(const void *)&data,
        /* n    = */sizeof(long));
  }

  return buf;
}


static size_t _seekfd_write(
    int        fd,
    const void *buf,
    size_t     size) {
  ssize_t write_size, cur = 0;

  while((write_size = write(fd, buf + cur, size - cur)) > 0) {
    cur += write_size;
  }

  return cur;
}
