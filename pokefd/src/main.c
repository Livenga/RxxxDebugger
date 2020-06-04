#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <dirent.h>
#include <pthread.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "../include/pokefd_type.h"
#include "../include/util.h"


#define GET_OPTARG(argv, optarg, optind) \
  ((optarg != NULL) ? optarg : *(argv + optind))

/** src/ptrace.c */
extern void *pthread_pokefd(void *p_args);

static uint8_t exists_process(pid_t pid);
static void display_opened_fd(const pid_t pid);
static int32_t find_file_descriptor(pid_t pid, const char *path);

uint8_t g_is_continue = 0;
uint8_t g_f_verbose = 0;


int main(
    int argc,
    char *argv[]) {
  const struct option opts[] = {
    {"process-id", required_argument, 0, 'p'},
    {"target-fd",  required_argument, 0, 'f'},
    {"verbose",    no_argument,       0, 'v'},
    {0, 0, 0, 0},
  };


  pid_t pid = -1;
  int opt;
  int32_t target_fd = -1;

  while((opt = getopt_long(
          /* argc      = */argc,
          /* argv      = */argv,
          /* optstring = */"p:f:v",
          /* longopts  = */opts,
          /* longindex = */NULL)) > 0) {
    char *_optarg, *_endptr = NULL;

    switch(opt) {
      case 'p':
        _optarg = GET_OPTARG(argv, optarg, optind);
        pid     = (pid_t)strtol(_optarg, &_endptr, 10);
        if(pid < 0 || _endptr == _optarg) {
          pid = -1;
          fprintf(stderr, "%s: 不正なプロセスIDが指定されました.\n", _optarg);
        }
        break;

      case 'f':
        _optarg   = GET_OPTARG(argv, optarg, optind);
        target_fd = (int32_t)strtol(_optarg, &_endptr, 10);
        if(_optarg == _endptr || target_fd < 0) {
          if(*_optarg == '/') {
            // ファイルディスクリプタの検索
            target_fd = find_file_descriptor(pid, _optarg);
          }
        }
        break;

      case 'v':
        g_f_verbose = 1;
        break;
    }
  }

  if(getuid() != 0) {
    fprintf(stderr, "- \033[1;31mroot\033[0m で実行してください.\n");
    return EOF;
  }

  if(! exists_process(pid)) {
    fprintf(stderr, "- %d: 指定されてプロセスが存在しません.\n", pid);
    return EOF;
  }

#if 1
  if(target_fd < 0) {
    char *_endptr = NULL;
    char _input_buf[256];

    memset((void *)_input_buf, '\0', sizeof(_input_buf));

    // ファイルディスクリプタ一覧を表示と選択
    display_opened_fd(pid);
    fprintf(stderr, "- 監視対象のファイルディスクリプタを指定してください.\n");
    fprintf(stderr, "  File Descriptor: ");

    fgets(_input_buf, 256, stdin);
    //fread((void *)_input_buf, sizeof(char), 256, stdin);

    target_fd = (int32_t)strtol(_input_buf, &_endptr, 10);
    if(_input_buf == _endptr || target_fd < 0) {
      _endptr = strchr(_input_buf, '\n');
      if(_endptr != NULL) {
        *_endptr = '\0';
      }

      fprintf(stderr, "%s: 不正なファイルディスクリプタが指定されました.\n", _input_buf);
      return EOF;
    }
  }
#endif

if(g_f_verbose) {
    fprintf(stderr, "- プロセスID: %d\n", (int)pid);
    fprintf(stderr, "- 監視対象fd: %d\n", (int)target_fd);
  }


  int status;
  pthread_t thread;

  pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
  status = pthread_mutex_init(&mutex, NULL);

  if(status != 0) {
    eprintf(stderr, "pthread_mutex_init(3)", NULL);
    return EOF;
  }

    struct pokefd_thread_args_t thr_args;
  memset((void *)&thr_args, 0, sizeof(struct pokefd_thread_args_t));

  thr_args.mutex      = &mutex;
  thr_args.target_pid = pid;
  thr_args.target_fd  = target_fd;


  g_is_continue = 1;
  status        = pthread_create(
      /* thread        = */&thread,
      /* attr          = */NULL,
      /* start_routine = */pthread_pokefd,
      /* arg           = */(void *)&thr_args);
  if(status != 0) {
    eprintf(stderr, "pthread_create(3)", NULL);
    return EOF;
  }

  getchar();

  pthread_mutex_lock(&mutex);
  g_is_continue = 0;
  pthread_mutex_unlock(&mutex);

  pthread_join(thread, NULL);
  pthread_mutex_destroy(&mutex);


  return 0;
}


//
static uint8_t exists_process(pid_t pid) {
  char path[256];
  memset((void *)path, '\0', sizeof(path));

  snprintf(path, 256, "/proc/%d", (int)pid);

  return (access(path, R_OK) == 0) ? 1 : 0;
}


//
static void display_opened_fd(const pid_t pid) {
  char path[256];
  memset((void *)path, '\0', sizeof(path));
  snprintf(path, 256, "/proc/%d/fd", (int32_t)pid);

  DIR *odir = opendir(path);
  if(odir == NULL) {
    eprintf(stderr, "opendir(3)", path);
    return;
  }

  char real[1024];
  struct dirent *rdir = NULL;

  while((rdir = readdir(odir)) != NULL) {
    if(*rdir->d_name != '.') {
      size_t _len   = strlen(path) + strlen(rdir->d_name) + 1;
      char   *_path = (char *)calloc(_len + 1, sizeof(char));

      strncat(_path, path, _len);
      strncat(_path, "/", _len);
      strncat(_path, rdir->d_name, _len);

      struct stat statbuf;
      memset((void *)&statbuf, 0, sizeof(struct stat));

      int status = lstat(_path, &statbuf);
      if(status != 0) {
        eprintf(stderr, "stat(2)", _path);
      }

      memset((void *)real, '\0', sizeof(real));
      // XXX: errno を用いて状態の確認を摺る必要がある.
      realpath(_path, real);
      fprintf(stderr, "%4s: \033[36;1m%s\033[0m -> %s\n",
          rdir->d_name,
          _path,
          real);

      memset((void *)_path, '\0', sizeof(char) * _len);
      free((void *)_path);
      _path = NULL;
    }
  }
}


//
static int32_t find_file_descriptor(pid_t pid, const char *path) {
  int32_t nfd = -1;
  if(pid < 0) {
    return EOF;
  }

  char proc_fd[256];
  snprintf(proc_fd, 256, "/proc/%d/fd", pid);

  DIR *odir = opendir(proc_fd);
  if(odir == NULL) {
    eprintf(stderr, "opendir(3)", path);
    return -1;
  }

  struct dirent *rdir = NULL;

  char fd_path[256];
  char real[1024];

  while((rdir = readdir(odir)) != NULL) {
    if(*rdir->d_name != '.') {
      memset((void *)fd_path, '\0', sizeof(fd_path));
      memset((void *)real, '\0', sizeof(real));

      snprintf(fd_path, 256, "/proc/%d/fd/", pid);
      strncat(fd_path, rdir->d_name, 256);

      realpath(fd_path, real);

      if(strcmp(path, real) == 0) {
        nfd = (int32_t)strtol(rdir->d_name, NULL, 10);
        break;
      }
    }
  }
  closedir(odir);

  return nfd;
}
