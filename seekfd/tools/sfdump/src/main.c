#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>


#define __DEBUG__
#define GET_OPTARG(argv, optarg, optind) \
  ((optarg != NULL) ? optarg : *(argv + optind))


/* src/sfdump.c */
extern void analyze_sfdump(
    const char *path,
    const char *define_path);


static uint8_t is_optarg(
    const struct option *opts,
    int argc,
    char *argv[],
    const char *value);

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
    return EOF;
  }


  char define_path[1024];
  memset((void *)define_path, '\0', sizeof(define_path));

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
        break;

      case 'v':
        f_verbose = 1;
        break;

      case 'h':
        break;
    }
  }


  if(*(char *)*(argv + (argc - 1)) == '-'
      || is_optarg(opts, argc, argv, NULL)) {
    fprintf(stderr, "*.sfdump ファイルを指定してください.\n");
    return EOF;
  }

  const char *sfd_path = *(argv + (argc - 1));

  analyze_sfdump(
      /* path        = */sfd_path,
      /* define_path = */(strlen(define_path) > 0) ? define_path : NULL);

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
