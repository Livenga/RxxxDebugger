#ifndef _SFDUMP_TYPE_H
#define _SFDUMP_TYPE_H

#include <stdint.h>

struct sfdump_t {
  uint8_t  magic_number[2];
  uint16_t version;
  uint32_t register_size;
};


#endif
