#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "../../include/inline-asm.h"
#include "../../libstm32f042k6/include/stm32f042k6.h"


extern void usart_putchar(
    usart_t *this,
    uint8_t chr);

extern void
usart_print(
    usart_t *this,
    uint8_t *str);

extern void
usart_println(
    usart_t *this,
    uint8_t *str);


void USART2_handler(void) {
  if((USART2->ISR & USART_ISR_TC) == USART_ISR_TC) {
    USART2->ICR = USART_ICR_TCCF;
  } else if((USART2->ISR & USART_ISR_RXNE) & USART_ISR_RXNE) {
    const uint8_t recv = (uint8_t)(USART2->RDR & 0xff);

#ifdef ENABLE_RANDOM
    // ボタン押下時に接地をさせているため, 結果を反転させる
    const uint8_t f_enable_random = ! ((GPIO_B->IDR >> 7) & 1);

#ifdef ENABLE_REJECT_NL
    if(f_enable_random) {
      usart_putchar(USART2, (uint8_t)(rand() % 0xff));
    } else {
      usart_putchar(USART2, recv);
    }
#else
    if(f_enable_random && recv != '\n') {
      usart_putchar(USART2, (uint8_t)(rand() % 0xff));
    } else {
      usart_putchar(USART2, recv);
    }
#endif

#else
    usart_putchar(USART2, recv);
#endif

pt_rqr:
    USART2->RQR = USART_RQR_RXFRQ;
  }
}
