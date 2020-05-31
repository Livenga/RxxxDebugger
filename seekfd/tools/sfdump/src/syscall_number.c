#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#include "../include/syscall_number_location_t.h"
#include "../include/util.h"


static struct syscall_number_location_t *_syscall_number_new(FILE *fp);

struct syscall_number_location_t *syscall_number_get(
    FILE *fp,
    size_t *p_count) {
  fseek(fp, 0L, SEEK_SET);

  struct syscall_number_location_t *p_root = NULL,
                                   *p_prev = NULL;
  while(feof(fp) == 0) {
    struct syscall_number_location_t *p_nsys = _syscall_number_new(fp);

    if(p_nsys != NULL) {
      //fprintf(stderr, "%d\n", p_nsys->number);

      if(p_root == NULL) {
        p_root = p_nsys;
      }

      if(p_prev != NULL) {
        p_prev->next = p_nsys;
      }
      p_prev = p_nsys;
    }
  }

  uint32_t count = 0;
  for(p_prev = p_root; p_prev != NULL; p_prev = p_prev->next) {
    ++count;
  }

  struct syscall_number_location_t *n_nsyses = (struct syscall_number_location_t *)calloc(count, sizeof(struct syscall_number_location_t));
  if(p_count != NULL) {
    *p_count = count;
  }

  count = 0;
  for(p_prev = p_root; p_prev != NULL; p_prev = p_prev->next) {
    struct syscall_number_location_t *p_nsys = n_nsyses + count++;
    memcpy(p_nsys, p_prev, sizeof(struct syscall_number_location_t));

    p_nsys->next = NULL;
  }

  do {
    p_prev = p_root->next;

    memset((void *)p_root, '\0', sizeof(struct syscall_number_location_t));
    p_root->next = NULL;
    free((void *)p_root);

    p_root = p_prev;
  } while(p_root != NULL);


  return n_nsyses;
}


static struct syscall_number_location_t *_syscall_number_new(FILE *fp) {
  char buf[1024];
  memset((void *)buf, '\0', sizeof(buf));

  if(fgets((char *)buf, 1024, fp) == NULL) {
#if defined(__DEBUG__)
    eprintf(stderr, "fgets(3)", NULL);
#endif
    return NULL;
  }

  long l_start = 0, l_end = 0;

  // システムコール名開始位置
  // ファイルディスクリプタの現在の位置から
  // fgets で読み取った文字数を戻す.
  l_start = ftell(fp) - strlen(buf);

  char *p_eq = strchr(buf, '='),
       *p_nl = strchr(buf, '\n');

  // 改行文字の除去
  if(p_nl != NULL) {
    *p_nl = '\0';
  } else {
    p_nl = (char *)(buf + strlen(buf) - 1);
  }

  if(p_eq == NULL) {
    extern uint8_t f_verbose;
    if(f_verbose) {
      fprintf(stderr, "%s: フォーマットエラー. [key]=[value]の形式になっていない.\n", buf);
    }
    return NULL;
  }

  // 開始位置に '=' までの文字数を加算
  // l_start から l_end までがシステムコール名になる
  l_end = l_start + (p_eq - buf);


  char *endptr = NULL;
  int32_t number = strtol(p_eq + 1, &endptr, 10);
  if((p_eq + 1) == endptr) {
    return NULL;
  }

  struct syscall_number_location_t *p_ret = NULL;
  p_ret = (struct syscall_number_location_t *)calloc(1, sizeof(struct syscall_number_location_t));

  if(p_ret == NULL) {
    eprintf(stderr, "calloc(3)", NULL);

    return NULL;
  }

  p_ret->start  = l_start;
  p_ret->end    = l_end;
  p_ret->number = number;
  p_ret->next   = NULL;

  return p_ret;
}
