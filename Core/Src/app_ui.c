/*
 * app_ui.c
 */

#include "app_ui_internal.h"

AppUiContext_t g_ui = {
  .runtime = {
    .bg_status_text = "bg:init",
    .sprite_status_text = "sp:init",
    .block_status_text = "bl:init",
    .bg_size = {LCD_HOR_RES, LCD_VER_RES},
    .player = {
      .box = {1U, 1U},
      .idle = {1U, 1U},
      .walk = {1U, 1U},
      .jump = {1U, 1U}
    },
    .block = {
      .normal = {1U, 1U},
      .broken = {1U, 1U},
      .ground = {1U, 1U},
      .question = {1U, 1U},
      .pipe = {1U, 1U},
      .goomba = {1U, 1U}
    },
    .level_flag = {1U, 1U},
    .level_fortress = {1U, 1U},
    .player_anim_state = APP_UI_PLAYER_SPRITE_IDLE
  }
};

const AppUI_LevelBlock g_default_level_blocks[] = {
  {2, 6, APP_UI_BLOCK_NORMAL},
  {3, 6, APP_UI_BLOCK_NORMAL},
  {4, 6, APP_UI_BLOCK_BROKEN},
  {5, 6, APP_UI_BLOCK_NORMAL},
  {6, 6, APP_UI_BLOCK_NORMAL},
  {10, 4, APP_UI_BLOCK_BROKEN},
  {11, 4, APP_UI_BLOCK_NORMAL},
  {12, 4, APP_UI_BLOCK_QUESTION},
  {14, 6, APP_UI_BLOCK_GOOMBA},
  {16, 6, APP_UI_BLOCK_PIPE},
  {16, 5, APP_UI_BLOCK_PIPE},
  {20, 6, APP_UI_BLOCK_NORMAL},
  {22, 6, APP_UI_BLOCK_NORMAL},
  {24, 6, APP_UI_BLOCK_QUESTION},
  {26, 6, APP_UI_BLOCK_NORMAL},
  {0, 8, APP_UI_BLOCK_GROUND}, {1, 8, APP_UI_BLOCK_GROUND}, {2, 8, APP_UI_BLOCK_GROUND},
  {3, 8, APP_UI_BLOCK_GROUND}, {4, 8, APP_UI_BLOCK_GROUND}, {5, 8, APP_UI_BLOCK_GROUND},
  {6, 8, APP_UI_BLOCK_GROUND}, {7, 8, APP_UI_BLOCK_GROUND}, {8, 8, APP_UI_BLOCK_GROUND},
  {9, 8, APP_UI_BLOCK_GROUND}, {10, 8, APP_UI_BLOCK_GROUND}, {11, 8, APP_UI_BLOCK_GROUND},
  {12, 8, APP_UI_BLOCK_GROUND}, {13, 8, APP_UI_BLOCK_GROUND}, {14, 8, APP_UI_BLOCK_GROUND},
  {15, 8, APP_UI_BLOCK_GROUND}, {16, 8, APP_UI_BLOCK_GROUND}, {17, 8, APP_UI_BLOCK_GROUND},
  {18, 8, APP_UI_BLOCK_GROUND}, {19, 8, APP_UI_BLOCK_GROUND}, {20, 8, APP_UI_BLOCK_GROUND},
  {21, 8, APP_UI_BLOCK_GROUND}, {22, 8, APP_UI_BLOCK_GROUND}, {23, 8, APP_UI_BLOCK_GROUND},
  {24, 8, APP_UI_BLOCK_GROUND}, {25, 8, APP_UI_BLOCK_GROUND}, {26, 8, APP_UI_BLOCK_GROUND},
  {30, 8, APP_UI_BLOCK_GROUND}, {31, 8, APP_UI_BLOCK_GROUND}, {32, 8, APP_UI_BLOCK_GROUND},
};

const uint16_t g_default_level_block_count =
  (uint16_t)(sizeof(g_default_level_blocks) / sizeof(g_default_level_blocks[0]));
