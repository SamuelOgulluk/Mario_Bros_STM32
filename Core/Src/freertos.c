/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "player.h"
#include "app_ui.h"
#include "app_audio.h"
#include "freertos_app.h"
#include "lvgl.h"

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
/* USER CODE BEGIN Variables */
SemaphoreHandle_t g_adc1_mutex;
/* USER CODE END Variables */
osThreadId defaultTaskHandle;
osThreadId audioTaskHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void const * argument);
void StartAudioTask(void const * argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
  *ppxIdleTaskStackBuffer = &xIdleStack[0];
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
  /* place for user code */
}
/* USER CODE END GET_IDLE_TASK_MEMORY */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  g_adc1_mutex = xSemaphoreCreateMutex();
  if(g_adc1_mutex == NULL) {
    Error_Handler();
  }

  /* Create the thread(s) */
  /* definition and creation of audioTask */
  osThreadDef(audioTask, StartAudioTask, osPriorityBelowNormal, 0, AUDIO_TASK_STACK_WORDS);
  audioTaskHandle = osThreadCreate(osThread(audioTask), NULL);
  if(audioTaskHandle == NULL) {
    Error_Handler();
  }

  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, DEFAULT_TASK_STACK_WORDS);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);
  if(defaultTaskHandle == NULL) {
    Error_Handler();
  }

  /* USER CODE BEGIN RTOS_THREADS */
  /* audio runs in a dedicated task; gameplay stays in defaultTask for LVGL safety */
  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_StartAudioTask */
/**
  * @brief  Function implementing the audioTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartAudioTask */
void StartAudioTask(void const * argument)
{
  /* USER CODE BEGIN StartAudioTask */
  AudioTaskState_t st;

  (void) argument;
  st = (AudioTaskState_t){
    .audio_retry_tick = HAL_GetTick()
  };

  for(;;)
  {
    if((AppAudio_IsRunning() == 0U) && ((int32_t)(HAL_GetTick() - st.audio_retry_tick) >= 0)) {
      (void)AppAudio_StartFromFile("0:/son/theme.wav");
      st.audio_retry_tick = HAL_GetTick() + 1000U;
    }

    AppAudio_Process();

    vTaskDelay(pdMS_TO_TICKS(AUDIO_TASK_PERIOD_MS));
  }
  /* USER CODE END StartAudioTask */
}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void const * argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  DefaultTaskState_t st;

  (void) argument;
  Player_Init();
  st = (DefaultTaskState_t){
  };

  /* Infinite loop */
  for(;;)
  {
    st.control_tick_count++;
    if(st.control_tick_count >= CONTROL_UPDATE_LOOP_DIV) {
      st.control_tick_count = 0U;
      Player_Update();

      st.debug_tick_count++;
      if(st.debug_tick_count >= DEBUG_UPDATE_LOOP_DIV) {
        PlayerDebug debug;

        Player_GetDebug(&debug);
        UI_SetDebugInput(debug.dx,
                         debug.vy,
                         debug.jump,
                         debug.axis_raw[0],
                         debug.axis_raw[1],
                         debug.axis_raw[2],
                         debug.axis_raw[3]);
        st.debug_tick_count = 0U;
      }
    }

    lv_tick_inc(MAIN_LOOP_PERIOD_MS);
    st.lvgl_tick_count++;
    if(st.lvgl_tick_count >= LVGL_UPDATE_LOOP_DIV) {
      st.lvgl_tick_count = 0U;
      lv_timer_handler();
    }

    st.led_tick_count++;
    if(st.led_tick_count >= LED_TOGGLE_LOOP_DIV) {
      HAL_GPIO_TogglePin(LED14_GPIO_Port, LED14_Pin);
      HAL_GPIO_TogglePin(LED16_GPIO_Port, LED16_Pin);
      st.led_tick_count = 0U;
    }

    vTaskDelay(pdMS_TO_TICKS(MAIN_LOOP_PERIOD_MS));
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
/* USER CODE END Application */

