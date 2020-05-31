#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "../include/util.h"
#include "../include/sfdump_type.h"



#if defined(__ENABLE_SIZE_32__)
static uint32_t conv_uint32_t(void *regs, uint16_t position, uint32_t value_size);
#else
static uint64_t conv_uint64_t(void *regs, uint16_t position, uint32_t value_size);
#endif


void analyze_sfdump(
    const char *path,
    const char *define_path) {
  extern uint8_t f_verbose;

  if(f_verbose) {
    fprintf(stderr, "path: %s\n", path);

    if(define_path != NULL) {
      fprintf(stderr, "dpath: %s\n", define_path);
    }
  }

  FILE *fp = fopen(path, "rb");
  if(fp == NULL) {
    eprintf(stderr, "fopen(3)", path);
    return;
  }

  struct sfdump_t header;
  memset((void *)&header, '\0', sizeof(struct sfdump_t));

  fread((void *)&header, sizeof(struct sfdump_t), 1, fp);
  fprintf(stdout, "header: %c%02x, v%u, register size: %u\n",
      *(header.magic_number + 0),
      *(header.magic_number + 1),
      header.version,
      header.register_size);

  uint16_t reg_count;
  switch(header.version) {
    case 1:
      reg_count = 6;
      break;

    default:
      fprintf(stderr, "未対応のバージョン%u が指定されました.", header.version);
      goto pt_end;
  }

  void *reg_values = calloc(reg_count, header.register_size);

  size_t read_size = 0;
  while((read_size = fread(
          /* ptr    = */reg_values,
          /* size   = */header.register_size,
          /* nmemb  = */reg_count,
          /* stream = */fp)) > 0) {
#if defined(__ENABLE_SIZE_32__)
    uint32_t sys = conv_uint32_t(reg_values, 0, header.register_size),
    ip  = conv_uint32_t(reg_values, 1, header.register_size),
    ret = conv_uint32_t(reg_values, 2, header.register_size),
    r0  = conv_uint32_t(reg_values, 3, header.register_size),
    r1  = conv_uint32_t(reg_values, 4, header.register_size),
    r2  = conv_uint32_t(reg_values, 5, header.register_size);

    fprintf(stdout, "0x%08x: 0x%08x = %4u(%u, %u, %u)\n",
        ip, ret, sys,
        r0, r1, r2);
#else
    uint64_t sys = conv_uint64_t(reg_values, 0, header.register_size),
    ip  = conv_uint64_t(reg_values, 1, header.register_size),
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
