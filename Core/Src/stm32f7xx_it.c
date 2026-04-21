/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32f7xx_it.c
  * @brief   Interrupt Service Routines.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f7xx_it.h"
#include "cmsis_os.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stm32746g_discovery_sd.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
volatile HardFaultContext_t g_hardfault_ctx;
static volatile uint32_t g_hardfault_bkpt_latched = 0U;
static volatile uint32_t g_hardfault_count = 0U;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */
static uint32_t HardFault_IsValidStackPointer(const uint32_t * stack_ptr);
static void HardFault_SaveContext(uint32_t * stack_ptr, uint32_t exc_return);
__attribute__((used, noinline)) static void HardFault_HandlerC(uint32_t * stack_ptr, uint32_t exc_return);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

static uint32_t HardFault_IsValidStackPointer(const uint32_t * stack_ptr)
{
  uint32_t sp;

  if(stack_ptr == NULL) {
    return 0U;
  }

  sp = (uint32_t)stack_ptr;

  if((sp & 0x3U) != 0U) {
    return 0U;
  }

  if((sp >= 0x20000000U) && (sp <= (0x20050000U - 32U))) {
    return 1U;
  }

  return 0U;
}

static void HardFault_SaveContext(uint32_t * stack_ptr, uint32_t exc_return)
{
  g_hardfault_ctx.exc_return = exc_return;
  g_hardfault_ctx.active_sp = (uint32_t)stack_ptr;
  g_hardfault_ctx.msp = __get_MSP();
  g_hardfault_ctx.psp = __get_PSP();
  g_hardfault_ctx.stack_frame_valid = HardFault_IsValidStackPointer(stack_ptr);

  if(g_hardfault_ctx.stack_frame_valid != 0U) {
    g_hardfault_ctx.r0 = stack_ptr[0];
    g_hardfault_ctx.r1 = stack_ptr[1];
    g_hardfault_ctx.r2 = stack_ptr[2];
    g_hardfault_ctx.r3 = stack_ptr[3];
    g_hardfault_ctx.r12 = stack_ptr[4];
    g_hardfault_ctx.lr = stack_ptr[5];
    g_hardfault_ctx.pc = stack_ptr[6];
    g_hardfault_ctx.psr = stack_ptr[7];
  }

  g_hardfault_ctx.cfsr = SCB->CFSR;
  g_hardfault_ctx.hfsr = SCB->HFSR;
  g_hardfault_ctx.dfsr = SCB->DFSR;
  g_hardfault_ctx.afsr = SCB->AFSR;
  g_hardfault_ctx.bfar = SCB->BFAR;
  g_hardfault_ctx.mmfar = SCB->MMFAR;
  g_hardfault_ctx.shcsr = SCB->SHCSR;
  g_hardfault_ctx.icsr = SCB->ICSR;
}

__attribute__((used, noinline)) static void HardFault_HandlerC(uint32_t * stack_ptr, uint32_t exc_return)
{
  g_hardfault_count++;
  HardFault_SaveContext(stack_ptr, exc_return);

  if(((CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk) != 0U) &&
     (g_hardfault_bkpt_latched == 0U)) {
    g_hardfault_bkpt_latched = 1U;
    __BKPT(0);
  }

  while(1)
  {
  }
}

/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/
extern DAC_HandleTypeDef hdac;
extern DMA_HandleTypeDef hdma_dac1;
extern DMA2D_HandleTypeDef hdma2d;
extern LTDC_HandleTypeDef hltdc;
extern UART_HandleTypeDef huart1;
extern TIM_HandleTypeDef htim6;

/* USER CODE BEGIN EV */

/* USER CODE END EV */

/******************************************************************************/
/*           Cortex-M7 Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void)
{
  /* USER CODE BEGIN NonMaskableInt_IRQn 0 */

  /* USER CODE END NonMaskableInt_IRQn 0 */
  /* USER CODE BEGIN NonMaskableInt_IRQn 1 */
  while (1)
  {
  }
  /* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
  * @brief This function handles Hard fault interrupt.
  */
void HardFault_Handler(void)
{
  /* USER CODE BEGIN HardFault_IRQn 0 */

  __asm volatile
  (
    "mov r1, lr                        \n"
    "tst lr, #4                        \n"
    "ite eq                            \n"
    "mrseq r0, msp                     \n"
    "mrsne r0, psp                     \n"
    "b HardFault_HandlerC              \n"
  );

  /* USER CODE END HardFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_HardFault_IRQn 0 */
    /* USER CODE END W1_HardFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Memory management fault.
  */
void MemManage_Handler(void)
{
  /* USER CODE BEGIN MemoryManagement_IRQn 0 */

  /* USER CODE END MemoryManagement_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_MemoryManagement_IRQn 0 */
    /* USER CODE END W1_MemoryManagement_IRQn 0 */
  }
}

/**
  * @brief This function handles Pre-fetch fault, memory access fault.
  */
void BusFault_Handler(void)
{
  /* USER CODE BEGIN BusFault_IRQn 0 */

  /* USER CODE END BusFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_BusFault_IRQn 0 */
    /* USER CODE END W1_BusFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Undefined instruction or illegal state.
  */
void UsageFault_Handler(void)
{
  /* USER CODE BEGIN UsageFault_IRQn 0 */

  /* USER CODE END UsageFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_UsageFault_IRQn 0 */
    /* USER CODE END W1_UsageFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Debug monitor.
  */
void DebugMon_Handler(void)
{
  /* USER CODE BEGIN DebugMonitor_IRQn 0 */

  /* USER CODE END DebugMonitor_IRQn 0 */
  /* USER CODE BEGIN DebugMonitor_IRQn 1 */

  /* USER CODE END DebugMonitor_IRQn 1 */
}

/**
  * @brief This function handles SysTick interrupt.
  */
void SysTick_Handler(void)
{
  HAL_IncTick();
  osSystickHandler();
}

/******************************************************************************/
/* STM32F7xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32f7xx.s).                    */
/******************************************************************************/

/**
  * @brief This function handles USART1 global interrupt.
  */
void USART1_IRQHandler(void)
{
  /* USER CODE BEGIN USART1_IRQn 0 */

  /* USER CODE END USART1_IRQn 0 */
  HAL_UART_IRQHandler(&huart1);
  /* USER CODE BEGIN USART1_IRQn 1 */

  /* USER CODE END USART1_IRQn 1 */
}

/**
  * @brief This function handles TIM6 global interrupt, DAC1 and DAC2 underrun error interrupts.
  */
void TIM6_DAC_IRQHandler(void)
{
  /* USER CODE BEGIN TIM6_DAC_IRQn 0 */

  /* USER CODE END TIM6_DAC_IRQn 0 */
  if (hdac.State != HAL_DAC_STATE_RESET) {
    HAL_DAC_IRQHandler(&hdac);
  }
  HAL_TIM_IRQHandler(&htim6);
  /* USER CODE BEGIN TIM6_DAC_IRQn 1 */

  /* USER CODE END TIM6_DAC_IRQn 1 */
}

/**
  * @brief This function handles DMA1 stream5 global interrupt.
  */
void DMA1_Stream5_IRQHandler(void)
{
  HAL_DMA_IRQHandler(&hdma_dac1);
}

/**
  * @brief This function handles LTDC global interrupt.
  */
void LTDC_IRQHandler(void)
{
  /* USER CODE BEGIN LTDC_IRQn 0 */

  /* USER CODE END LTDC_IRQn 0 */
  HAL_LTDC_IRQHandler(&hltdc);
  /* USER CODE BEGIN LTDC_IRQn 1 */

  /* USER CODE END LTDC_IRQn 1 */
}

/**
  * @brief This function handles DMA2D global interrupt.
  */
void DMA2D_IRQHandler(void)
{
  /* USER CODE BEGIN DMA2D_IRQn 0 */

  /* USER CODE END DMA2D_IRQn 0 */
  HAL_DMA2D_IRQHandler(&hdma2d);
  /* USER CODE BEGIN DMA2D_IRQn 1 */

  /* USER CODE END DMA2D_IRQn 1 */
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
