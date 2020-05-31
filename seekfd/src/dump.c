#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "../include/util.h"


/** 指定プロセスID(pid) が開放しているファイルディスクリプタについて fd に書き込みを行う.
 * @param fd
 * @param pid
 * @returne 成否 [0 : EOF(-1)]
 */
int dump_write_file_descriptors(
    int   fd,
    pid_t pid) {
  extern uint8_t f_verbose;
  char path[1024];

  memset((void *)path, '\0', sizeof(path));
  snprintf(path, 1024, "/proc/%d/fd", pid);


  //
  uint16_t fd_count = 0;
  off_t ptr_fd_count = lseek(fd, 0L, SEEK_CUR);
  write(fd, (const void *)&fd_count, sizeof(uint16_t));

  DIR *odir = opendir(path);
  if(odir == NULL) {
    // opendir(3) 失敗時には,
    // 記録する, 指定プロセスが開放しているファイルディスクリプタの個数を 0 のままにする.
    eprintf(stderr, "opendir(3)", path);

    return EOF;
  }

  struct dirent *rdir = NULL;

  while((rdir = readdir(odir)) != NULL) {
    char *endptr = NULL;
    if(*rdir->d_name != '.') {
      int32_t n_fd = (int32_t)strtol(rdir->d_name, &endptr, 10);
      if(rdir->d_name != endptr && n_fd >= 0) {
        struct stat statbuf;
        memset((void *)&statbuf, '\0', sizeof(struct stat));

        snprintf(path, 1024, "/proc/%d/fd/%d", pid, n_fd);

        int status = lstat(path, &statbuf);
        if(status == 0) {
          if((statbuf.st_mode & S_IFMT) == S_IFLNK) {
            char resolved_path[1024];
            memset((void *)resolved_path, '\0', sizeof(resolved_path));

            // リンク先のパスが socket, pipe などのように存在しない場合,
            // エラーを返却するため, 格納されて resolved_path の長さでパスの上書きを行う.
            realpath(path, resolved_path);
            if(strlen(resolved_path) > 0) {
              // パスの上書き
              strncpy(path, resolved_path, 1024);
            }
          }
        } else {
          if(f_verbose) {
            eprintf(stderr, "stat(2)", path);
          }
        }

        int16_t path_len = (int16_t)strlen(path);

        // fd number
        write(fd, (const void *)&n_fd,     sizeof(int32_t));
        write(fd, (const void *)&path_len, sizeof(int16_t));
        write(fd, (const void *)path,      path_len);

        ++fd_count;
      }
    }
  }
  closedir(odir);


  off_t ptr_current = lseek(fd, 0L, SEEK_CUR);
  // ファイルディスクリプタの個数を書き込む位置に移動し, カウントした個数を書き込む
  lseek(fd, ptr_fd_count, SEEK_SET);
  write(fd, (const void *)&fd_count, sizeof(uint16_t));

  // 終端へ移動
  lseek(fd, ptr_current, SEEK_SET);

  return 0;
}
