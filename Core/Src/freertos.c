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
/* USER CODE END Variables */
osThreadId defaultTaskHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void const * argument);

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

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, DEFAULT_TASK_STACK_WORDS);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* input/game loop runs in defaultTask */
  /* USER CODE END RTOS_THREADS */

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
    .next_update_tick = HAL_GetTick()
  };
  st.audio_retry_tick = st.next_update_tick + 1000U;

  /* Infinite loop */
  for(;;)
  {
    if((AppAudio_IsRunning() == 0U) && ((int32_t)(HAL_GetTick() - st.audio_retry_tick) >= 0)) {
      (void)AppAudio_StartFromFile("0:/son/theme.wav");
      st.audio_retry_tick = HAL_GetTick() + 1000U;
    }

    AppAudio_Process();

    st.control_tick_count++;
    if(st.control_tick_count >= CONTROL_UPDATE_LOOP_DIV) {
      st.control_tick_count = 0U;
      Player_Update();

      st.debug_tick_count++;
      if(st.debug_tick_count >= DEBUG_UPDATE_LOOP_DIV) {
        Player_GetDebug(&st.debug);
        UI_SetDebugInput(st.debug.dx,
                         st.debug.vy,
                         st.debug.jump,
                         st.debug.axis_raw[0],
                         st.debug.axis_raw[1],
                         st.debug.axis_raw[2],
                         st.debug.axis_raw[3]);
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

    while((int32_t)(HAL_GetTick() - st.next_update_tick) < (int32_t)MAIN_LOOP_PERIOD_MS)
    {
      taskYIELD();
    }
    st.next_update_tick += MAIN_LOOP_PERIOD_MS;
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
/* USER CODE END Application */

