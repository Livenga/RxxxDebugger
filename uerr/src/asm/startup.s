.cpu cortex-m0
.thumb


.global reset_handler
.global irq_vector


.section .text.reset_handler
.weak reset_handler
.type reset_handler, %function


reset_handler:
  bl __init_arm
  bl main


default_handler:
inf_loop:
  bl inf_loop


.section .vector, "a", %progbits
.type irq_vector, %object
.size irq_vector, .-irq_vector

irq_vector:
 .word estack                      // 0x0000     Reserved
 .word reset_handler               // 0x0004 -3  Reset
 .word NMI_handler                 // 0x0008 -2  NMI
 .word HardFault_handler           // 0x000c -1  Hardfault
 .word 0                           // 0x0010
 .word 0                           // 0x0014
 .word 0                           // 0x0018
 .word 0                           // 0x001c
 .word 0                           // 0x0020
 .word 0                           // 0x0024
 .word 0                           // 0x0028
 .word SVCall_handler              // 0x002c   3 SVCall
 .word 0                           // 0x0030
 .word 0                           // 0x0034
 .word PendSV_handler              // 0x0038   5 PendSV
 .word SysTick_handler             // 0x003c   6 SysTick
 .word WWDG_handler                // 0x0040   7 WWDG
 .word PVD_VDDIO2_handler          // 0x0044   8 PVD_VDDIO2
 .word RTC_handler                 // 0x0048   9 RTC
 .word FLASH_handler               // 0x004c  10 FLASH global interrupt
 .word RCC_CRS_handler             // 0x0050  11 RCC CRS
 .word EXTI0_1_handler             // 0x0054  12 EXTI Line[1:0] interrupts
 .word EXTI2_3_handler             // 0x0058  13 EXTI Line[3:2] interrupts
 .word EXTI4_15_handler            // 0x005c  14 EXTI Line[15:4] interrupts
 .word TSC_handler                 // 0x0060  15 Touch sensing interrupt
 .word DMA_CH1_handler             // 0x0064  16 DMA channel 1 interrupt
 .word DMA_CH2_3_handler           // 0x0068  17 DMA channel 2 and 3 interrupts, DMA2 channel 1 and 2 interrupts
 .word DMA_CH_4_5_6_7_handler      // 0x006c  18 DMA channel 4, 5, 6, and 7 interrupts, DMA2 channel 3, 4 and 5 interrupts
 .word ADC_COMP_handler            // 0x0070  19 ADC and COMP interrupts
 .word TIM1_BRK_UP_TRG_COM_handler // 0x0074  20 TIM1 break, update, trigger and commutation interrupt
 .word TIM1_CC_handler             // 0x0078  21 TIM1 capture compare interrupt
 .word TIM2_handler                // 0x007c  22 TIM2 global interrupt
 .word TIM3_handler                // 0x0080  23 TIM3 global interrupt
 .word TIM6_DAC_handler            // 0x0084  24 TIM6 global interrupt and DAC underrun interrupt
 .word TIM7_handler                // 0x0088  25 TIM7 global interrupt
 .word TIM14_handler               // 0x008c  26 TIM14 global interrupt
 .word TIM15_handler               // 0x0090  27 TIM15 global interrupt
 .word TIM16_handler               // 0x0094  28 TIM16 global interrupt
 .word TIM17_handler               // 0x0098  29 TIM17 global interrupt
 .word I2C1_handler                // 0x009c  30 I2C1 global interrupt
 .word I2C2_handler                // 0x00a0  31 I2C2 global interrupt
 .word SPI1_handler                // 0x00a4  32 SPI1 global interrupt
 .word SPI2_handler                // 0x00a8  33 SPI2 global interrupt
 .word USART1_handler              // 0x00ac  34 USART1 global interrupt
 .word USART2_handler              // 0x00b0  35 USART2 global interrupt
 .word USART3_4_5_6_7_8_handler    // 0x00b4  36 USART3, 4, 5, 6, 7 and 8 global interupts
 .word CEC_CAN_handler             // 0x00b8  37 CEC and CAN global interrupts
 .word USB_handler                 // 0x00bc  38 USB global interrupt


.weak NMI_handler
.thumb_set NMI_handler, default_handler

.weak HardFault_handler
.thumb_set HardFault_handler, default_handler

.weak SVCall_handler
.thumb_set SVCall_handler, default_handler

.weak PendSV_handler
.thumb_set PendSV_handler, default_handler

.weak SysTick_handler
.thumb_set SysTick_handler, default_handler

.weak WWDG_handler
.thumb_set WWDG_handler, default_handler

.weak PVD_VDDIO2_handler
.thumb_set PVD_VDDIO2_handler, default_handler

.weak RTC_handler
.thumb_set RTC_handler, default_handler

.weak FLASH_handler
.thumb_set FLASH_handler, default_handler

.weak RCC_CRS_handler
.thumb_set RCC_CRS_handler, default_handler

.weak EXTI0_1_handler
.thumb_set EXTI0_1_handler, default_handler

.weak EXTI2_3_handler
.thumb_set EXTI2_3_handler, default_handler

.weak EXTI4_15_handler
.thumb_set EXTI4_15_handler, default_handler

.weak TSC_handler
.thumb_set TSC_handler, default_handler

.weak DMA_CH1_handler
.thumb_set DMA_CH1_handler, default_handler

.weak DMA_CH2_3_handler
.thumb_set DMA_CH2_3_handler, default_handler

.weak DMA_CH_4_5_6_7_handler
.thumb_set DMA_CH_4_5_6_7_handler, default_handler

.weak ADC_COMP_handler
.thumb_set ADC_COMP_handler, default_handler

.weak TIM1_BRK_UP_TRG_COM_handler
.thumb_set TIM1_BRK_UP_TRG_COM_handler, default_handler

.weak TIM1_CC_handler
.thumb_set TIM1_CC_handler, default_handler

.weak TIM2_handler
.thumb_set TIM2_handler, default_handler

.weak TIM3_handler
.thumb_set TIM3_handler, default_handler

.weak TIM6_DAC_handler
.thumb_set TIM6_DAC_handler, default_handler

.weak TIM7_handler
.thumb_set TIM7_handler, default_handler

.weak TIM14_handler
.thumb_set TIM14_handler, default_handler

.weak TIM15_handler
.thumb_set TIM15_handler, default_handler

.weak TIM16_handler
.thumb_set TIM16_handler, default_handler

.weak TIM17_handler
.thumb_set TIM17_handler, default_handler

.weak I2C1_handler
.thumb_set I2C1_handler, default_handler

.weak I2C2_handler
.thumb_set I2C2_handler, default_handler

.weak SPI1_handler
.thumb_set SPI1_handler, default_handler

.weak SPI2_handler
.thumb_set SPI2_handler, default_handler

.weak USART1_handler
.thumb_set USART1_handler, default_handler

.weak USART2_handler
.thumb_set USART2_handler, default_handler

.weak USART3_4_5_6_7_8_handler
.thumb_set USART3_4_5_6_7_8_handler, default_handler

.weak CEC_CAN_handler
.thumb_set CEC_CAN_handler, default_handler

.weak USB_handler
.thumb_set USB_handler, default_handler
