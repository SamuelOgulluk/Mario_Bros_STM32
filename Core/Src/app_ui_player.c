/*
 * app_ui_player.c
 * Player motion, collision and gameplay-facing UI APIs.
 */

#include "app_ui_internal.h"

#include "main.h"
#include "app_audio.h"
#include "lvgl.h"
static uint8_t UI_BlockAabbOverlap(int32_t ax,
                                   int32_t ay,
                                   int32_t aw,
                                   int32_t ah,
                                   int32_t bx,
                                   int32_t by,
                                   int32_t bw,
                                   int32_t bh)
{
  if((ax + aw) <= bx) {
    return 0U;
  }
  if(ax >= (bx + bw)) {
    return 0U;
  }
  if((ay + ah) <= by) {
    return 0U;
  }
  if(ay >= (by + bh)) {
    return 0U;
  }

  return 1U;
}

static void UI_GetPlayerColliderRect(int32_t world_x,
                                     int32_t y,
                                     int32_t * cx,
                                     int32_t * cy,
                                     int32_t * cw,
                                     int32_t * ch)
{
  int32_t raw_w;
  int32_t raw_h;
  int32_t box_w;
  int32_t box_h;
  int32_t off_x;
  int32_t off_y;

  raw_w = (int32_t)UI_RT.player.box.w;
  raw_h = (int32_t)UI_RT.player.box.h;

  /* Keep gameplay collisions stable even if sprite BMP includes large transparent margins. */
  box_w = raw_w;
  box_h = raw_h;
  if(box_w > (int32_t)APP_UI_PLAYER_BOX_DEFAULT_W) {
    box_w = (int32_t)APP_UI_PLAYER_BOX_DEFAULT_W;
  }
  if(box_h > (int32_t)APP_UI_PLAYER_BOX_DEFAULT_H) {
    box_h = (int32_t)APP_UI_PLAYER_BOX_DEFAULT_H;
  }

  if(box_w > (2 * (int32_t)PLAYER_COLLIDER_MARGIN_X)) {
    box_w -= (2 * (int32_t)PLAYER_COLLIDER_MARGIN_X);
  }

  if(box_h > ((int32_t)PLAYER_COLLIDER_MARGIN_TOP + (int32_t)PLAYER_COLLIDER_MARGIN_BOTTOM)) {
    box_h -= ((int32_t)PLAYER_COLLIDER_MARGIN_TOP + (int32_t)PLAYER_COLLIDER_MARGIN_BOTTOM);
  }

  if(box_w < 8) {
    box_w = 8;
  }
  if(box_h < 8) {
    box_h = 8;
  }

  off_x = (raw_w - box_w) / 2;
  if(off_x < 0) {
    off_x = 0;
  }

  off_y = raw_h - box_h - (int32_t)PLAYER_COLLIDER_MARGIN_BOTTOM;
  if(off_y < 0) {
    off_y = 0;
  }

  if(cx != NULL) {
    *cx = world_x + off_x;
  }
  if(cy != NULL) {
    *cy = y + off_y;
  }
  if(cw != NULL) {
    *cw = box_w;
  }
  if(ch != NULL) {
    *ch = box_h;
  }
}

static uint8_t UI_PlayerOverlapsSolidAt(int32_t world_x, int32_t y)
{
  int32_t col_x;
  int32_t col_y;
  int32_t col_w;
  int32_t col_h;
  int32_t min_grid_x;
  int32_t max_grid_x;
  int32_t min_grid_y;
  int32_t max_grid_y;
  int32_t gx;
  int32_t gy;

  UI_GetPlayerColliderRect(world_x, y, &col_x, &col_y, &col_w, &col_h);

  min_grid_x = (col_x / (int32_t)APP_UI_BLOCK_GRID_PX) - 1;
  max_grid_x = ((col_x + col_w - 1) / (int32_t)APP_UI_BLOCK_GRID_PX) + 1;
  min_grid_y = (col_y / (int32_t)APP_UI_BLOCK_GRID_PX) - 1;
  max_grid_y = ((col_y + col_h - 1) / (int32_t)APP_UI_BLOCK_GRID_PX) + 1;

  if(max_grid_x < 0 || max_grid_y < 0 || min_grid_x >= (int32_t)APP_UI_SOLID_GRID_W || min_grid_y >= (int32_t)APP_UI_SOLID_GRID_H) {
    return 0U;
  }

  if(min_grid_x < 0) {
    min_grid_x = 0;
  }
  if(min_grid_y < 0) {
    min_grid_y = 0;
  }
  if(max_grid_x >= (int32_t)APP_UI_SOLID_GRID_W) {
    max_grid_x = (int32_t)APP_UI_SOLID_GRID_W - 1;
  }
  if(max_grid_y >= (int32_t)APP_UI_SOLID_GRID_H) {
    max_grid_y = (int32_t)APP_UI_SOLID_GRID_H - 1;
  }

  for(gy = min_grid_y; gy <= max_grid_y; gy++) {
    for(gx = min_grid_x; gx <= max_grid_x; gx++) {
      int32_t bx;
      int32_t by;

      if(UI_LEVEL.solid_grid[gy][gx] == 0U) {
        continue;
      }

      bx = gx * (int32_t)APP_UI_BLOCK_GRID_PX;
      by = gy * (int32_t)APP_UI_BLOCK_GRID_PX;

      if(UI_BlockAabbOverlap(col_x,
                             col_y,
                             col_w,
                             col_h,
                             bx,
                             by,
                             (int32_t)APP_UI_BLOCK_GRID_PX,
                             (int32_t)APP_UI_BLOCK_GRID_PX) != 0U) {
        return 1U;
      }
    }
  }

  return 0U;
}

static int32_t UI_AbsS32(int32_t value)
{
  if(value < 0) {
    return -value;
  }
  return value;
}

static int32_t UI_ResolveHorizontalDelta(int32_t world_x, int32_t y, int32_t dx)
{
  int32_t move;
  int32_t step;
  int32_t i;
  int32_t probe_y;
  int32_t lift;

  if((dx == 0) || (UI_LEVEL.level_block_count == 0U)) {
    return dx;
  }

  probe_y = y;
  lift = 0;
  while((lift < PLAYER_HORIZONTAL_COLLISION_LIFT_PX) &&
        (UI_PlayerOverlapsSolidAt(world_x, probe_y) != 0U)) {
    probe_y--;
    lift++;
  }

  move = 0;
  step = (dx > 0) ? 1 : -1;
  for(i = 0; i < UI_AbsS32(dx); i++) {
    if(UI_PlayerOverlapsSolidAt(world_x + move + step, probe_y) != 0U) {
      break;
    }
    move += step;
  }

  return move;
}

static int32_t UI_ResolveVerticalDelta(int32_t world_x, int32_t y, int32_t dy)
{
  int32_t move;
  int32_t step;
  int32_t i;

  if((dy == 0) || (UI_LEVEL.level_block_count == 0U)) {
    return dy;
  }

  move = 0;
  step = (dy > 0) ? 1 : -1;
  for(i = 0; i < UI_AbsS32(dy); i++) {
    if(UI_PlayerOverlapsSolidAt(world_x, y + move + step) != 0U) {
      break;
    }
    move += step;
  }

  return move;
}

static void UI_ComputeMoveZone(int32_t max_player_x, int32_t * min_x, int32_t * max_x)
{
  int32_t zone_min;
  int32_t zone_max;

  zone_min = PLAYER_SCROLL_MIN_X;
  zone_max = PLAYER_SCROLL_MAX_X;

  if(zone_min > max_player_x) {
    zone_min = max_player_x;
  }
  if(zone_max > max_player_x) {
    zone_max = max_player_x;
  }
  if(zone_max < zone_min) {
    zone_max = zone_min;
  }

  if(min_x != NULL) {
    *min_x = zone_min;
  }
  if(max_x != NULL) {
    *max_x = zone_max;
  }
}

static void UI_UpdatePlayerFacingFromDelta(int16_t dx)
{
  if(dx < 0) {
    if(UI_RT.player_facing_left == 0U) {
      UI_RT.player_facing_left = 1U;
      UI_ApplyPlayerFacingDirection();
    }
  }
  else if(dx > 0) {
    if(UI_RT.player_facing_left != 0U) {
      UI_RT.player_facing_left = 0U;
      UI_ApplyPlayerFacingDirection();
    }
  }
}

void AppUI_PlayerMoveBy(int16_t dx, int16_t dy)
{
  int32_t world_dx;
  int32_t world_dy;
  int32_t world_x;
  int32_t next_world_x;
  int32_t current_x;
  int32_t current_y;
  int32_t move_zone_min_x;
  int32_t move_zone_max_x;
  int32_t max_player_x;
  int32_t screen_x;
  int32_t next_y;
  int32_t target_scroll_x;

  if(UI_SCENE.player_motion_layer == NULL) {
    return;
  }

  current_x = UI_RT.player_x;
  current_y = UI_RT.player_y;
  world_x = current_x + UI_RT.world_scroll_x;
  world_dx = UI_ResolveHorizontalDelta(world_x, current_y, (int32_t)dx);
  next_world_x = world_x + world_dx;
  world_dy = UI_ResolveVerticalDelta(next_world_x, current_y, (int32_t)dy);
  next_y = current_y + world_dy;

  max_player_x = (int32_t)LCD_HOR_RES - (int32_t)UI_RT.player.box.w;
  UI_ComputeMoveZone(max_player_x, &move_zone_min_x, &move_zone_max_x);

  screen_x = next_world_x - UI_RT.world_scroll_x;
  if(screen_x < move_zone_min_x) {
    target_scroll_x = next_world_x - move_zone_min_x;
    if(target_scroll_x < 0) {
      target_scroll_x = 0;
    }
    UI_RT.world_scroll_x = (target_scroll_x / PLAYER_SCROLL_STEP_PX) * PLAYER_SCROLL_STEP_PX;
    screen_x = next_world_x - UI_RT.world_scroll_x;
  }
  else if(screen_x > move_zone_max_x) {
    target_scroll_x = next_world_x - move_zone_max_x;
    if(target_scroll_x < 0) {
      target_scroll_x = 0;
    }
    UI_RT.world_scroll_x = ((target_scroll_x + PLAYER_SCROLL_STEP_PX - 1) / PLAYER_SCROLL_STEP_PX) * PLAYER_SCROLL_STEP_PX;
    screen_x = next_world_x - UI_RT.world_scroll_x;
  }

  if(screen_x < 0) {
    UI_RT.world_scroll_x = next_world_x;
    screen_x = 0;
  }
  else if(screen_x > max_player_x) {
    UI_RT.world_scroll_x = next_world_x - max_player_x;
    if(UI_RT.world_scroll_x < 0) {
      UI_RT.world_scroll_x = 0;
    }
    screen_x = max_player_x;
  }

  if(next_y < 0) {
    next_y = 0;
  }

  if((screen_x != current_x) || (next_y != current_y)) {
    lv_obj_set_pos(UI_SCENE.player_motion_layer, screen_x, next_y);
    UI_RT.player_x = screen_x;
    UI_RT.player_y = next_y;
  }
  UI_UpdateWorldScroll();

  if((UI_RT.level_completed == 0U) &&
     ((next_world_x + (int32_t)UI_RT.player.box.w) >= UI_RT.level_end_world_x)) {
    UI_RT.level_completed = 1U;
    AppUI_ShowLevelComplete(1U);
  }

  UI_UpdatePlayerFacingFromDelta(dx);
}

void AppUI_PlayerGetPosition(int16_t * x, int16_t * y)
{
  if(UI_SCENE.player_motion_layer == NULL) {
    if(x != NULL) {
      *x = 0;
    }
    if(y != NULL) {
      *y = 0;
    }
    return;
  }

  if(x != NULL) {
    *x = (int16_t)UI_RT.player_x;
  }
  if(y != NULL) {
    *y = (int16_t)UI_RT.player_y;
  }
}

void AppUI_ResetPlayerPosition(void)
{
  if(UI_SCENE.player_motion_layer == NULL) {
    return;
  }
  
  UI_RT.world_scroll_x = 0;
  UI_RT.player_x = PLAYER_SCROLL_MIN_X;
  UI_RT.player_y = 0;
  lv_obj_set_pos(UI_SCENE.player_motion_layer, UI_RT.player_x, UI_RT.player_y);
  UI_UpdateWorldScroll();
}

void AppUI_ShowGameOver(uint8_t visible)
{
  if(UI_SCENE.game_over_label == NULL) {
    return;
  }

  if(visible != 0U) {
    lv_obj_clear_flag(UI_SCENE.game_over_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(UI_SCENE.game_over_label);
  }
  else {
    lv_obj_add_flag(UI_SCENE.game_over_label, LV_OBJ_FLAG_HIDDEN);
  }
}

uint8_t AppUI_IsLevelCompleted(void)
{
  return UI_RT.level_completed;
}

void AppUI_ShowLevelComplete(uint8_t visible)
{
  if(UI_SCENE.level_complete_label == NULL) {
    return;
  }

  if(visible != 0U) {
    lv_obj_clear_flag(UI_SCENE.level_complete_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(UI_SCENE.level_complete_label);
  }
  else {
    lv_obj_add_flag(UI_SCENE.level_complete_label, LV_OBJ_FLAG_HIDDEN);
  }
}

void AppUI_ResetLevelState(void)
{
  UI_RT.level_completed = 0U;
  AppUI_ShowLevelComplete(0U);
}

int16_t AppUI_PlayerGetGroundY(void)
{
  int16_t player_x;
  int16_t player_y;
  int32_t world_x;
  int32_t probe_y;
  int32_t y;

  if(UI_SCENE.player_motion_layer == NULL) {
    return 0;
  }

  AppUI_PlayerGetPosition(&player_x, &player_y);
  world_x = (int32_t)player_x + UI_RT.world_scroll_x;
  probe_y = (int32_t)player_y;

  for(y = probe_y; y < (probe_y + (int32_t)LCD_VER_RES + (int32_t)APP_UI_BLOCK_GRID_PX); y++) {
    if(UI_ResolveVerticalDelta(world_x, y, 1) == 0) {
      return (int16_t)y;
    }
  }

  return (int16_t)(probe_y + (int32_t)LCD_VER_RES);
}

uint8_t AppUI_PlayerIsOnGround(void)
{
  int16_t player_x;
  int16_t player_y;
  int32_t world_x;
  uint8_t overlaps_now;
  int32_t can_move_down;

  if(UI_SCENE.player_motion_layer == NULL) {
    return 0U;
  }

  AppUI_PlayerGetPosition(&player_x, &player_y);
  world_x = (int32_t)player_x + UI_RT.world_scroll_x;

  overlaps_now = UI_PlayerOverlapsSolidAt(world_x, (int32_t)player_y);
  if(overlaps_now != 0U) {
    return 0U;
  }

  can_move_down = UI_ResolveVerticalDelta(world_x, (int32_t)player_y, 1);
  return (can_move_down == 0) ? 1U : 0U;
}

void UI_SetDebugInput(int16_t dx, int16_t vy, uint8_t jump, uint32_t a0, uint32_t a8, uint32_t a6, uint32_t a7)
{
  char * debug_text;
  const char * audio_status;

  if(UI_SCENE.debug_label == NULL) {
    return;
  }

  audio_status = AppAudio_GetStatus();
  UI_RT.debug_text_index ^= 1U;
  debug_text = UI_RT.debug_text[UI_RT.debug_text_index];

  lv_snprintf(debug_text,
              sizeof(UI_RT.debug_text[0]),
              "A0:%u A8:%u\nA6:%u A7:%u\ndx:%d j:%u vy:%d\n%s %s %s\n%s",
              (unsigned int)a0,
              (unsigned int)a8,
              (unsigned int)a6,
              (unsigned int)a7,
              (int)dx,
              (unsigned int)jump,
              (int)vy,
              UI_RT.bg_status_text,
              UI_RT.sprite_status_text,
              UI_RT.block_status_text,
              audio_status);
  lv_label_set_text_static(UI_SCENE.debug_label, debug_text);
}
