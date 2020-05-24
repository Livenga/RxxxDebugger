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



//static uint32_t receive_counter = 0;

// USART1 は現在の接続の関係上, リーダ -> アンテナ基板
void USART1_handler(void) {
  if((USART1->ISR & USART_ISR_TC) == USART_ISR_TC) {
    USART1->ICR = USART_ICR_TCCF;
  } else if((USART1->ISR & USART_ISR_RXNE) == USART_ISR_RXNE) {
    const uint8_t recv = (uint8_t)(USART1->RDR & 0xff);

#ifdef ENABLE_RANDOM
    const uint8_t f_enable_random = ! ((GPIO_B->IDR >> 0) & 1);

#ifdef ENABLE_REJECT_NL
    // 改行を無視してランダム値を送信
    if(f_enable_random) {
      usart_putchar(USART1, (uint8_t)(rand() % 0xff));
    } else {
      usart_putchar(USART1, recv);
    }
#else
    // 改行を考慮に入れた場合.
    if(f_enable_random && recv != '\n') {
      usart_putchar(USART1, (uint8_t)(rand() % 0xff));
    } else {
      usart_putchar(USART1, recv);
    }
#endif

#else
    usart_putchar(USART1, recv);
#endif

pt_rqr:
    USART1->RQR = USART_RQR_RXFRQ;
  }
}
