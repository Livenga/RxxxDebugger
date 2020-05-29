#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <getopt.h>
#include <time.h>
#include <pthread.h>
#include <limits.h>
#include <regex.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ptrace.h>
#include <sys/types.h>

#include "../include/seekfd_type.h"
#include "../include/util.h"


#define SEEKFD_VERSION (1)


#define GET_OPTARG(argv, optarg, optind) \
  ((optarg != NULL) ? optarg : argv[optind])


/** プロセス実行名からプロセス ID の取得
 * @param process_name 対象プロセス名
 * @return 対象プロセス名に該当するプロセスが見つかれば, そのプロセスID を返す. 失敗時は 0 を返す.
 */
static pid_t get_pid_by_name(const char *process_name);

/** ファイルディスクリプタの検索
 * @param pid 監視対象プロセスID
 * @param target_path 開かれている可能性があるファイルパス
 * @return /proc/[pid]/fd ディレクトリ中に, target_path が参照先となるシンボリックリングが存在した場合それに対応するファイルディスクリプタ.
 *         失敗時 -1
 */
static int32_t find_file_descriptor(
    const pid_t pid,
    const char *target_path);

/** 日時をファイル名としたパスの作成
 * @param path 格納先ポインタ. strncat を用いるため, 格納されている文字列の次にパスを追加する
 * @param size 格納先ポインタのサイズ
 * @return path
 */
static char *generate_output_path(
    char *path,
    size_t size);

/** ヘルプ表示
 * @param app 実行ファイル名(argv[0])
 */
static void print_help(const char *app);

extern void *thread_seekfd(void *p_arg);

uint8_t f_verbose     = 0;
uint8_t f_thread_exit = 0;
uint8_t f_output      = 0;


/**
 */
int main(
    int argc,
    char *argv[]) {
  const struct option opts[] = {
    {"pid",              required_argument, 0, 'p'},
    {"process-name",     required_argument, 0, 'n'},
    {"file-descriptor",  required_argument, 0, 'f'},
    {"output-path",      optional_argument, 0, 'o'},
    {"output-directory", required_argument, 0, 'd'},
    {"verbose",          no_argument,       0, 'v'},
    {"help",             no_argument,       0, 'h'},
    {0, 0, 0, 0},
  };


  pid_t   target_pid = 0;
  int32_t target_fd  = -1;

  char fd_path[1024];
  char output_path[1024];

  memset((void *)fd_path, '\0', sizeof(fd_path));
  memset((void *)output_path, '\0', sizeof(output_path));

  int opt;
  char *_optarg, *endptr = NULL;
  while((opt = getopt_long(argc, argv, "p:n:f:od:vh", opts, NULL)) > 0) {
    switch(opt) {
      case 'p':
        if(target_pid == 0) {
          target_pid = strtoul(GET_OPTARG(argv, optarg, optind), NULL, 10);
        }
        break;

      case 'n':
        if(target_pid == 0) {
          target_pid = get_pid_by_name(GET_OPTARG(argv, optarg, optind));
        }
        break;

      case 'f':
        _optarg   = GET_OPTARG(argv, optarg, optind);
        target_fd = (int32_t)strtol(_optarg, &endptr, 10);
        if(target_fd < 0 || _optarg == endptr) {
          // 指定された値が数値以外の可能背がある.
          target_fd = -1;
          strncpy((char *)fd_path, (const char *)_optarg, 1024);
        }
        break;


      case 'o':
        if(! f_output) {
          _optarg = GET_OPTARG(argv, optarg, optind);
          if(_optarg != NULL && *(_optarg + 0) != '-') {
            strncpy(output_path, _optarg, 1024);
          } else {
            char *p_ret = generate_output_path(output_path, 1024);
            if(p_ret == NULL) {
              return 133;
            }
            strncpy((char *)output_path, (const char *)(output_path + 1), 1023);
          }

          f_output = 1;
        }
        break;

      case 'd':
        if(! f_output) {
          _optarg = GET_OPTARG(argv, optarg, optind);
          strncpy(output_path, _optarg, 1024);
          if(generate_output_path(output_path, 1024) == NULL) {
            return 131;
          }

          f_output = 1;
        }
        break;

      case 'v':
        f_verbose = 1;
        break;
      case 'h':
        print_help(argv[0]);
        return 0;
    }
  }

#if defined(__REQUIRED_ROOT__) && __REQUIRED_ROOT__ == 1
  if(getuid() != 0) {
    fprintf(stderr, "- \033[1;31mroot\033[0m 権限が必要です.\n");
    return 127;
  }
#endif


  if(target_pid == 0) {
    fprintf(stderr, "- 検索対象のプロセス ID か未指定か探索に失敗しました.\n");
    return 128;
  }

  if(target_fd == -1 && strlen(fd_path) > 0) {
    target_fd = find_file_descriptor(target_pid, fd_path);

    if(target_fd == -1) {
      fprintf(
          stderr,
          "%s: 監視対象のファイルディスクリプタが見つからなかったため,"
          "すべてのファイルディスクリプタを監視対象にします.\n",
          fd_path);
    }
  }

  if(f_verbose) {
    fprintf(stderr, "対象プロセスID: %u\n", target_pid);

    if(target_fd > 0) {
      fprintf(stderr, "対象ファイルディスクリプタ: %d\n", target_fd);
    } else {
      fprintf(stderr, "read/wrtie が呼び出された場合, すべてがデータを監視対象.\n");
    }

    if(f_output) {
      fprintf(stderr, "出力ファイルパス: %s\n", output_path);
    }
  }

  // 出力するファイルディスクリプタの作成
  int output_fd = -1;
  if(f_output) {
    output_fd = open(output_path, O_WRONLY | O_CREAT | O_APPEND, 0644);

    if(output_fd < 0) {
      eprintf(stderr, "open(2)", output_path);
      return 132;
    }

    uint8_t chr = 's';
    uint16_t version = SEEKFD_VERSION;

    write(output_fd, (const void *)&chr, 1);
    chr = 0xfd;
    write(output_fd, (const void *)&chr, 1);
    write(output_fd, (const void *)&version, sizeof(uint16_t));
  }


  int status;

  pthread_t thr_seek;
  pthread_mutex_t mtx_seek = PTHREAD_MUTEX_INITIALIZER;
  pthread_mutexattr_t mtx_attr_seek;

  status = pthread_mutex_init(&mtx_seek, NULL);
  if(status != 0) {
    eprintf(stderr, "pthread_mutex_init(3)", NULL);
  }

  status = pthread_mutex_init(&mtx_seek, &mtx_attr_seek);
  if(status != 0) {
    eprintf(stderr, "pthread_mutex_init(3)", NULL);
  }

  struct thread_seekfd_arg_t thr_args;
  memset((void *)&thr_args, 0, sizeof(struct thread_seekfd_arg_t));

  thr_args.pid       = target_pid;
  thr_args.mutex     = &mtx_seek;
  thr_args.target_fd = target_fd;
  thr_args.output_fd = output_fd;


  status = pthread_create(
      /* thread        = */&thr_seek,
      /* attr          = */NULL,
      /* start_routine = */thread_seekfd,
      /* arg           = */(void *)&thr_args);

  if(status != 0) {
    eprintf(stderr, "pthread_create(3)", NULL);
    return 129;
  }

  getchar();

  pthread_mutex_lock(&mtx_seek);
  f_thread_exit = 1;
  pthread_mutex_unlock(&mtx_seek);

  status = pthread_join(thr_seek, NULL);
  if(status != 0) {
    eprintf(stderr, "pthread_join(3)", NULL);
    return 130;
  }

  if(thr_args.output_fd > 0) {
    close(thr_args.target_fd);
  }
  pthread_mutex_destroy(&mtx_seek);

  return 0;
}


//
static pid_t get_pid_by_name(const char *process_name) {
  pid_t pid = 0;

  DIR *odir = opendir("/proc");
  if(odir == NULL) {
    return 0;
  }

  struct dirent *rdir = NULL;
  while((rdir = readdir(odir)) != NULL) {
    char *endptr;
    uint32_t _pid = strtoul(rdir->d_name, &endptr, 10);

    if(endptr != rdir->d_name && _pid > 0) {
      char path[1024];
      memset((void *)path, '\0', sizeof(path));
      snprintf(path, 1024, "/proc/%u/cmdline", _pid);


      int  fd = open(path, O_RDONLY | O_ASYNC);
      if(fd == -1) {
        eprintf(stderr, "open(2)", path);
        continue;
      }


      char buf[1024];
      memset((void *)buf, '\0', sizeof(buf));

      ssize_t read_size = read(fd, (void *)buf, sizeof(char) * 1023);
      close(fd);

      if(read_size > 0) {
        char *p_chr = strchr(buf, ' ');
        if(p_chr != NULL) {
          *p_chr = '\0';
        }

        char *p_cmd = NULL;
        for(p_chr = (char *)buf; p_chr != NULL; p_chr = strchr(p_chr + 1, '/')) {
          p_cmd = p_chr;
        }

        if(*p_cmd == '/') {
          ++p_cmd;
        }

        if(strcmp((const char *)p_cmd, (const char *)process_name) == 0) {
          pid = _pid;
          break;
        }
      }
    }
  }

  closedir(odir);

  return pid;
}


//
static int32_t find_file_descriptor(
    const pid_t pid,
    const char  *target_path) {
  int target_fd = -1;
  char path[1024];
  memset((void *)path, '\0', sizeof(path));
  snprintf(path, 1024, "/proc/%d/fd", pid);


  DIR           *odir = opendir(path);
  struct dirent *rdir = NULL;
  if(odir == NULL) {
    eprintf(stderr, "opendir(3)", path);
    return -1;
  }

  while((rdir = readdir(odir)) != NULL) {
    char resolved_path[1024];

    memset((void *)path, '\0', sizeof(path));
    memset((void *)resolved_path, '\0', sizeof(resolved_path));

    snprintf(path, 1024, "/proc/%d/fd/%s", pid, rdir->d_name);
    if(realpath(path, resolved_path) != NULL && strcmp(target_path, resolved_path) == 0) {
      char *endptr = NULL;

      target_fd = (int32_t)strtol(rdir->d_name, &endptr, 10);
      target_fd = (target_fd > 0 && rdir->d_name != endptr)
        ? target_fd
        : -1;

      // ここまで到達したら, strtol に失敗はないとはず
      if(target_fd == -1) {
        eprintf(stderr, "strtol(3)", rdir->d_name);
      }
    }
  }

  closedir(odir);

  return target_fd;
}


//
static char *generate_output_path(char *path, size_t size) {
  const time_t now = time(NULL);
  struct tm *p_tm = localtime(&now);
  if(p_tm == NULL) {
    return NULL;
  }

  int i;
  for(i = strlen(path) - 1; i >= 0 && *(path + i) == '/'; --i) {
    *(path + i) = '\0';
  }

  char filename[32];
  memset((void *)filename, '\0', sizeof(filename));
  snprintf(filename, 32,
      "%04d_%02d_%02d_%02d_%02d_%02d.sfdump",
      (1900 + p_tm->tm_year), (1 + p_tm->tm_mon), p_tm->tm_mday,
      p_tm->tm_hour, p_tm->tm_min, p_tm->tm_sec);

  if((strlen(path) + strlen(filename) + 1) > size) {
    fprintf(stderr, "%d: ファイル名の長さが超過しています.\n", size);
    return NULL;
  }

  strncat(path, "/", size);
  strncat(path, filename, size);

  return path;
}


//
static void print_help(const char *app) {
  fprintf(stdout, "Usage: %s [OPTION]\n\n", app);
  fprintf(stdout, "[Options]\n");
  fprintf(stdout, "  -p, --pid:             process ID\n");
  fprintf(stdout, "  -n, --process-name:    process name\n");
  fprintf(stdout, "  -f, --file-descriptor: File descriptor\n");
  fprintf(stdout, "\n");
  fprintf(stdout, "  -o, --output-path:      出力先パス\n");
  fprintf(stdout, "  -d, --output-directory: 出力先ディレクトリ\n");
  fprintf(stdout, "                          出力パスは先に指定されたオプションが優先される.\n");
  fprintf(stdout, "\n");
}
