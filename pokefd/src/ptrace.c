#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/syscall.h>

#include "../include/pokefd_type.h"
#include "../include/util.h"


/**
 */
static uint8_t _is_continue(pthread_mutex_t *pmtx);



void *pthread_pokefd(void *p_args) {
  extern uint8_t g_f_verbose;
  const struct pokefd_thread_args_t *args = (struct pokefd_thread_args_t *)p_args;

  int status = ptrace(
      /* request = */PTRACE_ATTACH,
      /* pid     = */args->target_pid,
      /* addr    = */NULL,
      /* data    = */NULL);
  if(status != 0) {
    eprintf(stderr, "ptrace(2)", "PTRACE_ATTACH");
    return NULL;
  }

  while(_is_continue(args->mutex)) {
    int wstatus = 0;
    status = waitpid(args->target_pid, &wstatus, 0);

    if(WIFEXITED(wstatus)) {
      break;
    } else if(WIFSIGNALED(wstatus)) {
      if(g_f_verbose) {
        fprintf(stderr, "[D] terminated by signal %d\n", WTERMSIG(wstatus));
      }
    } else if(WIFSTOPPED(wstatus)) {
#if defined(__ARM_EABI__) && __ARM_EABI__ == 1
      struct user_regs regs;
#else
      struct user_regs_struct regs;
#endif
      status = ptrace(
          /* request = */PTRACE_GETREGS,
          /* pid     = */args->target_pid,
          /* addr    = */NULL,
          /* data    = */&regs);

#if defined(__ARM_EABI__) && __ARM_EABI__ == 1
      int32_t ip  = *(regs.uregs + 12);
      int32_t ret = (ip == 1) ? *(regs.uregs + 0) : -1;
      int32_t sys = *(regs.uregs + 7);

      int32_t r0  = (ip == 0) ? *(regs.uregs + 0) : *(regs.uregs + 17);
      int32_t r1 = *(regs.uregs + 1);
      int32_t r2 = *(regs.uregs + 2);
#else
      int64_t ip  = ((int64_t)-ENOSYS != regs.rax);
      int64_t ret = (ip == 1) ? regs.rax : -1;
      int64_t sys = regs.orig_rax;

      int64_t r0  = regs.rdi;
      int64_t r1  = regs.rsi;
      int64_t r2  = regs.rdx;
#endif

      // システムコールの戻りのみを検出
      if(0 && ip == 0) {
        switch(sys) {
          // read / write
          case SYS_read:
          case SYS_write:
#if 1
            fprintf(stderr, "%ld, %ld = %ld(%ld, %ld, %ld)\n", ip, ret, sys, r0, r1, r2);
#else
            if(args->target_fd == r0) {
              r2 = 0;
              fprintf(stderr, "%ld, %ld = %ld(%ld, %ld, %ld)\n", ip, ret, sys, r0, r1, r2);
              ptrace(PTRACE_SETREGS, args->target_pid, NULL, &regs); 
            }
#endif
            break;
        }
      } else if(ip == 1) {
        switch(sys) {
          // read / write
          case SYS_read:
          case SYS_write:
#if 1
            fprintf(stderr, "%ld, %ld = %ld(%ld, %ld, %ld)\n", ip, ret, sys, r0, r1, r2);
#else
            if(args->target_fd == r0) {
              fprintf(stderr, "%ld, %ld = %ld(%ld, %ld, %ld)\n", ip, ret, sys, r0, r1, r2);
            }
#endif
            break;
        }
      }
    }

    if(_is_continue(args->mutex)) {
      ptrace(
          PTRACE_SYSCALL,
          args->target_pid,
          NULL,
          NULL);
    }
  }

  ptrace(
      /* request = */PTRACE_DETACH,
      /* pid     = */args->target_pid,
      /* addr    = */NULL,
      /* data    = */NULL);

  return NULL;
}


//
static uint8_t _is_continue(pthread_mutex_t *pmtx) {
  extern uint8_t g_is_continue;
  uint8_t f = 0;

  pthread_mutex_lock(pmtx);
  f = g_is_continue;
  pthread_mutex_unlock(pmtx);

  return f;
}
