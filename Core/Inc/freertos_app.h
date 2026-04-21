#ifndef FREERTOS_APP_H
#define FREERTOS_APP_H

#include "player.h"

#define MAIN_LOOP_PERIOD_MS 5U
#define CONTROL_UPDATE_LOOP_DIV (15U / MAIN_LOOP_PERIOD_MS)
#define LVGL_UPDATE_LOOP_DIV (15U / MAIN_LOOP_PERIOD_MS)
#define LED_TOGGLE_LOOP_DIV (500U / MAIN_LOOP_PERIOD_MS)
#define DEBUG_UPDATE_LOOP_DIV 5U
#define DEFAULT_TASK_STACK_WORDS 2048U

typedef struct {
	PlayerDebug debug;
	uint32_t led_tick_count;
	uint32_t control_tick_count;
	uint32_t lvgl_tick_count;
	uint32_t debug_tick_count;
	uint32_t next_update_tick;
	uint32_t audio_retry_tick;
} DefaultTaskState_t;

#endif /* FREERTOS_APP_H */
