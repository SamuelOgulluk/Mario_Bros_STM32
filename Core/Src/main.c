/* USER CODE BEGIN Header */
/*
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "cmsis_os.h"
#include "adc.h"
#include "dma2d.h"
#include "fmc.h"
#include "gpio.h"
#include "i2c.h"
#include "ltdc.h"
#include "tim.h"
#include "dac.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "app_ui.h"
#include "app_audio.h"
#include "main_app.h"
#include "stm32746g_discovery_sdram.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
static const AppUI_LevelBlock g_level_blocks[] = {
  /* Zone 1: opening */
  {0, 8, APP_UI_BLOCK_GROUND}, {1, 8, APP_UI_BLOCK_GROUND}, {2, 8, APP_UI_BLOCK_GROUND},
  {3, 8, APP_UI_BLOCK_GROUND}, {4, 8, APP_UI_BLOCK_GROUND}, {5, 8, APP_UI_BLOCK_GROUND},
  {6, 8, APP_UI_BLOCK_GROUND}, {7, 8, APP_UI_BLOCK_GROUND}, {8, 8, APP_UI_BLOCK_GROUND},
  {3, 6, APP_UI_BLOCK_NORMAL}, {4, 6, APP_UI_BLOCK_QUESTION}, {5, 6, APP_UI_BLOCK_NORMAL},

  /* Zone 2: first gaps and elevated route */
  {13, 8, APP_UI_BLOCK_GROUND}, {14, 8, APP_UI_BLOCK_GROUND}, {15, 8, APP_UI_BLOCK_GROUND},
  {16, 8, APP_UI_BLOCK_GROUND}, {17, 8, APP_UI_BLOCK_GROUND}, {18, 8, APP_UI_BLOCK_GROUND},
  {14, 6, APP_UI_BLOCK_NORMAL}, {15, 6, APP_UI_BLOCK_BROKEN}, {16, 6, APP_UI_BLOCK_NORMAL},
  {18, 5, APP_UI_BLOCK_QUESTION}, {19, 5, APP_UI_BLOCK_NORMAL},

  /* Zone 3: platforms and pipe pair */
  {22, 7, APP_UI_BLOCK_GROUND}, {23, 7, APP_UI_BLOCK_GROUND}, {24, 7, APP_UI_BLOCK_GROUND},
  {25, 7, APP_UI_BLOCK_GROUND}, {26, 7, APP_UI_BLOCK_GROUND},
  {24, 5, APP_UI_BLOCK_QUESTION}, {25, 5, APP_UI_BLOCK_NORMAL},
  {27, 8, APP_UI_BLOCK_PIPE}, {27, 7, APP_UI_BLOCK_PIPE},
  {29, 8, APP_UI_BLOCK_PIPE}, {29, 7, APP_UI_BLOCK_PIPE},

  /* Zone 4: dense middle section */
  {31, 8, APP_UI_BLOCK_GROUND}, {32, 8, APP_UI_BLOCK_GROUND}, {33, 8, APP_UI_BLOCK_GROUND},
  {34, 8, APP_UI_BLOCK_GROUND}, {35, 8, APP_UI_BLOCK_GROUND}, {36, 8, APP_UI_BLOCK_GROUND},
  {37, 8, APP_UI_BLOCK_GROUND}, {38, 8, APP_UI_BLOCK_GROUND},
  {32, 6, APP_UI_BLOCK_NORMAL}, {33, 6, APP_UI_BLOCK_QUESTION}, {34, 6, APP_UI_BLOCK_NORMAL},
  {36, 5, APP_UI_BLOCK_NORMAL}, {37, 5, APP_UI_BLOCK_BROKEN}, {38, 5, APP_UI_BLOCK_NORMAL},
  {35, 8, APP_UI_BLOCK_GOOMBA},

  /* Zone 5: stair and long jump */
  {41, 8, APP_UI_BLOCK_GROUND}, {42, 7, APP_UI_BLOCK_GROUND}, {43, 6, APP_UI_BLOCK_GROUND},
  {44, 5, APP_UI_BLOCK_GROUND}, {45, 4, APP_UI_BLOCK_GROUND},
  {48, 8, APP_UI_BLOCK_GROUND}, {49, 8, APP_UI_BLOCK_GROUND}, {50, 8, APP_UI_BLOCK_GROUND},
  {51, 8, APP_UI_BLOCK_GROUND}, {52, 8, APP_UI_BLOCK_GROUND},
  {49, 6, APP_UI_BLOCK_QUESTION}, {51, 6, APP_UI_BLOCK_QUESTION},

  /* Zone 6: split path */
  {55, 8, APP_UI_BLOCK_GROUND}, {56, 8, APP_UI_BLOCK_GROUND}, {57, 8, APP_UI_BLOCK_GROUND},
  {58, 8, APP_UI_BLOCK_GROUND}, {59, 8, APP_UI_BLOCK_GROUND}, {60, 8, APP_UI_BLOCK_GROUND},
  {61, 8, APP_UI_BLOCK_GROUND},
  {56, 5, APP_UI_BLOCK_NORMAL}, {57, 5, APP_UI_BLOCK_NORMAL}, {58, 5, APP_UI_BLOCK_QUESTION},
  {59, 5, APP_UI_BLOCK_NORMAL}, {60, 5, APP_UI_BLOCK_NORMAL},
  {61, 8, APP_UI_BLOCK_GOOMBA},

  /* Zone 7: pipe corridor */
  {64, 8, APP_UI_BLOCK_PIPE}, {64, 7, APP_UI_BLOCK_PIPE},
  {66, 8, APP_UI_BLOCK_GROUND}, {67, 8, APP_UI_BLOCK_GROUND}, {68, 8, APP_UI_BLOCK_GROUND},
  {69, 8, APP_UI_BLOCK_GROUND}, {70, 8, APP_UI_BLOCK_GROUND},
  {71, 8, APP_UI_BLOCK_PIPE}, {71, 7, APP_UI_BLOCK_PIPE},
  {73, 8, APP_UI_BLOCK_GROUND}, {74, 8, APP_UI_BLOCK_GROUND}, {75, 8, APP_UI_BLOCK_GROUND},

  /* Zone 8: upper challenge */
  {76, 6, APP_UI_BLOCK_NORMAL}, {77, 6, APP_UI_BLOCK_BROKEN}, {78, 6, APP_UI_BLOCK_NORMAL},
  {79, 6, APP_UI_BLOCK_QUESTION}, {80, 6, APP_UI_BLOCK_NORMAL},
  {82, 5, APP_UI_BLOCK_NORMAL}, {83, 5, APP_UI_BLOCK_QUESTION}, {84, 5, APP_UI_BLOCK_NORMAL},

  /* Zone 9: long progression */
  {86, 8, APP_UI_BLOCK_GROUND}, {87, 8, APP_UI_BLOCK_GROUND}, {88, 8, APP_UI_BLOCK_GROUND},
  {89, 8, APP_UI_BLOCK_GROUND}, {90, 8, APP_UI_BLOCK_GROUND}, {91, 8, APP_UI_BLOCK_GROUND},
  {92, 8, APP_UI_BLOCK_GROUND}, {93, 8, APP_UI_BLOCK_GROUND},
  {90, 6, APP_UI_BLOCK_NORMAL}, {91, 6, APP_UI_BLOCK_NORMAL}, {92, 6, APP_UI_BLOCK_QUESTION},
  {93, 6, APP_UI_BLOCK_NORMAL},
  {93, 8, APP_UI_BLOCK_GOOMBA},

  /* Zone 10: finale avec escalier vers le drapeau/forteresse */
  {96, 8, APP_UI_BLOCK_GROUND}, {97, 7, APP_UI_BLOCK_GROUND}, {98, 6, APP_UI_BLOCK_GROUND},
  {99, 5, APP_UI_BLOCK_GROUND}, {100, 4, APP_UI_BLOCK_GROUND},

  {102, 8, APP_UI_BLOCK_GROUND}, {103, 8, APP_UI_BLOCK_GROUND}, {104, 8, APP_UI_BLOCK_GROUND},
  {105, 8, APP_UI_BLOCK_GROUND}, {106, 8, APP_UI_BLOCK_GROUND}, {107, 8, APP_UI_BLOCK_GROUND},
  {108, 8, APP_UI_BLOCK_GROUND}, {109, 8, APP_UI_BLOCK_GROUND}, {110, 8, APP_UI_BLOCK_GROUND},
  {111, 8, APP_UI_BLOCK_GROUND}, {112, 8, APP_UI_BLOCK_GROUND},

  /* Escalier montant */
  {109, 7, APP_UI_BLOCK_GROUND}, {110, 6, APP_UI_BLOCK_GROUND}, {111, 5, APP_UI_BLOCK_GROUND},
  {112, 4, APP_UI_BLOCK_GROUND}, {113, 3, APP_UI_BLOCK_GROUND}, {114, 3, APP_UI_BLOCK_GROUND},

  /* Petits bonus sur la derniere ligne droite */
  {104, 6, APP_UI_BLOCK_QUESTION}, {106, 6, APP_UI_BLOCK_NORMAL},
  {112, 5, APP_UI_BLOCK_QUESTION}, {114, 4, APP_UI_BLOCK_NORMAL},
  {112, 8, APP_UI_BLOCK_GOOMBA}
};

static const AppBootConfig_t g_boot_cfg = {
  .level_blocks = g_level_blocks,
  .level_block_count = (uint16_t)(sizeof(g_level_blocks) / sizeof(g_level_blocks[0]))
};
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MX_FREERTOS_Init(void);
/* USER CODE BEGIN PFP */
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_ADC1_Init();
  MX_ADC3_Init();
  MX_DMA2D_Init();
  MX_TIM2_Init();
  MX_DAC_Init();
  MX_FMC_Init();
  /* BSP_SDRAM_Init performs FMC SDRAM init + SDRAM command sequence. */
  if(BSP_SDRAM_Init() != SDRAM_OK) {
    Error_Handler();
  }

  MX_I2C3_Init();
  MX_LTDC_Init();
  /* USER CODE BEGIN 2 */
  AppUI_Init();
  AppUI_SetLevelBlocks(g_boot_cfg.level_blocks, g_boot_cfg.level_block_count);
  /* USER CODE END 2 */

  /* Call init function for freertos objects (in cmsis_os2.c) */
  MX_FREERTOS_Init();

  /* Start audio immediately to avoid task-start timing issues. */
  (void)AppAudio_StartFromFile("0:/son/theme.wav");

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    HAL_GPIO_TogglePin(LED14_GPIO_Port, LED14_Pin);
    HAL_GPIO_TogglePin(LED16_GPIO_Port, LED16_Pin);
    HAL_Delay(100U);
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 400;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Activate the Over-Drive mode
  */
  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_6) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM6 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM6)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
