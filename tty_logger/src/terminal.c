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

#include "../include/util.h"

// tty 読み込みバッファサイズ
#define BUFFER_SIZE 128


/** tty デバイスのオープン
 *
 * @param path 対象デバイスパス
 * @param p_old 対象デバイスの設定を格納するポインタ
 * @return fd
*/
int tty_open(
    const char     *path,
    struct termios *p_old) {
  int fd;


  //fd = open(path, O_RDWR | O_NOCTTY | O_NONBLOCK);
  fd = open(path, O_RDWR | O_NONBLOCK);
  if(fd <= 0) {
    eprintf(stderr, "open(2)", path);
    return EOF;
  }

  if(p_old != NULL) {
    int status = ioctl(fd, TCGETS, (void *)p_old);
    if(status != 0) {
      eprintf(stderr, "ioctl(2)", "TCGETS");
    }
  }

  return fd;
}


/** 既存の設定を元にして開いたデバイスの設定を行う.
 * @param fd デバイスの fd
 * @param old_termios 元にする struct termios
 * @return 成功時に 0.
*/
int tty_setup(
    int fd,
    struct termios old_termios) {
  struct termios term;
  memcpy((void *)&term, (const void *)&old_termios, sizeof(struct termios));


  // 入力: パリティ, break を無効. 入力文字は UTF-8
  term.c_iflag &= ~(INPCK);
  term.c_iflag |= IGNPAR | IGNBRK | IUTF8;

  // 読み込みを許可. 読み込みビット数 8
  term.c_cflag &= ~(CSIZE | CSTOPB | PARENB);
  term.c_cflag |= CS8 | CREAD;

  // 非カノニカル
  term.c_lflag &= ~(ECHO | ICANON);

  term.c_cc[VMIN] = 1;


  // TODO: 場合によっては Baudrate を可変に
  // Baudrate 57600
  cfsetispeed(&term, B57600);
  cfsetospeed(&term, B57600);


  int status = ioctl(fd, TCSETS, (const void *)&term);
  if(status != 0) {
    eprintf(stderr, "ioctl(2)", "TCSETS");
    return EOF;
  }

  return 0;
}


/** tty デバイスから受信した値を処理
 * @param path デバイスパス
 * @return 成功時 0
*/
int tty_capture(const char *path) {
  int status;
  struct termios old_termios;

  memset((void *)&old_termios, '\0', sizeof(struct termios));

  int fd = tty_open(path, &old_termios);
  if(fd == EOF) {
    return EOF;
  }

  tty_setup(fd, old_termios);


  extern uint8_t f_stopped;

  fd_set readfds;
  struct timeval timeout = {
    .tv_sec  = 1,
    .tv_usec = 0
  };

  FD_ZERO(&readfds);
  FD_SET(fd, &readfds);


  uint32_t cntr = 0;
  uint8_t buffer[BUFFER_SIZE];
  memset((void *)buffer, '\0', sizeof(buffer));

  while(! f_stopped) {
    fd_set _readfds;
    uint8_t received;
    memcpy((void *)&_readfds, (const void *)&readfds, sizeof(fd_set));

    status = select(fd + 1, &_readfds, NULL, NULL, &timeout);
    if(status < 0) {
      eprintf(stderr, "select(2)", NULL);
      break;
    }

    if(FD_ISSET(fd, &_readfds)) {
      ssize_t recv_size = read(fd, (void *)&received, 1);

      if(recv_size <= 0) {
        break;
      }

      *(buffer + cntr++) = received;
      if(cntr == BUFFER_SIZE) {
        // TODO: 書き込み対象をファイルにする
        write(STDOUT_FILENO, (const void *)buffer, cntr - 1);
        cntr = 0;
      }
    }
  }
  if(cntr > 0) {
    write(STDOUT_FILENO, (const void *)buffer, cntr - 1);
  }


  status = ioctl(fd, TCSETS, (const void *)&old_termios);
  if(status != 0) {
    eprintf(stderr, "ioctl(2)", "TCSETS");
  }

  close(fd);

  return 0;
}
