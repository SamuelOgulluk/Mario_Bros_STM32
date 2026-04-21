#ifndef PLAYER_TYPES_H
#define PLAYER_TYPES_H

#include "main.h"
#include "adc.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct {
  ADC_HandleTypeDef * hadc;
  uint32_t channel;
  uint32_t center;
  uint32_t value;
} JoystickAxis_t;

typedef struct {
  JoystickAxis_t axis0;
  JoystickAxis_t axis1;
  JoystickAxis_t axis2;
  JoystickAxis_t axis3;
  GPIO_TypeDef * jump_button_port;
  uint16_t jump_button_pin;
  GPIO_PinState jump_button_idle_state;
  GPIO_TypeDef * sprint_button_port;
  uint16_t sprint_button_pin;
  GPIO_PinState sprint_button_idle_state;
} JoystickDevice_t;

typedef struct {
  int16_t dx;
  bool jump_pressed;
  bool sprint_pressed;
  uint32_t axis_raw[4];
  int32_t axis_delta[4];
} PlayerInput_t;

typedef struct {
  int32_t velocity_x_fp;
  int32_t velocity_y_fp;
  int32_t move_accum_x_fp;
  int32_t move_accum_y_fp;
  uint8_t coyote_ticks;
  uint8_t jump_buffer_ticks;
  bool jump_was_pressed;
  bool is_grounded;
} PlayerPhysics_t;

typedef struct {
  JoystickDevice_t joystick;
  PlayerInput_t input;
  PlayerPhysics_t physics;
  bool game_over_waiting_restart;
  bool level_complete_waiting_restart;
  uint8_t jump_button_debounce;
  uint8_t sprint_button_debounce;
  uint8_t anim_phase;
  int8_t x_direction;
} PlayerController_t;

#endif /* PLAYER_TYPES_H */
