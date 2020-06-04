#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <sys/syscall.h>

#include "../include/seekfd_type.h"
#include "../include/seekfd_dump.h"
#include "../include/util.h"


static uint8_t is_without_fd(int32_t fd);
//#define BUFFER_LIMIT 8192
#define BUFFER_LIMIT 65536

extern void seekfd_write_reg(
    int fd,
    unsigned long int sys,
    unsigned long int ip,
    unsigned long int ret,
    unsigned long int r0,
    unsigned long int r1,
    unsigned long int r2,
    unsigned long int r3,
    unsigned long int r4,
    unsigned long int r5);


#if 0
/** 対象プロセスで管理されているメモリからデータを取得
 * @param pid  対象プロセスID
 * @param addr 開始アドレス
 * @param buf  読込データ格納先
 * @param size 読込サイズ
 * @return 成功時: 読込サイズ, (ptrace)失敗時: -1
 */
static size_t peek_data(
    pid_t    pid,
    uint32_t addr,
    uint8_t  *buf,
    uint32_t size);
#endif

/**
*/
int do_seekfd(struct seekfd_arg_t args) {
  extern uint8_t f_verbose;
  extern uint8_t f_output;
  extern uint8_t g_f_is_continue;


  int status;

  status = ptrace(PTRACE_ATTACH, args.pid, NULL, NULL);
  if(status == -1) {
    eprintf(stderr, "ptrace(2)", "PTRACE_ATTACH");

    return EOF;
  }
  status = ptrace(PTRACE_SYSCALL, args.pid, NULL, NULL);


  int wstatus = 0;
#if defined(__ARM_EABI__)
  struct user_regs regs;
  memset((void *)&regs, 0, sizeof(struct user_regs));
#else
  struct user_regs_struct regs;
  memset((void *)&regs, 0, sizeof(struct user_regs_struct));
#endif


  char msg[128];
  memset((void *)msg, '\0', sizeof(msg));

  while(g_f_is_continue) {
    //fprintf(stderr, "--- Running...\n");
    status = waitpid(args.pid, &wstatus, 0);

    if(WIFEXITED(wstatus)) {
      break;
    } else if(WIFSIGNALED(wstatus)) {
      if(f_verbose) {
        fprintf(stderr, "- terminated by signal %d\n", WTERMSIG(wstatus));
      }
    } else if(WIFSTOPPED(wstatus)) {
      ptrace(PTRACE_GETREGS, args.pid, NULL, &regs);

#if defined(__ARM_EABI__)
      unsigned long int ret = 0;
      unsigned long int r0  = 0,
                    r1 = 0,
                    r2 = 0,
                    r3 = 0,
                    r4 = 0,
                    r5 = 0;
      unsigned long int sys = 0,
                    ip = 0;

      //uint8_t *seek_buffer = NULL;

      sys = *(regs.uregs + 7);
      ip  = *(regs.uregs + 12);
      if(ip  == 1) {
        r0  = *(regs.uregs + 17);
        ret = *(regs.uregs + 0);
      } else {
        r0  = *(regs.uregs + 0);
        ret = 0;
      }
      r1 = *(regs.uregs + 1);
      r2 = *(regs.uregs + 2);
      r3 = *(regs.uregs + 3);
      r4 = *(regs.uregs + 4);
      r5 = *(regs.uregs + 5);


      // このプログラムではデータの操作を想定していないため
      // 呼び出し完了時の処理に限定
      if(ip == 1) {

        // Note: 書き込み順序について下記の順序に従う.
        // v1   @ [sys, ip, ret, r0, r1, r2]
        // v1.1 @ [sys, ip, ret, r0, r1, r2, r3, r4, r5]
        // v2.0 @ [sys, ip, ret, r0, r1, r2, r3, r4, r5] [ buffer_size(int32_t) buffer ]
        if(sys == SYS_read
            || sys == SYS_readv
            || sys == SYS_recv
            || sys == SYS_write
            || sys == SYS_writev
            || sys == SYS_send) {
          if(is_without_fd(r0)) {
            goto pt_continue;
          }
        }

        if(f_output) {
          int32_t _ret_size = 0;

          seekfd_write_reg(
              args.output_fd,
              sys, ip, ret,
              r0, r1, r2, r3, r4, r5);

          switch(sys) {
            case SYS_open:
              seekfd_write_peekdata(
                  args.output_fd,
                  args.pid,
                  r0,
                  256);
              break;

            case SYS_read:
            case SYS_recv:
            case SYS_write:
            case SYS_send:
              seekfd_write_peekdata(
                  args.output_fd,
                  args.pid,
                  r1,
                  ret);
              break;

            case SYS_readv:
              seekfd_dump_readv(
                  args.output_fd,
                  args.pid,
                  r1,
                  r2);
              break;

            case SYS_writev:
              seekfd_dump_writev(
                  args.output_fd,
                  args.pid,
                  r1,
                  r2);
              break;

            default:
              _ret_size = 0;
              write(args.output_fd, (const void *)&_ret_size, sizeof(int32_t));
              break;
          }
        }

        if(f_verbose) {
          //memset((void *)msg, '\0', sizeof(msg));
          snprintf(msg, 128, "%lu = %lu(%lu, %lu, %lu, %lu, %lu, %lu)\n",
              ret, sys, r0, r1, r2, r3, r4, r5);

          write(STDOUT_FILENO, (const void *)msg, sizeof(char) * strlen(msg));
        }
      }
#endif

#if 0
      if(f_verbose) {
        fprintf(stderr, "- stopped by signal %d\n", WSTOPSIG(wstatus));
      }
#endif
    }

pt_continue:
    ptrace(PTRACE_SYSCALL, args.pid, NULL, NULL);
  }

  status = ptrace(PTRACE_DETACH, args.pid, NULL, NULL);
  if(status == -1) {
    eprintf(stderr, "ptrace(2)", "PTRACE_DETACH");
  }

  return 0;
}

//
static uint8_t is_without_fd(int32_t fd) {
  extern int32_t g_without_fd[32];

  int i;
  for(i = 0; i < 32; ++i) {
    if(*(g_without_fd + i) == fd) {
      return 1;
    }
  }

  return 0;
}


#if 0
//
static size_t peek_data(
    pid_t    pid,
    uint32_t addr,
    uint8_t  *buf,
    uint32_t size) {
  extern uint8_t f_verbose;

  int i;

  for(i = 0; i < size; i += sizeof(long)) {
    long data = ptrace(
        /* request = */PTRACE_PEEKDATA,
        /* pid     = */pid, 
        /* addr    = */addr + i,
        /* data    = */NULL);

    if(data != -1) {
      memcpy((void *)(buf + i), (const void *)&data, sizeof(long));
    } else {
      if(f_verbose) {
        eprintf(stderr, "ptrace(2)", "PTRACE_PEEKDATA");
      }
      return -1;
    }
  }

  return (size_t)size;
}
#endif
