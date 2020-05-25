#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "../include/util.h"


/** src/io.c */
extern ssize_t write_buffer(
    int        fd,
    const void *buffer,
    size_t     count);


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
int tty_capture(
    const char *path,
    const char *output_directory) {
  int status;
  struct termios old_termios;

  memset((void *)&old_termios, '\0', sizeof(struct termios));

  int fd = tty_open(path, &old_termios);
  if(fd == EOF) {
    return EOF;
  }

  tty_setup(fd, old_termios);


  extern uint8_t f_stopped;
  extern uint8_t f_verbose;

  fd_set readfds;

  // 出力先ファイル名の割当
  const char *dev_name = NULL;
  const char *p_cursor;
  char *output_path = NULL;

  for(p_cursor = path; p_cursor != NULL; p_cursor = strchr(p_cursor + 1, '/')) {
    dev_name = p_cursor;
  }

  if(dev_name != NULL) {
    ++dev_name;

    // ファイル名の生成
    const time_t now = time(NULL);
    struct tm *ltime= localtime(&now);

    const size_t filename_len = strlen(dev_name) + 25;
    char *filename = (char *)calloc(
        filename_len + 1,
        sizeof(char));
    if(filename == NULL) {
      eprintf(stderr, "calloc(3)", NULL);
      return EOF;
    }

    snprintf(filename, filename_len, "%04d_%02d_%02d_%02d_%02d_%02d_%s.txt",
        (1900 + ltime->tm_year), (1 + ltime->tm_mon), ltime->tm_mday,
        ltime->tm_hour, ltime->tm_min, ltime->tm_sec,
        dev_name);


    const size_t output_path_len = strlen(output_directory) + filename_len + 1;
    output_path = (char *)calloc(
        output_path_len + 1,
        sizeof(char));

    if(output_path == NULL) {
      eprintf(stderr, "calloc(3)", NULL);

      memset((void *)filename, '\0', sizeof(char) * filename_len);
      free((void *)filename);
      filename = NULL;

      return EOF;
    }

    strncat(output_path, output_directory, output_path_len);
    strncat(output_path, "/", output_path_len);
    strncat(output_path, filename, output_path_len);

    memset((void *)filename, '\0', sizeof(char) * filename_len);
    free((void *)filename);
    filename = NULL;
  }

  if(output_path == NULL) {
    fprintf(stderr, "出力先ファイル名の取得に失敗.");
    return EOF;
  }

  if(f_verbose) {
    fprintf(stderr, "-- Output file path: %s\n", output_path);
  }

  int output_fd = open(output_path, O_CREAT | O_WRONLY | O_APPEND, 0644);

  // ファイルディスクリプタ生成後, 割り当てた出力パスを解放
  memset((void *)output_path, '\0', sizeof(char) * strlen(output_path));
  free((void *)output_path);
  output_path = NULL;

  if(output_fd < 0) {
    eprintf(stderr, "open(2)", output_path);

    return EOF;
  }

  FD_ZERO(&readfds);
  FD_SET(fd, &readfds);


  while(! f_stopped) {
    uint8_t received;
    struct timeval timeout = {
      .tv_sec  = 15,
      .tv_usec = 0
    };

    fd_set _readfds;
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

      if(f_verbose) {
        write(STDOUT_FILENO, (const void *)&received, 1);
      }
      write(output_fd, (const void *)&received, 1);
    }
  }


  status = ioctl(fd, TCSETS, (const void *)&old_termios);
  if(status != 0) {
    eprintf(stderr, "ioctl(2)", "TCSETS");
  }

  close(fd);
  close(output_fd);

  return 0;
}
