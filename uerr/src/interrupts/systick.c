#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "../../libstm32f042k6/include/stm32f042k6.h"
#include "../../include/usart_function.h"

static uint8_t f_flag = 0;


void SysTick_handler(void) {
  if(f_flag) {
    GPIO_ON(GPIO_B, 3);
  } else {
    GPIO_OFF(GPIO_B, 3);
  }

  f_flag ^= 1;
}
