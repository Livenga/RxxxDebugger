#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <getopt.h>
#include <errno.h>
#include <dirent.h>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>


#define GET_OPTARG(argv, optarg, optind) \
  (optarg != NULL) ? optarg : argv[optind]


//
static void print_fds(pid_t pid);
//
static int eprintf(FILE *strm, const char *fn, const char *param);
//
static void print_help(const char *app);


uint8_t f_verbose = 0;


int main(
    int argc,
    char *argv[]) {
  const struct option opts[] = {
    {"pid",     required_argument, 0, 'p'},
    {"verbose", no_argument,       0, 'v'},
    {"help",    no_argument,       0, 'h'},
    {0, 0, 0, 0},
  };

  int opt;
  while((opt = getopt_long(argc, argv, "p:vh", opts, NULL)) > 0) {
    int32_t pid = 0;
    char *_optarg = NULL,
         *endptr = NULL;

    switch(opt) {
      case 'p':
        _optarg = GET_OPTARG(argv, optarg, optind);
        pid = (int32_t)strtol(_optarg, &endptr, 10);

        if(_optarg < endptr) {
          print_fds(pid);
        }
        return 0;

      case 'v':
        f_verbose = 1;
        break;

      case 'h':
        print_help(argv[0]);
        return 0;
    }
  }

  DIR *odir = NULL;
  if((odir = opendir("/proc")) == NULL) {
    return EOF;
  }

  struct dirent *rdir = NULL;
  while((rdir = readdir(odir)) != NULL) {
    if(*(rdir->d_name + 0) == '.') {
      continue;
    }

    char *endptr = NULL;
    int32_t pid = strtol(rdir->d_name, &endptr, 10);

    if(pid > 0 && pid != getpid() && rdir->d_name != endptr) {
      print_fds(pid);
    }
  }


  closedir(odir);
  return 0;
}

//
static void print_fds(pid_t pid) {
  char path[1024];

  memset((void *)path, '\0', sizeof(path));

  snprintf(path, 1024, "/proc/%d/cmdline", pid);
  int fd = open(path, O_RDONLY);
  if(fd > 0) {
    char buffer[1024];

    memset((void *)buffer, '\0', sizeof(buffer));
    ssize_t rsize = read(fd, (void *)buffer, sizeof(char) * 1023);
    close(fd);

    if(rsize > 0) {
      char *p = strchr(buffer, ' ');
      if(p != NULL) {
        fwrite((const void *)buffer, sizeof(char),  (p - buffer), stdout);
      } else {
        fwrite((const void *)buffer, sizeof(char),  rsize, stdout);
      }
    } else {
      char *p = NULL, *p_cursor = NULL;
      for(p = (char *)path;
          p != NULL;
          p = strchr(p + 1, '/')) {
        p_cursor = p;
      }
      if(p_cursor != NULL) {
        *p_cursor = '\0';
      }

      fwrite((const void *)path, sizeof(char), strlen(path), stdout);
    }
    fwrite((const void *)"\n", sizeof(char), 1, stdout);
  } else {
    if(f_verbose) {
      eprintf(stderr, "open(2)", path);
    }
    return;
  }

  memset((void *)path, '\0', sizeof(path));
  snprintf(path, 1024, "/proc/%d/fd", pid);

  DIR *odir = opendir(path);
  if(odir == NULL) {
    if(f_verbose) {
      eprintf(stderr, "opendir(3)", path);
    }
    fprintf(stderr, "\n");
    return;
  }

  struct dirent *rdir = NULL;
  char *resolved_path = NULL;

  resolved_path = (char *)malloc((PATH_MAX + 1) * sizeof(char));
  if(resolved_path == NULL) {
    eprintf(stderr, "malloc(3)", NULL);

    closedir(odir);
    return;
  }

  while((rdir = readdir(odir)) != NULL) {
    if(*(rdir->d_name + 0) == '.') {
      continue;
    }

    memset((void *)path, '\0', sizeof(path));
    memset((void *)resolved_path, '\0', sizeof(char) * PATH_MAX);

    snprintf(path, 1024, "/proc/%d/fd/%s", pid, rdir->d_name);

    struct stat statbuf;
    memset((void *)&statbuf, '\0', sizeof(struct stat));

    int status = lstat(path, &statbuf);
    if(status != 0) {
      eprintf(stderr, "stat(2)", path);
    }


    switch(statbuf.st_mode & S_IFMT) {
      case S_IFLNK:
        realpath(path, resolved_path);
        fprintf(stdout, "\t\033[36;1m%s\033[0m -> %s", path, resolved_path);
        break;

      default:
        fprintf(stdout, "\t%s", path);
        break;
    }

    fprintf(stdout, "\n");
  }

  closedir(odir);

  memset((void *)resolved_path, '\0', sizeof(char) * PATH_MAX);
  free((void *)resolved_path);
  resolved_path = NULL;

  fwrite((const void *)"\n", sizeof(char), 1, stdout);
}

//
static int eprintf(
    FILE *strm,
    const char *fn,
    const char *param) {
  if(fn == NULL && param == NULL) {
    return EOF;
  }

  return (param != NULL)
    ? fprintf(strm, "%s: %s \033[1;31m%s\033[0m(%d)\n", fn, param, strerror(errno), errno)
    : fprintf(strm, "%s: \033[1;31m%s\033[0m(%d)\n", fn, strerror(errno), errno);
}

//
static void print_help(const char *app) {
  fprintf(stdout, "Usage: %s [OPTION]\n\n", app);
  fprintf(stdout, "[Options]\n");
  fprintf(stdout, "  -p, --pid     : プロセスID\n");
  fprintf(stdout, "  -v, --verbose : エラーメッセージ等を表示.\n");
  fprintf(stdout, "  -h, --help    : ヘルプを表示");
}
