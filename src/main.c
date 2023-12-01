//
// This file is part of the GNU ARM Eclipse distribution.
// Copyright (c) 2014 Liviu Ionescu.
//

// ----------------------------------------------------------------------------
// School: University of Victoria, Canada.
// Course: ECE 355 "Microprocessor-Based Systems".
// This is template code for Part 2 of Introductory Lab.
//
// See "system/include/cmsis/stm32f051x8.h" for register/bit definitions.
// See "system/src/cmsis/vectors_stm32f051x8.c" for handler declarations.
// ----------------------------------------------------------------------------
#include "../system/include/cmsis/cmsis_device.h"
#include "../system/include/diag/trace.h"
#include "../system/include/stm32f0-hal/stm32f0xx_hal_spi.h"

//#include "../provided_headers/stm32f051x8.h"

// TODO Consider imports, these have been commented out as repel doesn't
// recognise roots from my knowledge
/*
#include "cmsis/cmsis_device.h"
#include "diag/Trace.h"
#include "include/stm32f0-hal/stmf0xx_hal_spi.h"
*/
#include <stdio.h>
/*
░█▀▄░█▀▀░█▀█░█▀▄░░░█▄█░█▀▀
░█▀▄░█▀▀░█▀█░█░█░░░█░█░█▀▀
░▀░▀░▀▀▀░▀░▀░▀▀░░░░▀░▀░▀▀▀

Typical Run:
myGPIOA_Init();
myGPIOB_Init();

myTIM2_Init();
myTIM3_Init();
oled_config();
myEXTI_Init();
myADC_Init();
myDAC_Init();

Frequency Generator

Timer Frequency

 */
// Decomposing devices into own sections
// SPI and OLED Display, init and functions
#include "./devices/oled_implementation.h"

// ADC and DAC init

// Octcoupler Frequency Generator, init and functions

// Everything to include in main
// Interrupts and Polling Operations

// ----------------------------------------------------------------------------
//
// STM32F0 empty sample (trace via $(trace)).
//
// Trace support is enabled by adding the TRACE macro definition.
// By default the trace messages are forwarded to the $(trace) output,
// but can be rerouted to any device or completely suppressed, by
// changing the definitions required in system/src/diag/trace_impl.c
// (currently OS_USE_TRACE_ITM, OS_USE_TRACE_SEMIHOSTING_DEBUG/_STDOUT).
//

// ----- main() ---------------------------------------------------------------

// Sample pragmas to cope with warnings. Please note the related line at
// the end of this function, used to pop the compiler diagnostics status.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#pragma GCC diagnostic ignored "-Wreturn-type"

/* Definitions of registers and their bits are
   given in system/include/cmsis/stm32f051x8.h */

/* Clock prescaler for TIM2 timer: no prescaling */
#define myTIM2_PRESCALER ((uint16_t)0x0000)
/* Maximum possible setting for overflow */
#define myTIM2_PERIOD ((uint32_t)0xFFFFFFFF)

#define myTIM3_PRESCALER ((uint16_t)48000)
/* Maximum possible setting for overflow */
#define myTIM3_PERIOD ((uint32_t)0xFFFF)

void myGPIOA_Init(void);
void myGPIOB_Init(void);
void mySPI_Init(void);
void myTIM2_Init(void);
void myTIM3_Init(void);
void myEXTI_Init(void);
void myADC_Init(void);
void myDAC_Init(void);

void oled_Write(unsigned char);
void oled_Write_Cmd(unsigned char);
void oled_Write_Data(unsigned char);

void oled_config(void);
void refresh_OLED(float);

int Debug = 0;
// 1 = Function Generator, 0 = NE355 Timer
int inSig = 1;
float freq; // stores frequency
// Declare/initialize your global variables here...
// NOTE: You'll need at least one global variable
// (say, timerTriggered = 0 or 1) to indicate
// whether TIM2 has started counting or not.

/*** Call this function to boost the STM32F0xx clock to 48 MHz ***/

void SystemClock48MHz(void) {
  //
  // Disable the PLL
  //
  RCC->CR &= ~(RCC_CR_PLLON);
  //
  // Wait for the PLL to unlock
  //
  while ((RCC->CR & RCC_CR_PLLRDY) != 0)
    ;
  //
  // Configure the PLL for 48-MHz system clock
  //
  RCC->CFGR = 0x00280000;
  //
  // Enable the PLL
  //
  RCC->CR |= RCC_CR_PLLON;
  //
  // Wait for the PLL to lock
  //
  while ((RCC->CR & RCC_CR_PLLRDY) != RCC_CR_PLLRDY)
    ;
  //
  // Switch the processor to the PLL clock source
  //
  RCC->CFGR = (RCC->CFGR & (~RCC_CFGR_SW_Msk)) | RCC_CFGR_SW_PLL;
  //
  // Update the system with the new clock frequency
  //
  SystemCoreClockUpdate();
}

/*****************************************************************/

int main(int argc, char *argv[])

{

  SystemClock48MHz();

  trace_printf("This is the final part of the lab...\n");
  trace_printf("System clock: %u Hz\n", SystemCoreClock);
  myGPIOA_Init(); /* Initialize I/O port PA */
  myGPIOB_Init(); /* Initialize I/O port PB */
  myTIM2_Init();  /* Initialize timer TIM2 */
  myTIM3_Init();  /* Initialize timer TIM3 */
  oled_config();  /* Initialize OLED */
  myEXTI_Init();  /* Initialize EXTI (external interrupts)*/
  myADC_Init();   /* Initialize ADC */
  myDAC_Init();   /* Initialize DAC */

  while (1) {
    ADC1->CR |= 0b100; // start ADC (set start bit to 1)
    while ((ADC1->ISR & ADC_ISR_EOC) != 0x4)
      ; // wait until ADC conversion is done
    if (Debug) {
      trace_printf("%h", ADC1->DR);
    }
    DAC->DHR12R1 =
        ADC1->DR; // reads data from ADC to DAC, clears EOC flag of ADC
    // refresh_OLED(freq); // sends frequency to oled and refreshes it
  }

  return 0;
}

void myGPIOA_Init() {

  RCC->AHBENR |= RCC_AHBENR_GPIOAEN; // Enable GPIOA clock

  // Relevant register: GPIOA->MODER
  GPIOA->MODER |= 0b111100000000; // Configure PA0,1,2 as input, PA4,5 as analog

  /* Ensure no pull-up/pull-down for PA */
  // Relevant register: GPIOA->PUPDR
  GPIOA->PUPDR &= 0x0000;
}

void myGPIOB_Init() {
  /* Enable clock for GPIOB peripheral */
  // Relevant register: RCC->AHBENR
  RCC->AHBENR |= RCC_AHBENR_GPIOBEN;

  // Relevant register: GPIOB->MODER
  GPIOB->MODER |= 0b0101100110
                  << 0x6; // PB3 to be AF0, PB4 to be output, PB5 to be AF0, PB6
                          // to be output, PB7 to be output

  /* Ensure no pull-up/pull-down for PB */
  // Relevant register: GPIOB->PUPDR
  GPIOB->PUPDR &= 0x0000;
}

void myADC_Init() {
  RCC->APB2ENR |= RCC_APB2ENR_ADCEN; // Enable ADC clock

  ADC1->CR |= 0x01; // Enable ADC via ADEN

  // ADC1-> x For registers
  ADC1->SMPR |= 0x7; // max clock cycles
  // 1.221
  ADC1->CHSELR |= 0x20; // bit 5 = 1 to select from PA5

  ADC1->CFGR1 |=
      0b11 << 12; // set to continuous conversion mode on, register overwritten
                  // with last conversion when overrun detected

  while ((ADC1->ISR & ADC_ISR_ADRDY) != 0x1)
    ; // wait til ADC ready flag = 1
}

void myDAC_Init() {
  RCC->APB1ENR |= RCC_APB1ENR_DACEN; // Enable DAC clock
  DAC->CR = 0b001;                   // enables DAC
}

void myTIM2_Init() {
  /* Enable clock for TIM2 peripheral */
  // Relevant register: RCC->APB1ENR

  RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

  /* Configure TIM2: buffer auto-reload, count up, stop on overflow,
   * enable update events, interrupt on overflow only */
  // Relevant register: TIM2->CR1
  TIM2->CR1 = (int16_t)0x008C;

  /* Set clock prescaler value */
  TIM2->PSC = myTIM2_PRESCALER;
  /* Set auto-reloaded delay */
  TIM2->ARR = myTIM2_PERIOD;

  /* Update timer registers */
  // Relevant register: TIM2->EGR
  TIM2->EGR = ((uint16_t)0x0001);

  /* Assign TIM2 interrupt priority = 0 in NVIC */
  // Relevant register: NVIC->IP[3], or use NVIC_SetPriority
  NVIC_SetPriority(TIM2_IRQn, 0);

  /* Enable TIM2 interrupts in NVIC */
  // Relevant register: NVIC->ISER[0], or use NVIC_EnableIRQ
  NVIC_EnableIRQ(TIM2_IRQn);

  /* Enable update interrupt generation */
  // Relevant register: TIM2->DIER
  TIM2->DIER |= TIM_DIER_UIE;
}

void myTIM3_Init() {
  /* Enable clock for TIM3 peripheral */
  // Relevant register: RCC->APB1ENR
  RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;

  /* Configure TIM3: buffer auto-reload, count up, stop on overflow,
   * enable update events, interrupt on overflow only */
  // Relevant register: TIM2->CR1
  TIM3->CR1 = (int16_t)0x008C;

  /* Set clock prescaler value */
  TIM3->PSC = myTIM3_PRESCALER; // changed clock counts to ms to avoid overflow
  /* Set auto-reloaded delay */
  TIM3->ARR = myTIM3_PERIOD;

  /* Update timer registers */
  // Relevant register: TIM2->EGR
  TIM3->EGR = ((uint16_t)0x0001);
}

void myEXTI_Init() {
  /* Map EXTI2 line to PA2 */
  // Relevant register: SYSCFG->EXTICR[0]
  // Enable interrupts from PA
  SYSCFG->EXTICR[0] &= ~(0x000F); // user button and NE555 timer
  SYSCFG->EXTICR[0] &= ~(0x00F0); // function generation
  // SYSCFG->EXTICR[0] &= ~(0x0F00); // function generation

  /* EXTI2 line interrupts: set rising-edge trigger */
  // Relevant register: EXTI->RTSR
  // Enabling the rising trigger for input lines
  EXTI->RTSR |= 1; // button rising edge
  EXTI->RTSR |= 2; // NE555 rising edge
  EXTI->RTSR |= 4; // fucntion generation rising edge

  /* Unmask interrupts from EXTI2 line */
  // Relevant register: EXTI->IMR
  // Unmask interrupt requests from line 2
  EXTI->IMR |= 1;    // allow interrupt from user button
  EXTI->IMR &= ~(2); // masks NE355 timer
  EXTI->IMR |= 4;    // allow interrupts from function generator

  /* Assign EXTI interrupt priority = 0 in NVIC */
  // Relevant register: NVIC->IP[2], or use NVIC_SetPriority
  // Set interrupt priority for line 2 and 3 to 0
  NVIC_SetPriority(EXTI0_1_IRQn, 0);
  NVIC_SetPriority(EXTI2_3_IRQn, 0);

  /* Enable EXTI interrupts in NVIC */
  // Relevant register: NVIC->ISER[0], or use NVIC_EnableIRQ
  NVIC_EnableIRQ(EXTI0_1_IRQn);
  NVIC_EnableIRQ(EXTI2_3_IRQn);
}

/* This handler is declared in system/src/cmsis/vectors_stm32f051x8.c */
void TIM2_IRQHandler() {
  /* Check if update interrupt flag is indeed set */
  if ((TIM2->SR & TIM_SR_UIF) != 0) {
    trace_printf("\n*** Overflow! ***\n");

    /* Clear update interrupt flag */
    // Relevant register: TIM2->SR
    // Sets everything to zero
    TIM2->SR &= ~(TIM_SR_UIF);

    /* Restart stopped timer */
    // Relevant register: TIM2->CR1
    TIM2->CR1 |= TIM_CR1_CEN;
  }
}

uint32_t count = 0; // variable to track edges.

void EXTI0_1_IRQHandler() {

  if ((EXTI->PR & EXTI_PR_PR0) != 0) { // check if pending request is from PA0

    while ((GPIOA->IDR & 0x01) == 0x01)
      ; // while button is still pressed
    // If inSig 1 = Function Generator, inSig 0 = NE555 Timer

    EXTI->IMR ^=
        0x6; // switch interrupt mask -- btwn NE555 and function generator
    inSig = !inSig; // track state

    if (inSig) {
      trace_printf("\nButton! Reading Function Generator  %d\n", inSig);
    } else {
      trace_printf("\nButton! Reading NE555 Timer %d\n", inSig);
    }

    count = 0;       // reset count
    EXTI->PR = 0x01; // clear pending interrupt

  } else { // if here then interrupt is from PA1
    if (count == 0) {
      // First edge
      TIM2->CNT = 0x00;         // Clear Timer
      TIM2->CR1 |= TIM_CR1_CEN; // start timer
      count++;
    } else {
      // Second edge
      TIM2->CR1 &= ~(TIM_CR1_CEN);       // Stop Clock
      uint32_t time_elapsed = TIM2->CNT; // Reads count from clock

      float period = ((float)time_elapsed) / ((float)48000000);
      freq = ((float)1) / period;
      if (Debug) {
        trace_printf("*Time Elapsed(cycles): %u | Frequency(Hz): %f | "
                     "Period(s): %f | Resistance: %f\n",
                     time_elapsed, freq, period, ADC1->DR * 1.221);
      }
      refresh_OLED(freq);
      count = 0;
      TIM2->CNT = 0x00; // Clear Timer
    }

    EXTI->PR = 0x02; // Clear pending interrupt by writing 1
  }
}

/* This handler is declared in system/src/cmsis/vectors_stm32f051x8.c */
void EXTI2_3_IRQHandler() {

  /* Check if EXTI2 interrupt pending flag is indeed set */
  if ((EXTI->PR & EXTI_PR_PR2) != 0) {

    if (count == 0) {
      // First edge
      TIM2->CNT = 0x00;         // Clear Timer
      TIM2->CR1 |= TIM_CR1_CEN; // start timer
      count++;
    } else {
      // Second edge
      TIM2->CR1 &= ~(TIM_CR1_CEN);       // Stop Clock
      uint32_t time_elapsed = TIM2->CNT; // Reads count from clock

      float period = ((float)time_elapsed) / ((float)48000000);
      freq = ((float)1) / period;

      if (Debug) {
        trace_printf("Time Elapsed(cycles): %u | Frequency(Hz): %f | "
                     "Period(s): %f | Resistance: %f\n",
                     time_elapsed, freq, period, ADC1->DR * 1.221);
      }
      refresh_OLED(freq);
      count = 0;
      TIM2->CNT = 0x00;
    }
    EXTI->PR = 0x04;
  }
}

#pragma GCC diagnostic pop

// ----------------------------------------------------------------------------
