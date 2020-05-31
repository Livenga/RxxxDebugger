#ifndef _SYSCALL_NUMBER_LOCATION_T_H
#define _SYSCALL_NUMBER_LOCATION_T_H

#include <stdint.h>

struct syscall_number_location_t {
  int32_t number;

  uint32_t start;
  uint32_t end;

  struct syscall_number_location_t *next;
};

#endif
