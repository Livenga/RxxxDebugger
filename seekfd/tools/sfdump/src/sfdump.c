#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "../include/sfdump_type.h"
#include "../include/syscall_number_location_t.h"
#include "../include/util.h"



#if defined(__ENABLE_SIZE_32__)
//
static uint32_t conv_uint32_t(void *regs, uint16_t position, uint32_t value_size);
#else
//
static uint64_t conv_uint64_t(void *regs, uint16_t position, uint32_t value_size);
#endif

/**
 */
static struct syscall_number_location_t *_find_syscall(
    int32_t                          number,
    struct syscall_number_location_t *p_nsys,
    size_t                           size);

/**
 */
static char *_get_sycall_name(
    FILE    *fp,
    int32_t start,
    int32_t end,
    char    *buf,
    size_t  size);


void analyze_sfdump(
    const char *path,
    struct syscall_number_location_t *p_nsys,
    size_t syscall_count,
    FILE *fp_syscall) {
  extern uint8_t f_verbose;

  // システムコールが含まれているファイルへの fp とシステムコールの位置情報が取得済みの場合
  // システムコール名の取得を有効
  const uint8_t f_enable_syscall_name = p_nsys != NULL && fp_syscall;

  FILE *fp = fopen(path, "rb");
  if(fp == NULL) {
    eprintf(stderr, "fopen(3)", path);
    return;
  }

  char syscall_name[256];
  void *reg_values = NULL;

  struct sfdump_t header;
  memset((void *)&header, '\0', sizeof(struct sfdump_t));


  fread((void *)&header, sizeof(struct sfdump_t), 1, fp);
  if(f_verbose) {
    fprintf(stdout, "header: %c%02x, v%u, register size: %u\n",
        *(header.magic_number + 0),
        *(header.magic_number + 1),
        header.version,
        header.register_size);
  }

  uint16_t reg_count;
  switch(header.version) {
    case 100:
      reg_count = 6;
      break;

      // TODO: 各々のバージョンに合わせてデータを取得

    default:
      fprintf(stderr, "未対応のバージョン%u が指定されました.\n", header.version);
      goto pt_end;
  }


  reg_values = calloc(reg_count, header.register_size);
  if(reg_values == NULL) {
    eprintf(stderr, "calloc(3)", NULL);

    return;
  }

  size_t read_size = 0;
  while((read_size = fread(
          /* ptr    = */reg_values,
          /* size   = */header.register_size,
          /* nmemb  = */reg_count,
          /* stream = */fp)) > 0) {
#if defined(__ENABLE_SIZE_32__)
    uint32_t sys = conv_uint32_t(reg_values, 0, header.register_size);
#else
    uint64_t sys = conv_uint64_t(reg_values, 0, header.register_size);
#endif

    if(f_enable_syscall_name) {
      memset((void *)syscall_name, '\0', sizeof(syscall_name));

      struct syscall_number_location_t *p_sys = _find_syscall(
          /* number = */sys,
          /* p_nsys = */p_nsys,
          /* count  = */syscall_count);

      if(p_sys == NULL || 
          _get_sycall_name(
            fp_syscall,
            p_sys->start,
            p_sys->end,
            (char *)syscall_name,
            256) == NULL) {;
        snprintf(syscall_name, 256, "%u", sys);
      }
    } else {
      snprintf(syscall_name, 256, "%u", sys);
    }

#if defined(__ENABLE_SIZE_32__)
    uint32_t ip  = conv_uint32_t(reg_values, 1, header.register_size),
             ret = conv_uint32_t(reg_values, 2, header.register_size),
             r0  = conv_uint32_t(reg_values, 3, header.register_size),
             r1  = conv_uint32_t(reg_values, 4, header.register_size),
             r2  = conv_uint32_t(reg_values, 5, header.register_size);

    fprintf(stdout, "0x%08x: 0x%08x = %s(%u, %u, %u)\n",
        ip, ret, syscall_name,
        r0, r1, r2);
#else
    uint64_t ip  = conv_uint64_t(reg_values, 1, header.register_size),
             ret = conv_uint64_t(reg_values, 2, header.register_size),
             r0  = conv_uint64_t(reg_values, 3, header.register_size),
             r1  = conv_uint64_t(reg_values, 4, header.register_size),
             r2  = conv_uint64_t(reg_values, 5, header.register_size);

    fprintf(stdout, "%2lu: 0x%08lx = %4lu(%lu, %lu, %lu)\n",
        ip, ret, sys,
        r0, r1, r2);
#endif
  }


pt_end:
  fclose(fp);

  if(reg_values != NULL) {
    memset(reg_values, 0, reg_count * header.register_size);

    free(reg_values);
    reg_values = NULL;
  }
}


#if defined(__ENABLE_SIZE_32__)
static uint32_t conv_uint32_t(void *regs, uint16_t position, uint32_t value_size) {
  uint32_t value = 0;
  memcpy((void *)&value, regs + (position * value_size), value_size);
  return value;
}
#else
static uint64_t conv_uint64_t(void *regs, uint16_t position, uint32_t value_size) {
  uint64_t value = 0;
  memcpy((void *)&value, regs + (position * value_size), value_size);
  return value;
}
#endif


static char *_get_sycall_name(
    FILE   *fp,
    int32_t start,
    int32_t end,
    char    *buf,
    size_t  size) {
  int status = fseek(fp, start, SEEK_SET);
  if(status != 0) {
    eprintf(stderr, "fseek(3)", NULL);
    return NULL;
  }

  size_t range = (end - start) < size
    ? (end - start)
    : size; 

  fread(buf, sizeof(char), range, fp);

  return buf;
}

static struct syscall_number_location_t *_find_syscall(
    int32_t                          number,
    struct syscall_number_location_t *p_nsys,
    size_t                           size) {
  int i;
  for(i = 0; i < size; ++i) {
    if((p_nsys + i)->number == number) {
      return (p_nsys + i);
    }
  }

  return NULL;
}
