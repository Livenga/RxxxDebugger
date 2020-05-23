#include <stdio.h>
#include <stdint.h>

void __init_arm(void) {
  uint8_t *p_cursor = NULL, *p_src = NULL;

  extern uint8_t *__data_start__,
         *__data_end__,
         *_etext;

  extern uint8_t *__bss_start__,
         *__bss_end__;

  for(p_cursor = (uint8_t*)&__data_start__, p_src = (uint8_t *)&_etext;
      p_cursor < (uint8_t *)&__data_end__;
      ++p_cursor, ++p_src) {
    *p_cursor = *p_src;
  }

  for(p_cursor = (uint8_t *)&__bss_start__;
      p_cursor < (uint8_t *)&__bss_end__;
      ++p_cursor) {
    *p_cursor = '\0';
  }
}
