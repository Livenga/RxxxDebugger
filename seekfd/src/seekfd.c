#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
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


//#define BUFFER_LIMIT 8192
#define BUFFER_LIMIT 65536


/** スレッドを持続可能かどうかを確認
 * @param mutex Muex
 * @return 1 or 0
 */
static uint8_t is_continue(pthread_mutex_t *p_mutex);

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


/**
 */
void *thread_seekfd(void *p_arg) {
  extern uint8_t f_verbose;
  extern uint8_t f_output;

  struct thread_seekfd_arg_t *arg = (struct thread_seekfd_arg_t *)p_arg;
  int status;


  status = ptrace(PTRACE_ATTACH, arg->pid, NULL, NULL);
  if(status == -1) {
    eprintf(stderr, "ptrace(2)", "PTRACE_ATTACH");

    return NULL;
  }


  int wstatus = 0;
#if defined(__ARM_EABI__)
  struct user_regs regs;
  memset((void *)&regs, 0, sizeof(struct user_regs));

  if(f_output) {
    uint32_t register_size = sizeof(regs.uregs) / sizeof(regs.uregs[0]);
    write(arg->output_fd, (const void *)&register_size, sizeof(uint32_t));
  }
#else
  struct user_regs_struct regs;
  memset((void *)&regs, 0, sizeof(struct user_regs_struct));
#endif


  char msg[128];
  memset((void *)msg, '\0', sizeof(msg));

  for(;;) {
    //fprintf(stderr, "--- Running...\n");
    status = waitpid(arg->pid, &wstatus, 0);

    if(WIFEXITED(wstatus)) {
      break;
    } else if(WIFSIGNALED(wstatus)) {
      if(f_verbose) {
        fprintf(stderr, "- terminated by signal %d\n", WTERMSIG(wstatus));
      }
    } else if(WIFSTOPPED(wstatus)) {
      ptrace(PTRACE_GETREGS, arg->pid, NULL, &regs);

#if defined(__ARM_EABI__)
      unsigned long int ret = 0;
      unsigned long int r0  = 0,
                    r1 = 0,
                    r2 = 0;
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
      //r3 = *(regs.uregs + 3);
      //r4 = *(regs.uregs + 4);

      if(f_output) {
        // Note: 書き込み順序について下記の順序に従う.
        // v1 @ [sys, ip, ret, r0, r1, r2]
        write(arg->output_fd, (const void *)&sys, sizeof(sys));
        write(arg->output_fd, (const void *)&ip,  sizeof(ip));
        write(arg->output_fd, (const void *)&r0,  sizeof(r0));
        write(arg->output_fd, (const void *)&r1,  sizeof(r1));
        write(arg->output_fd, (const void *)&r2,  sizeof(r2));

        // TODO: 書き込み及び受信するデータや第三引数以降のデータを記録
      }

#if 0
      // Note: ヘルスチェックに失敗する可能性があり, 再起動を検出
      snprintf(msg, 128, "%lu = %lu(%lu, %lu, %lu)\n",
          ret, sys, r0, r1, r2);
      write(STDOUT_FILENO, (const void *)msg, sizeof(char) * strlen(msg));
#else
      switch(sys) {
        case SYS_read:
        case SYS_write:
          snprintf(msg, 128, "%lu = %lu(%lu, %lu, %lu)\n",
              ret, sys, r0, r1, r2);
          write(STDOUT_FILENO, (const void *)msg, sizeof(char) * strlen(msg));
          break;
      }
#endif
#endif
      //fprintf(stderr, "- stopped by signal %d\n", WSTOPSIG(wstatus));
    }

    if(is_continue(arg->mutex)) {
      ptrace(PTRACE_SYSCALL, arg->pid, NULL, NULL);
    } else {
      break;
    }
  }

  status = ptrace(PTRACE_DETACH, arg->pid, NULL, NULL);
  if(status == -1) {
    eprintf(stderr, "ptrace(2)", "PTRACE_DETACH");
  }

  return NULL;
}


static uint8_t is_continue(pthread_mutex_t *p_mutex) {
  extern uint8_t f_thread_exit;
  uint8_t ret = 0;

  int status = pthread_mutex_lock(p_mutex);
  if(status != 0) {
    eprintf(stderr, "pthread_mutex_lock(3)", NULL);

    return 0;
  }

  ret = (f_thread_exit == 0);

  status = pthread_mutex_unlock(p_mutex);
  if(status != 0) {
    eprintf(stderr, "pthread_mutex_unlock(3)", NULL);

    return 0;
  }

  return ret;
}


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
