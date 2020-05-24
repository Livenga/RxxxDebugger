#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <termios.h>
#include <signal.h>
#include <fcntl.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "../include/util.h"



static void print_help(const char *app);
extern int signal_init(void);
extern int tty_capture(
    const char *path,
    const char *output_directory);


uint8_t f_verbose = 0;
uint8_t f_stopped = 0;


#define GET_OPTARG(argv, optarg, optind) \
  ((optarg != NULL) ? optarg : argv[optind])

int main(
    int argc,
    char *argv[]) {
  const struct option opts[] = {
    {"output-directory", required_argument, 0, 'd'},
    {"device",  required_argument, 0, 'D'},
    {"verbose", no_argument,       0, 'v'},
    {"help",    no_argument,       0, 'h'},
    {0, 0, 0, 0},
  };


  char device_path[1024];
  char output_directory_path[1024];
  memset((void *)device_path, '\0', sizeof(device_path));
  memset((void *)output_directory_path, '\0', sizeof(output_directory_path));

  struct stat stat;
  int opt, longopt;

  *output_directory_path  = '.';
  while((opt = getopt_long(
          /* argc      = */argc,
          /* argv      = */argv,
          /* optstring = */"d:D:vh",
          /* longopts  = */opts,
          /* longindex = */&longopt)) > 0) {
    char *_optarg = NULL;

    switch(opt) {
      case 'd':
        _optarg = GET_OPTARG(argv, optarg, optind);
        if(strlen(_optarg) > 1024) {
          fprintf(stderr, "%s: ディレクトリ名が長すぎます.\n", _optarg);
          return EOF;
        }

        if(access(_optarg, R_OK | W_OK) != 0) {
          fprintf(stderr, "%s: 指定されたディレクトリが存在しないか, 読込 / 書込の権限がありません.\n", _optarg);
          eprintf(stderr, "access(2)", _optarg);
          return EOF;
        }

        memset((void *)&stat, '\0', sizeof(struct stat));
        if(lstat(_optarg, &stat) != 0) {
          eprintf(stderr, "lstat(2)", _optarg);
          return EOF;
        }

        if(! S_ISDIR(stat.st_mode)) {
          fprintf(stderr, "%s: ディレクトリを指定してください.\n", _optarg);
          return EOF;
        }

        strncpy(output_directory_path, _optarg, 1024);
        // 終端文字の確認
        int i;
        for(i = strlen(output_directory_path) - 1;
            i > 0 && *(output_directory_path + i) == '/';
            --i) {
          *(output_directory_path + i) = '\0';
        }

        break;

      case 'D':
        _optarg = GET_OPTARG(argv, optarg, optind);
        strncpy((char *)device_path, (const char *)_optarg, 1023);
        break;

      case 'v':
        f_verbose = 1;
        break;

      case 'h':
        print_help(argv[0]);
        return 0;
    }
  }

  if(strlen(device_path) == 0) {
    fprintf(stderr, "デバイスを指定してください.\n");
    return EOF;
  }

  if(signal_init() != 0) {
    eprintf(stderr, "sigaction(2)", NULL);

    return EOF;
  }


  if(f_verbose) {
    fprintf(stderr, "-- Target device path    : %s\n", device_path);
    fprintf(stderr, "-- Output Directory path : %s\n", output_directory_path);
  }

  tty_capture(device_path, output_directory_path);

  return 0;
}


static void print_help(const char *app) {
  fprintf(stderr, "Usage: %s [OPTION]...\n\n", app);
  fprintf(stderr, "[Option]\n");
  fprintf(stderr, "  -D, --device           : デバイス名\n");
  fprintf(stderr, "  -d, --output-directory : 出力先ディレクトリの指定.\n\n");
  fprintf(stderr, "  -v, --verbose          : 冗長\n");
  fprintf(stderr, "  -h, --help             : ヘルプ\n");
  fprintf(stderr, "\n");
}
