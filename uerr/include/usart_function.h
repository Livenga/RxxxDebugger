#ifndef _USART_FUNCTION_H
#define _USART_FUNCTION_H

#include <stdint.h>
#include "../libstm32f042k6/include/stm32f042k6.h"


/* src/usart.c */
extern void
usart_putchar(
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


#endif
