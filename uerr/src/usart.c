#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include "../include/inline-asm.h"
#include "../libstm32f042k6/include/stm32f042k6.h"



void
usart_putchar(
    usart_t *this,
    uint8_t chr) {
  this->TDR = chr;

  while((this->ISR & USART_ISR_TXE) == 0) {
    NOP();
  }
}


void
usart_print(
    usart_t *this,
    uint8_t *str) {
  while(*str) {
    usart_putchar(this, *(str++));
  }
}

void
usart_println(
    usart_t *this,
    uint8_t *str) {
  usart_print(this, str);
  usart_putchar(this, '\r');
  usart_putchar(this, '\n');
}
