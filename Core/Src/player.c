#include "player.h"

#include "main.h"
#include "adc.h"
#include "app_ui_player_backend.h"
#include "freertos_app.h"
#include "player_types.h"

#include <stdbool.h>

static PlayerController_t g_player_controller;

static uint32_t Joystick_ReadAdcChannel(ADC_HandleTypeDef * hadc, uint32_t channel)
{
  ADC_ChannelConfTypeDef sConfig = {0};
  uint32_t value;
  BaseType_t locked;

  locked = pdTRUE;
  if(hadc == &hadc1) {
    locked = xSemaphoreTake(g_adc1_mutex, portMAX_DELAY);
    if(locked != pdTRUE) {
      return JOYSTICK_DEFAULT_CENTER;
    }
  }

  sConfig.Channel = channel;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_480CYCLES;

  if(HAL_ADC_ConfigChannel(hadc, &sConfig) != HAL_OK) {
    if(hadc == &hadc1) {
      (void)xSemaphoreGive(g_adc1_mutex);
    }
    return JOYSTICK_DEFAULT_CENTER;
  }

  if(HAL_ADC_Start(hadc) != HAL_OK) {
    if(hadc == &hadc1) {
      (void)xSemaphoreGive(g_adc1_mutex);
    }
    return JOYSTICK_DEFAULT_CENTER;
  }

  if(HAL_ADC_PollForConversion(hadc, 5U) != HAL_OK) {
    (void)HAL_ADC_Stop(hadc);
    if(hadc == &hadc1) {
      (void)xSemaphoreGive(g_adc1_mutex);
    }
    return JOYSTICK_DEFAULT_CENTER;
  }

  {
    value = HAL_ADC_GetValue(hadc);
    (void)HAL_ADC_Stop(hadc);
    if(hadc == &hadc1) {
      (void)xSemaphoreGive(g_adc1_mutex);
    }
    return value;
  }
}

// Initialise le joystick (calibrage des axes et lecture de l'etat initial des boutons) et les autres champs du controller.
static void JoystickAxis_Init(JoystickAxis_t * axis, ADC_HandleTypeDef * hadc, uint32_t channel)
{
  axis->hadc = hadc;
  axis->channel = channel;
  axis->center = JOYSTICK_DEFAULT_CENTER;
  axis->value = JOYSTICK_DEFAULT_CENTER;
}


// Lit l'etat du bouton de saut ou de sprint, sans debouncing, et retourne si le bouton est considéré comme pressé ou non.
static bool JoystickButton_IsPressed(GPIO_TypeDef * port, uint16_t pin, GPIO_PinState idle_state)
{
  GPIO_PinState current;

  current = HAL_GPIO_ReadPin(port, pin);
  return (current != idle_state);
}


// Lit l'etat du bouton de saut et de sprint en appliquant un debouncing, et retourne si le bouton est considéré comme pressé ou non.
static bool JoystickButton_IsPressedDebounced(GPIO_TypeDef * port,
                                              uint16_t pin,
                                              GPIO_PinState idle_state,
                                              uint8_t * debounce_counter)
{
  bool raw_pressed;

  raw_pressed = JoystickButton_IsPressed(port, pin, idle_state);
  if(raw_pressed) {
    if(*debounce_counter < JOYSTICK_BUTTON_DEBOUNCE_MAX) {
      (*debounce_counter)++;
    }
  }
  else {
    if(*debounce_counter > 0U) {
      (*debounce_counter)--;
    }
  }

  return (*debounce_counter >= (JOYSTICK_BUTTON_DEBOUNCE_MAX / 2U + 1U));
}


// Lit un echantillon DAC de 8 bits depuis le buffer d'entree, et définit le centre de gravité du joystick X en suivant la valeur lue
static void JoystickAxis_Calibrate(JoystickAxis_t * axis, uint32_t sample_count)
{
  uint32_t i;
  uint32_t sum;

  if(sample_count == 0U) {
    axis->center = JOYSTICK_DEFAULT_CENTER;
    axis->value = axis->center;
    return;
  }

  sum = 0U;
  for(i = 0U; i < sample_count; i++) {
    sum += Joystick_ReadAdcChannel(axis->hadc, axis->channel);
  }

  axis->center = sum / sample_count;
  if(axis->center == 0U) {
    axis->center = JOYSTICK_DEFAULT_CENTER;
  }

  axis->value = axis->center;
}

static void JoystickAxis_Update(JoystickAxis_t * axis)
{
  axis->value = Joystick_ReadAdcChannel(axis->hadc, axis->channel);
}

static int32_t JoystickAxis_GetDelta(const JoystickAxis_t * axis)
{
  return (int32_t)axis->value - (int32_t)axis->center;
}

static uint32_t AbsS32(int32_t value)
{ return (value < 0) ? (uint32_t)(-value) : (uint32_t)value; }

static int32_t AbsI32(int32_t value)
{  return (value < 0) ? -value : value;}

// Approche current vers target de maximum delta, en ne depassant pas target.
static int32_t ApproachI32(int32_t current, int32_t target, int32_t delta) {
    if (current < target) return (current + delta > target) ? target : current + delta;
    if (current > target) return (current - delta < target) ? target : current - delta;
    return target;
}

// On initialise les axes du joystick et les boutons, ainsi que l'etat de la physique et du jeu.
static void Controller_Init(PlayerController_t * controller)
{
  JoystickAxis_Init(&controller->joystick.axis0, &hadc1, ADC_CHANNEL_0);
  JoystickAxis_Init(&controller->joystick.axis1, &hadc3, ADC_CHANNEL_8);
  JoystickAxis_Init(&controller->joystick.axis2, &hadc3, ADC_CHANNEL_6);
  JoystickAxis_Init(&controller->joystick.axis3, &hadc3, ADC_CHANNEL_7);

  JoystickAxis_Calibrate(&controller->joystick.axis0, JOYSTICK_SAMPLE_COUNT);
  JoystickAxis_Calibrate(&controller->joystick.axis1, JOYSTICK_SAMPLE_COUNT);
  JoystickAxis_Calibrate(&controller->joystick.axis2, JOYSTICK_SAMPLE_COUNT);
  JoystickAxis_Calibrate(&controller->joystick.axis3, JOYSTICK_SAMPLE_COUNT);

  controller->joystick.jump_button_port = BP1_GPIO_Port;
  controller->joystick.jump_button_pin = BP1_Pin;
  controller->joystick.jump_button_idle_state = HAL_GPIO_ReadPin(BP1_GPIO_Port, BP1_Pin);
  controller->joystick.sprint_button_port = BP2_GPIO_Port;
  controller->joystick.sprint_button_pin = BP2_Pin;
  controller->joystick.sprint_button_idle_state = HAL_GPIO_ReadPin(BP2_GPIO_Port, BP2_Pin);

  controller->input = (PlayerInput_t){0};
  controller->physics.velocity_x_fp = 0;
  controller->physics.velocity_y_fp = 0;
  controller->physics.move_accum_x_fp = 0;
  controller->physics.move_accum_y_fp = 0;
  controller->physics.coyote_ticks = 0U;
  controller->physics.jump_buffer_ticks = 0U;
  controller->physics.jump_was_pressed = false;
  controller->physics.is_grounded = false;
  controller->game_over_waiting_restart = false;
  controller->level_complete_waiting_restart = false;
  controller->jump_button_debounce = 0U;
  controller->sprint_button_debounce = 0U;
  controller->anim_phase = 0U;
  controller->x_direction = 0;
}

static void Controller_UpdateInputs(PlayerController_t * controller)
{
  JoystickAxis_t * axes[JOYSTICK_AXIS_COUNT];
  int32_t delta[JOYSTICK_AXIS_COUNT];
  uint32_t i;
  int32_t x_delta;
  bool jump_from_button;
  bool sprint_from_button;
  int16_t filtered_dx;

  axes[0] = &controller->joystick.axis0;
  axes[1] = &controller->joystick.axis1;
  axes[2] = &controller->joystick.axis2;
  axes[3] = &controller->joystick.axis3;

  for(i = 0U; i < JOYSTICK_AXIS_COUNT; i++) {
    JoystickAxis_Update(axes[i]);
    delta[i] = JoystickAxis_GetDelta(axes[i]);
    controller->input.axis_raw[i] = axes[i]->value;
    controller->input.axis_delta[i] = delta[i];
  }

  // Dynamic center tracking for X axis
  if(AbsS32(delta[JOYSTICK_AXIS_X_INDEX]) < (uint32_t)JOYSTICK_X_CENTER_TRACK_WINDOW) {
    JoystickAxis_t * axis_x = axes[JOYSTICK_AXIS_X_INDEX];
    axis_x->center = ((axis_x->center * (JOYSTICK_X_CENTER_TRACK_ALPHA - 1U)) + axis_x->value) / JOYSTICK_X_CENTER_TRACK_ALPHA;
    delta[JOYSTICK_AXIS_X_INDEX] = JoystickAxis_GetDelta(axis_x);
    controller->input.axis_delta[JOYSTICK_AXIS_X_INDEX] = delta[JOYSTICK_AXIS_X_INDEX];
  }

  x_delta = delta[JOYSTICK_AXIS_X_INDEX];
#if JOYSTICK_AXIS_X_INVERT
  x_delta = -x_delta;
#endif

  // filtrage avec zone morte et hystérésis
  filtered_dx = controller->input.dx;
  if(AbsS32(x_delta) <= (uint32_t)JOYSTICK_DEADZONE_X_RELEASE) {
    filtered_dx = 0;
  } else if(filtered_dx == 0) {
    if(x_delta < -(int32_t)JOYSTICK_DEADZONE_X) {
      filtered_dx = -1;
    } else if(x_delta > (int32_t)JOYSTICK_DEADZONE_X) {
      filtered_dx = 1;
    }
  } else if((filtered_dx < 0 && x_delta > -(int32_t)JOYSTICK_DEADZONE_X_RELEASE) ||
            (filtered_dx > 0 && x_delta < (int32_t)JOYSTICK_DEADZONE_X_RELEASE)) {
    filtered_dx = 0;
  }

  controller->input.dx = filtered_dx;

  sprint_from_button = JoystickButton_IsPressedDebounced(controller->joystick.sprint_button_port,
                                                         controller->joystick.sprint_button_pin,
                                                         controller->joystick.sprint_button_idle_state,
                                                         &controller->sprint_button_debounce);
  controller->input.sprint_pressed = sprint_from_button;

  if(controller->input.dx != 0) {
    controller->x_direction = (int8_t)controller->input.dx;
  }

  jump_from_button = JoystickButton_IsPressedDebounced(controller->joystick.jump_button_port,
                                                       controller->joystick.jump_button_pin,
                                                       controller->joystick.jump_button_idle_state,
                                                       &controller->jump_button_debounce);
  controller->input.jump_pressed = jump_from_button;
}





// Met à jour la physique du joueur en fonction de son état actuel et des entrées lues, et applique les déplacements résultants en interagissant avec le moteur de jeu (collision incluse).
static void Controller_UpdatePhysics(PlayerController_t * controller)
{
  PlayerPhysics_t * physics;
  int16_t start_x;
  int16_t start_y;
  int16_t after_x_x;
  int16_t after_x_y;
  int16_t end_x;
  int16_t end_y;
  int16_t dx_px;
  int16_t dy_px;
  int16_t actual_dy;
  uint8_t moving;
  uint8_t jumping;
  int32_t target_vx_fp;
  int32_t max_speed_fp;
  int32_t accel_fp;
  int32_t decel_fp;
  int32_t jump_gravity_fp;
  bool jump_pressed;
  bool on_ground_start;
  bool on_ground_end;

  physics = &controller->physics;

  AppUI_PlayerGetPosition(&start_x, &start_y);
  on_ground_start = (AppUI_PlayerIsOnGround() != 0U);

  if(on_ground_start) {
    if(physics->velocity_y_fp > 0) {
      physics->velocity_y_fp = 0;
      physics->move_accum_y_fp = 0;
    }
    physics->coyote_ticks = PLAYER_COYOTE_TICKS;
  }
  else if(physics->coyote_ticks > 0U) {
    physics->coyote_ticks--;
  }

  jump_pressed = controller->input.jump_pressed;
  if(jump_pressed) {
    physics->jump_buffer_ticks = PLAYER_JUMP_BUFFER_TICKS;
  }
  else if(physics->jump_buffer_ticks > 0U) {
    physics->jump_buffer_ticks--;
  }

  if((physics->jump_buffer_ticks > 0U) &&
     (on_ground_start || (physics->coyote_ticks > 0U))) {
    physics->velocity_y_fp = -PLAYER_JUMP_SPEED_FP;
    physics->move_accum_y_fp = 0;
    physics->jump_buffer_ticks = 0U;
    physics->coyote_ticks = 0U;
    on_ground_start = false;
  }

  max_speed_fp = controller->input.sprint_pressed ? PLAYER_SPRINT_SPEED_FP : PLAYER_WALK_SPEED_FP;
  target_vx_fp = (int32_t)controller->input.dx * max_speed_fp;

  if(controller->input.dx != 0) {
    accel_fp = on_ground_start ? PLAYER_GROUND_ACCEL_FP : PLAYER_AIR_ACCEL_FP;
    physics->velocity_x_fp = ApproachI32(physics->velocity_x_fp, target_vx_fp, accel_fp);
    controller->x_direction = (int8_t)controller->input.dx;
  }
  else {
    decel_fp = on_ground_start ? PLAYER_GROUND_DECEL_FP : PLAYER_AIR_DECEL_FP;
    physics->velocity_x_fp = ApproachI32(physics->velocity_x_fp, 0, decel_fp);
  }

  jump_gravity_fp = PLAYER_GRAVITY_FP;

  if((!on_ground_start) || (physics->velocity_y_fp < 0)) {
    physics->velocity_y_fp += jump_gravity_fp;
  }

  if(physics->velocity_y_fp > PLAYER_MAX_FALL_SPEED_FP) {
    physics->velocity_y_fp = PLAYER_MAX_FALL_SPEED_FP;
  }

  if(AbsI32(physics->velocity_x_fp) < (PLAYER_FIXED_ONE / 8)) {
    physics->velocity_x_fp = 0;
    physics->move_accum_x_fp = 0;
  }

  physics->move_accum_x_fp += physics->velocity_x_fp;
  physics->move_accum_y_fp += physics->velocity_y_fp;

  dx_px = (int16_t)(physics->move_accum_x_fp / PLAYER_FIXED_ONE);
  dy_px = (int16_t)(physics->move_accum_y_fp / PLAYER_FIXED_ONE);
  physics->move_accum_x_fp -= ((int32_t)dx_px * PLAYER_FIXED_ONE);
  physics->move_accum_y_fp -= ((int32_t)dy_px * PLAYER_FIXED_ONE);

  if(dx_px > 0) {
    controller->x_direction = 1;
  }
  else if(dx_px < 0) {
    controller->x_direction = -1;
  }

  AppUI_PlayerMoveBy(dx_px, 0);
  AppUI_PlayerGetPosition(&after_x_x, &after_x_y);

  AppUI_PlayerMoveBy(0, dy_px);
  AppUI_PlayerGetPosition(&end_x, &end_y);
  actual_dy = (int16_t)(end_y - after_x_y);

  if((dy_px > 0) && (actual_dy < dy_px)) {
    physics->velocity_y_fp = 0;
    physics->move_accum_y_fp = 0;
  }
  else if((dy_px < 0) && (actual_dy > dy_px)) {
    physics->velocity_y_fp = 0;
    physics->move_accum_y_fp = 0;
  }

  on_ground_end = (AppUI_PlayerIsOnGround() != 0U);
  if(on_ground_end && (physics->velocity_y_fp > 0)) {
    physics->velocity_y_fp = 0;
    physics->move_accum_y_fp = 0;
  }

  if(physics->velocity_y_fp < 0) {
    on_ground_end = false;
  }
  if(on_ground_end) {
    physics->coyote_ticks = PLAYER_COYOTE_TICKS;
  }

  physics->is_grounded = on_ground_end;
  physics->jump_was_pressed = jump_pressed;

  moving = ((controller->input.dx != 0) || (AbsI32(physics->velocity_x_fp) >= (PLAYER_FIXED_ONE / 2))) ? 1U : 0U;
  jumping = on_ground_end ? 0U : 1U;
  controller->anim_phase++;
  AppUI_PlayerSetAnimation(moving, jumping, controller->anim_phase);
}

void Player_Init(void)
{
  Controller_Init(&g_player_controller);
}


// Met à jour le joueur et la physique du jeu via les entrées et gère les game over et victoires
void Player_Update(void)
{
  int16_t px, py;

  Controller_UpdateInputs(&g_player_controller);

  if(g_player_controller.game_over_waiting_restart) {
    if((g_player_controller.input.jump_pressed) ||
       (g_player_controller.input.sprint_pressed) ||
       (g_player_controller.input.dx != 0)) {
      AppUI_ResetPlayerPosition();
      g_player_controller.physics.velocity_x_fp = 0;
      g_player_controller.physics.velocity_y_fp = 0;
      g_player_controller.physics.move_accum_x_fp = 0;
      g_player_controller.physics.move_accum_y_fp = 0;
      g_player_controller.physics.coyote_ticks = 0U;
      g_player_controller.physics.jump_buffer_ticks = 0U;
      g_player_controller.physics.is_grounded = false;
      g_player_controller.game_over_waiting_restart = false;
      AppUI_ShowGameOver(0U);
    }
    return;
  }

  if(g_player_controller.level_complete_waiting_restart) {
    if(g_player_controller.input.jump_pressed) {
      AppUI_ResetPlayerPosition();
      AppUI_ResetLevelState();
      g_player_controller.physics.velocity_x_fp = 0;
      g_player_controller.physics.velocity_y_fp = 0;
      g_player_controller.physics.move_accum_x_fp = 0;
      g_player_controller.physics.move_accum_y_fp = 0;
      g_player_controller.physics.coyote_ticks = 0U;
      g_player_controller.physics.jump_buffer_ticks = 0U;
      g_player_controller.physics.is_grounded = false;
      g_player_controller.level_complete_waiting_restart = false;
    }
    return;
  }

  Controller_UpdatePhysics(&g_player_controller);

  AppUI_PlayerGetPosition(&px, &py);
  if(AppUI_IsLevelCompleted() != 0U) {
    g_player_controller.physics.velocity_x_fp = 0;
    g_player_controller.physics.velocity_y_fp = 0;
    g_player_controller.physics.move_accum_x_fp = 0;
    g_player_controller.physics.move_accum_y_fp = 0;
    g_player_controller.physics.is_grounded = false;
    g_player_controller.level_complete_waiting_restart = true;
    return;
  }

  if(py > PLAYER_GAME_OVER_Y) {
    g_player_controller.physics.velocity_x_fp = 0;
    g_player_controller.physics.velocity_y_fp = 0;
    g_player_controller.physics.move_accum_x_fp = 0;
    g_player_controller.physics.move_accum_y_fp = 0;
    g_player_controller.physics.is_grounded = false;
    g_player_controller.game_over_waiting_restart = true;
    AppUI_ShowGameOver(1U);
  }
}

void Player_GetDebug(PlayerDebug * debug)
{
  if(debug == NULL) {
    return;
  }

  debug->dx = g_player_controller.input.dx;
  debug->vy = (int16_t)(g_player_controller.physics.velocity_y_fp / PLAYER_FIXED_ONE);
  debug->jump = g_player_controller.input.jump_pressed ? 1U : 0U;
  debug->axis_raw[0] = g_player_controller.input.axis_raw[0];
  debug->axis_raw[1] = g_player_controller.input.axis_raw[1];
  debug->axis_raw[2] = g_player_controller.input.axis_raw[2];
  debug->axis_raw[3] = g_player_controller.input.axis_raw[3];
}
