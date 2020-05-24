#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <termios.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


/** write(2) 指定サイズすべての書き込み
 */
ssize_t write_buffer(
    int        fd,
    const void *buffer,
    size_t     count) {
  ssize_t write_size, cursor;

  cursor = 0;
  while((write_size = write(fd, (const void *)(buffer + cursor), count - cursor)) > 0) {
    cursor += write_size;
  }

  return cursor;
}
