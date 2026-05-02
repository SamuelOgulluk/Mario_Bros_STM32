#ifndef FREERTOS_APP_H
#define FREERTOS_APP_H

#include "player.h"
#include "FreeRTOS.h"
#include "semphr.h"

#define MAIN_LOOP_PERIOD_MS 5U
#define AUDIO_TASK_PERIOD_MS 5U
#define PLAYER_TASK_PERIOD_MS 10U
#define CONTROL_UPDATE_LOOP_DIV 1U
#define LVGL_UPDATE_LOOP_DIV 1U
#define LED_TOGGLE_LOOP_DIV (500U / MAIN_LOOP_PERIOD_MS)
#define DEBUG_UPDATE_LOOP_DIV 5U
#define DEFAULT_TASK_STACK_WORDS 2048U
#define AUDIO_TASK_STACK_WORDS 1536U
#define PLAYER_TASK_STACK_WORDS 1536U

typedef struct {
	uint32_t audio_retry_tick;
} AudioTaskState_t;

typedef struct {
	PlayerDebug debug;
	uint32_t control_tick_count;
	uint32_t debug_tick_count;
} PlayerTaskState_t;

typedef struct {
	PlayerDebug debug;
	uint32_t led_tick_count;
	uint32_t lvgl_tick_count;
	uint32_t control_tick_count;
	uint32_t debug_tick_count;
} DefaultTaskState_t;

extern SemaphoreHandle_t g_adc1_mutex;

#endif /* FREERTOS_APP_H */
