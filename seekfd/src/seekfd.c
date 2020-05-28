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
#else
  struct user_regs_struct regs;
  memset((void *)&regs, 0, sizeof(struct user_regs_struct));
#endif


  char verbose_message[128];
  memset((void *)verbose_message, '\0', sizeof(verbose_message));

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

      size_t seek_bsize;
      uint8_t *seek_buffer = NULL;

      long arg1;

#if defined(__ARM_EABI__)
      arg1 = *(regs.uregs + 4);

      switch(*(regs.uregs + 7)) {
        case SYS_read:
          if(arg->target_fd == -1 || arg1 == arg->target_fd) {
            if(*(regs.uregs + 12) == 1) {
              //size_t seek_bsize = *(regs.uregs + 2) + 1;
              seek_bsize = *(regs.uregs + 0);

              // 呼び出し
              seek_buffer = (uint8_t *)calloc(
                  /* nmemb = */seek_bsize + sizeof(long),
                  /* size  = */sizeof(uint8_t));
              if(seek_buffer != NULL) {
                unsigned long start_addr = *(regs.uregs + 1);

                peek_data(
                    /* pid  = */arg->pid,
                    /* addr = */start_addr,
                    /* buf  = */seek_buffer,
                    /* size = */seek_bsize);

                if(f_verbose) {
                  snprintf(verbose_message, 128, "<- read(fd: %ld)\n", arg1);
                  write(STDOUT_FILENO, (const void *)verbose_message, sizeof(char) * strlen(verbose_message));
                  write(STDOUT_FILENO, (const void *)seek_buffer, sizeof(char) * seek_bsize);
                  write(STDOUT_FILENO, (const void *)"\n", sizeof(char) * 1);
                } else {
                  write(STDOUT_FILENO, (const void *)seek_buffer, sizeof(char) * seek_bsize);
                }

                // 解放
                memset((void *)seek_buffer, '\0', sizeof(char) * seek_bsize);
                free((void *)seek_buffer);
                seek_buffer = NULL;
              } else {
                eprintf(stderr, "calloc(3)", NULL);
              }
            } else {
            }
          }
          break;

        case SYS_write:
          if(arg->target_fd == -1 || arg1 == arg->target_fd) {
            if(*(regs.uregs + 12) == 1) {
              seek_bsize  = *(regs.uregs + 0);
              seek_buffer = (uint8_t *)calloc(
                  /* nmemb = */seek_bsize + sizeof(long),
                  /* size  = */sizeof(uint8_t));
              if(seek_buffer != NULL) {
                unsigned long start_addr = *(regs.uregs + 1);

                peek_data(
                    /* pid  = */arg->pid,
                    /* addr = */start_addr,
                    /* buf  = */seek_buffer,
                    /* size = */seek_bsize);

                if(f_verbose) {
                  snprintf(verbose_message, 128, "-> write(fd: %ld)\n", arg1);
                  write(STDOUT_FILENO, (const void *)verbose_message, sizeof(char) * strlen(verbose_message));
                  write(STDOUT_FILENO, (const void *)seek_buffer, sizeof(char) * seek_bsize);
                  write(STDOUT_FILENO, (const void *)"\n", sizeof(char) * 1);
                } else {
                  write(STDOUT_FILENO, (const void *)seek_buffer, sizeof(char) * seek_bsize);
                }

                // 解放
                memset((void *)seek_buffer, '\0', sizeof(char) * seek_bsize);
                free((void *)seek_buffer);
                seek_buffer = NULL;
              } else {
                eprintf(stderr, "calloc(3)", NULL);
              }
            }
          }
          break;
      }
#if 0
      for(i = 0; i < 18; ++i) {
        fprintf(stderr, "uregs[%2d]: 0x%08X %lu\n",
            i,
            (unsigned)*(regs.uregs + i),
            *(regs.uregs + i));
      }
#endif
#else
      fprintf(stderr, "- %llu %llu %llu %llu\n",
          regs.orig_rax,
          regs.rdi,
          regs.rsi,
          regs.rdx);
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
      eprintf(stderr, "ptrace(2)", "PTRACE_PEEKDATA");
      return -1;
    }
  }

  return (size_t)size;
}
