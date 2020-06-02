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
#include "../include/util.h"

#include "../libtask/include/libtask.h"



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

      if(f_output) {
        // Note: 書き込み順序について下記の順序に従う.
        // v1 @ [sys, ip, ret, r0, r1, r2]
#if 0
        write(arg->output_fd, (const void *)&sys,  sizeof(sys));
        write(arg->output_fd, (const void *)&ip,   sizeof(ip));
        write(arg->output_fd, (const void *)&ret,  sizeof(ret));
        write(arg->output_fd, (const void *)&r0,   sizeof(r0));
        write(arg->output_fd, (const void *)&r1,   sizeof(r1));
        write(arg->output_fd, (const void *)&r2,   sizeof(r2));
        // 書き込み及び受信するデータや第三引数以降のデータを記録
        write(arg->output_fd, (const void *)&r3,   sizeof(r3));
        write(arg->output_fd, (const void *)&r4,   sizeof(r4));
        write(arg->output_fd, (const void *)&r5,   sizeof(r5));
#else
        seekfd_write_reg(
            args.output_fd,
            sys, ip, ret,
            r0, r1, r2, r3, r4, r5);
#endif
      }

#if defined(__ENABLE_DISPLAY_ALL__)
      // Note: ヘルスチェックに失敗する可能性があり, 再起動を検出
      snprintf(msg, 128, "%lu = %lu(%lu, %lu, %lu, %lu, %lu, %lu)\n",
          ret, sys, r0, r1, r2, r3, r4, r5);
      write(STDOUT_FILENO, (const void *)msg, sizeof(char) * strlen(msg));
#else
      switch(sys) {
        case SYS_read:
        case SYS_readv:
        case SYS_write:
        case SYS_writev:
        case SYS_send:
        case SYS_recv:
          snprintf(msg, 128, "%lu = %lu(%lu, %lu, %lu, %lu, %lu, %lu)\n",
              ret, sys, r0, r1, r2, r3, r4, r5);
          write(STDOUT_FILENO, (const void *)msg, sizeof(char) * strlen(msg));
          break;
      }
#endif
#endif
      if(f_verbose) {
        fprintf(stderr, "- stopped by signal %d\n", WSTOPSIG(wstatus));
      }
    }

    ptrace(PTRACE_SYSCALL, args.pid, NULL, NULL);
  }

  // Note: 下記の処理はメインスレッドで行う
  status = ptrace(PTRACE_DETACH, args.pid, NULL, NULL);
  if(status == -1) {
    eprintf(stderr, "ptrace(2)", "PTRACE_DETACH");
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
