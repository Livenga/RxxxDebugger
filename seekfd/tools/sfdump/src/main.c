#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>

#include "../include/syscall_number_location_t.h"
#include "../include/util.h"


#define GET_OPTARG(argv, optarg, optind) \
  ((optarg != NULL) ? optarg : *(argv + optind))


/* src/sfdump.c */
extern void analyze_sfdump(
    const char *path,
    struct syscall_number_location_t *p_nsys,
    size_t syscall_count,
    FILE *fp_syscall);


/* src/syscall_number.c */
extern struct syscall_number_location_t *syscall_number_get(
    FILE   *fp,
    size_t *p_count);


static uint8_t is_optarg(
    const struct option *opts,
    int                 argc,
    char                *argv[],
    const char          *value);

static void print_help(const char *app);

uint8_t f_verbose = 0;


/**
 */
int main(
    int argc,
    char *argv[]) {
  const struct option opts[] = {
    {"define-file", required_argument, 0, 'D'},
    {"verbose",     no_argument,       0, 'v'},
    {"help",        no_argument,       0, 'h'},
    {0, 0, 0, 0},
  };


  if(argc == 1) {
    print_help(argv[0]);
    return EOF;
  }


  FILE *fp_syscall = NULL;
  char define_path[1024];

  memset((void *)define_path, '\0', sizeof(define_path));

  size_t nsys_count = 0;
  struct syscall_number_location_t *p_nsys = NULL;


  int opt;
  while((opt = getopt_long(
          /* argc      = */argc,
          /* argv      = */argv,
          /* optstring = */"D:vh",
          /* longopts  = */opts,
          /* longindex = */NULL)) > 0) {
    char *_optarg = NULL;

    switch(opt) {
      case 'D':
        _optarg = GET_OPTARG(argv, optarg, optind);
        strncpy(define_path, _optarg, 1024);

        fp_syscall = fopen(define_path, "r");
        if(fp_syscall != NULL) {
          p_nsys = syscall_number_get(fp_syscall, &nsys_count);
        } else {
          eprintf(stderr, "fopen(3)", define_path);
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


  if(*(char *)*(argv + (argc - 1)) == '-'
      || is_optarg(opts, argc, argv, NULL)) {
    fprintf(stderr, "*.sfdump ファイルを指定してください.\n");
    return EOF;
  }

  const char *sfd_path = *(argv + (argc - 1));

  analyze_sfdump(
      /* path          = */sfd_path,
      /* p_nsys        = */p_nsys,
      /* syscall_count = */nsys_count,
      /* fp_syscall    = */fp_syscall);


  if(fp_syscall != NULL) {
    fclose(fp_syscall);
  }

  if(p_nsys != NULL) {
    memset((void *)p_nsys, 0, sizeof(struct syscall_number_location_t) * nsys_count);
    free((void *)p_nsys);

    p_nsys = NULL;
  }

  return 0;
}

static uint8_t is_optarg(
    const struct option *opts,
    int argc,
    char *argv[],
    const char *value) {
  char *arg = NULL;

  if(value == NULL) {
    value = (const char *)*(argv + (argc - 1));
  }
#if defined(__DEBUG__)
  fprintf(stderr, "[D] *value: %p %s\n", value, value);
#endif

  int i;
  for(i = 0; i < argc; ++i) {
    if(value == *(argv + i)) {
      arg = *(argv + i--);
      break;
    }
  }

  if(arg == NULL) {
    return 0;
  }

  char *p_pre = *(argv + i);
#if defined(__DEBUG__)
  fprintf(stderr, "[D] *arg: %p %s\n", arg, arg);
  fprintf(stderr, "[D] *(argv + %d): %p, %s\n", i, p_pre, p_pre);
#endif

  if(*p_pre != '-' || strlen(p_pre) <= 1) {
    return 0;
  }

  const struct option *_opts = opts;

  do {
    if(*p_pre == '-'
        && strcmp(_opts->name, (p_pre + 2)) == 0
        && _opts->has_arg == required_argument) {
      return 1;
    } else if(*(p_pre + 1) == _opts->val && _opts->has_arg == required_argument) {
      return 1;
    }
  } while((++_opts)->name != NULL);

  return 0;
}


//
static void print_help(const char *app) {
  fprintf(stdout, "Usage: %s [OPTION] [sfdump]ファイル\n\n", app);
  fprintf(stdout, "[Option]\n");
  fprintf(stdout, "    -D, --define-file : システムコール名定義ファイル\n");
  fprintf(stdout, "\n");
  fprintf(stdout, "    -v, --verbose : 冗長メッセージ表示.\n");
  fprintf(stdout, "    -h, --help    : ヘルプ表示.\n");
}
