#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include "../include/inline-asm.h"
#include "../libstm32f042k6/include/stm32f042k6.h"

#include "../include/usart_function.h"


//
static void init(void);

int main(void) {
  init();

  while(1) {
    NOP();
  }

  return 0;
}


//
static void init(void) {
  FLASH->ACR &= ~FLASH_ACR_LATENCY;

  RCC->CR |= RCC_CR_HSION;
  while((RCC->CR & RCC_CR_HSIRDY) == 0) {
    NOP();
  }


  RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_HSI;
  while((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSI) {
    NOP();
  }


  // GPIOA, GPIOB 有効化
  RCC->AHBENR  |= RCC_AHBENR_IOPAEN | RCC_AHBENR_IOPBEN;
  // USART1, USART2 有効化
  RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
  RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

  RCC->CFGR3 = (RCC->CFGR3 & ~(RCC_CFGR3_USART1SW | RCC_CFGR3_USART2SW))
    | RCC_CFGR3_USART1SW_SYSCLK
    | RCC_CFGR3_USART2SW_SYSCLK;

  // LED
  GPIO_SET_MODE(GPIO_B, 3, GPIO_MODER_OUTPUT);
  // ボタン入力 - 1
  GPIO_SET_MODE(GPIO_B, 0, GPIO_MODER_INPUT);
  // ボタン入力 - 2
  GPIO_SET_MODE(GPIO_B, 7, GPIO_MODER_INPUT);

  // 確認
  GPIO_SET_MODE(GPIO_B, 6, GPIO_MODER_OUTPUT);

  // ボタン入力 - 1 プルアップ
  GPIO_SET_PUPD(GPIO_B, 0, GPIO_PUPDR_PULL_UP);
  // ボタン入力 - 2 プルアップ
  GPIO_SET_PUPD(GPIO_B, 7, GPIO_PUPDR_PULL_UP);


  // PA9 - USART1 Tx
  GPIO_SET_MODE(GPIO_A, 9, GPIO_MODER_ALTERNATE_FUNCTION);
  // PA10 - USART1 Rx
  GPIO_SET_MODE(GPIO_A, 10, GPIO_MODER_ALTERNATE_FUNCTION);
  GPIO_A->AFRH = (GPIO_A->AFRH & ~(GPIO_AFRH_AFSEL9 | GPIO_AFRH_AFSEL10))
    | (GPIO_AFRH_AFSEL_AF1 << 4)
    | (GPIO_AFRH_AFSEL_AF1 << 8);

  // PA2 - USART2 Tx
  GPIO_SET_MODE(GPIO_A, 2, GPIO_MODER_ALTERNATE_FUNCTION);
  // PA3 - USART2 Rx
  GPIO_SET_MODE(GPIO_A, 3, GPIO_MODER_ALTERNATE_FUNCTION);
  GPIO_A->AFRL = (GPIO_A->AFRL & ~(GPIO_AFRL_AFSEL2 | GPIO_AFRL_AFSEL3))
    | (GPIO_AFRL_AFSEL_AF1 << 8)
    | (GPIO_AFRL_AFSEL_AF1 << 12);


  // USART1
  USART1->CR1 = (USART1->CR1 & ~(USART_CR1_M0 | USART_CR1_M1 | USART_CR1_PCE))
    | USART_CR1_RE
    | USART_CR1_TE;
  USART1->CR2 = (USART1->CR2 & ~(USART_CR2_STOP));

  // 割り込みの有効化
  USART1->CR1 |= USART_CR1_TCIE | USART_CR1_RXNEIE;

  // Baudrate 57600
  USART1->BRR = 0x008b;
  USART1->CR1 |= USART_CR1_UE;


  // USART2
  USART2->CR1 = (USART2->CR1 & ~(USART_CR1_M0 | USART_CR1_M1 | USART_CR1_PCE))
    | USART_CR1_RE
    | USART_CR1_TE;
  USART2->CR2 = (USART2->CR2 & ~(USART_CR2_STOP));

  // 割り込みの有効化
  USART2->CR1 |= USART_CR1_TCIE | USART_CR1_RXNEIE;

  // Baudrate 57600
  USART2->BRR = 0x008b;
  USART2->CR1 |= USART_CR1_UE;



  // SysTick の設定と開始
  SYSTICK->CSR = STK_CSR_CLKSOURCE | STK_CSR_TICKINT;
#if 1
  SYSTICK->RVR = 0x007a11ff;
#else
  SYSTICK->RVR = 0x007a11ff / 8;
#endif
  SYSTICK->CVR = 0;

  SYSTICK->CSR |= STK_CSR_ENABLE;


  Enable_NVIC(USART1_IRQn);
  Enable_NVIC(USART2_IRQn);
}
