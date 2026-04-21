/*
 * app_ui_scene.c
 * Scene, chargement des assets, rendu du niveau et des sprites, et mise à jour runtime du UI.
 */

#include "app_ui_internal.h"

#include "main.h"
#include "ltdc.h"
#include "app_audio.h"
#include "app_ui_assets.h"
#include "lvgl.h"
#include "stm32746g_discovery_sd.h"
#include "ff.h"
#include "ff_gen_drv.h"
#include "sd_diskio.h"

static uint8_t UI_IsSolidBlock(const AppUI_LevelBlock * block);

// Initialisation du stockage FATFS sur la carte SD, avec liaison du driver et montage de la partition.
static void Storage_Init(void)
{
  uint32_t retry;
  uint32_t tick_start;

  UI_STORAGE.ready = 0U;

  if(FATFS_LinkDriver(&SD_Driver, UI_STORAGE.path) != 0U) {
    lv_snprintf(UI_RT.bg_status_text, sizeof(UI_RT.bg_status_text), "bg:drv err");
    return;
  }

  for(retry = 0U; retry < 3U; retry++) {
    if(retry > 0U) {
      (void)BSP_SD_DeInit();
      HAL_Delay(10U);
    }

    if(BSP_SD_Init() == MSD_OK) {
      tick_start = HAL_GetTick();
      while((HAL_GetTick() - tick_start) < 500U) {
        if(BSP_SD_GetCardState() == SD_TRANSFER_OK) {
          break;
        }
      }

      HAL_Delay(20U);

      if(f_mount(&UI_STORAGE.fatfs, UI_STORAGE.path, 1U) == FR_OK) {
        UI_STORAGE.ready = 1U;
        lv_snprintf(UI_RT.bg_status_text, sizeof(UI_RT.bg_status_text), "bg:sd ok");
        return;
      }
    }

    HAL_Delay(30U);
  }

  lv_snprintf(UI_RT.bg_status_text, sizeof(UI_RT.bg_status_text), "bg:sd fail");
}


// Ouvre et charge le background BMP depuis la carte SD, et crée l'objet d'affichage correspondant.
static void UI_CreateBackgroundObject(lv_obj_t * parent)
{
  lv_obj_t * bg_img;
  if(AppUI_LoadBmpToDrawBuf(BACKGROUND_IMAGE_FATFS_PATH,
                            &UI_SCENE.background_draw_buf,
                            (void *)BACKGROUND_DRAW_BUF_ADDR,
                            BACKGROUND_DRAW_BUF_SIZE,
                            NULL,
                            &UI_RT.bg_size.w,
                            &UI_RT.bg_size.h,
                            LV_COLOR_FORMAT_RGB565,
                            0U,
                            UI_RT.bg_status_text,
                            sizeof(UI_RT.bg_status_text),
                            "bg") == 0U) {
    return;
  }

  bg_img = lv_image_create(parent);
  if(bg_img == NULL) {
    lv_snprintf(UI_RT.bg_status_text, sizeof(UI_RT.bg_status_text), "bg:create err");
    return;
  }

  UI_SCENE.background_image = bg_img;

  lv_obj_remove_style_all(bg_img);
  lv_image_set_src(bg_img, &UI_SCENE.background_draw_buf);
  lv_obj_set_size(bg_img, UI_RT.bg_size.w, UI_RT.bg_size.h);
  lv_image_set_inner_align(bg_img, LV_IMAGE_ALIGN_CENTER);
  lv_obj_set_pos(bg_img, 0, 0);
  lv_obj_set_style_opa(bg_img, LV_OPA_COVER, 0);
  lv_obj_set_style_image_opa(bg_img, LV_OPA_COVER, 0);
  lv_obj_clear_flag(bg_img, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_move_background(bg_img);
}


// Création de mario
static lv_obj_t * UI_CreatePlayerSpriteObject(lv_obj_t * parent,lv_draw_buf_t * draw_buf,uint16_t box_width,uint16_t box_height,uint8_t hidden)
{
  lv_obj_t * sprite_obj;
  uint16_t sprite_width;
  uint16_t sprite_height;
  int32_t pos_x;
  int32_t pos_y;

  if(draw_buf == NULL) {
    return NULL;
  }

  sprite_width = (uint16_t)draw_buf->header.w;
  sprite_height = (uint16_t)draw_buf->header.h;

  sprite_obj = lv_image_create(parent);
  if(sprite_obj == NULL) {
    return NULL;
  }

  lv_obj_remove_style_all(sprite_obj);
  lv_image_set_src(sprite_obj, draw_buf);
  lv_obj_set_size(sprite_obj, sprite_width, sprite_height);
  lv_image_set_inner_align(sprite_obj, LV_IMAGE_ALIGN_CENTER);
  lv_obj_set_style_opa(sprite_obj, LV_OPA_COVER, 0);
  lv_obj_set_style_image_opa(sprite_obj, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_opa(sprite_obj, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_opa(sprite_obj, LV_OPA_TRANSP, 0);
  lv_obj_clear_flag(sprite_obj, LV_OBJ_FLAG_CLICKABLE);

  pos_x = ((int32_t)box_width - (int32_t)sprite_width) / 2;
  if(pos_x < 0) {
    pos_x = 0;
  }
  pos_y = (int32_t)box_height - (int32_t)sprite_height;
  if(pos_y < 0) {
    pos_y = 0;
  }

  lv_obj_set_pos(sprite_obj, pos_x, pos_y);

  if(hidden != 0U) {
    lv_obj_add_flag(sprite_obj, LV_OBJ_FLAG_HIDDEN);
  }

  return sprite_obj;
}


// 
static lv_draw_buf_t * UI_CreateMirroredDrawBufArgb8888(const lv_draw_buf_t * src)
{
  lv_draw_buf_t * dst;
  uint32_t w;
  uint32_t h;
  uint32_t y;

  if((src == NULL) || (src->header.cf != LV_COLOR_FORMAT_ARGB8888)) {
    return NULL;
  }

  w = src->header.w;
  h = src->header.h;
  dst = lv_draw_buf_create(w, h, LV_COLOR_FORMAT_ARGB8888, LV_STRIDE_AUTO);
  if(dst == NULL) {
    return NULL;
  }

  for(y = 0U; y < h; y++) {
    lv_color32_t * src_row;
    lv_color32_t * dst_row;
    uint32_t x;

    src_row = (lv_color32_t *)lv_draw_buf_goto_xy((lv_draw_buf_t *)src, 0U, y);
    dst_row = (lv_color32_t *)lv_draw_buf_goto_xy(dst, 0U, y);
    if((src_row == NULL) || (dst_row == NULL)) {
      lv_draw_buf_destroy(dst);
      return NULL;
    }

    for(x = 0U; x < w; x++) {
      dst_row[x] = src_row[w - 1U - x];
    }
  }

  return dst;
}


// 
void UI_ApplyPlayerFacingDirection(void)
{
  lv_draw_buf_t * idle_src;
  lv_draw_buf_t * walk_src;
  lv_draw_buf_t * jump_src;

  if(UI_RT.player_facing_left != 0U) {
    idle_src = (UI_SCENE.player_idle_draw_buf_left != NULL) ? UI_SCENE.player_idle_draw_buf_left : UI_SCENE.player_idle_draw_buf;
    walk_src = (UI_SCENE.player_walk_draw_buf_left != NULL) ? UI_SCENE.player_walk_draw_buf_left : UI_SCENE.player_walk_draw_buf;
    jump_src = (UI_SCENE.player_jump_draw_buf_left != NULL) ? UI_SCENE.player_jump_draw_buf_left : UI_SCENE.player_jump_draw_buf;
  }
  else {
    idle_src = UI_SCENE.player_idle_draw_buf;
    walk_src = UI_SCENE.player_walk_draw_buf;
    jump_src = UI_SCENE.player_jump_draw_buf;
  }

  if((UI_SCENE.player_idle_obj != NULL) && (idle_src != NULL)) {
    lv_image_set_src(UI_SCENE.player_idle_obj, idle_src);
  }
  if((UI_SCENE.player_walk_obj != NULL) && (walk_src != NULL)) {
    lv_image_set_src(UI_SCENE.player_walk_obj, walk_src);
  }
  if((UI_SCENE.player_jump_obj != NULL) && (jump_src != NULL)) {
    lv_image_set_src(UI_SCENE.player_jump_obj, jump_src);
  }
}

static void UI_SetPlayerSpriteState(AppUI_PlayerSpriteState state)
{
  if(UI_SCENE.player_motion_layer == NULL) {
    return;
  }

  if(UI_RT.player_anim_state == state) {
    return;
  }

  if(state == APP_UI_PLAYER_SPRITE_JUMP) {
    if(UI_SCENE.player_jump_obj != NULL) {
      lv_obj_clear_flag(UI_SCENE.player_jump_obj, LV_OBJ_FLAG_HIDDEN);
      UI_SCENE.player_obj = UI_SCENE.player_jump_obj;
    }
    if(UI_SCENE.player_idle_obj != NULL) {
      lv_obj_add_flag(UI_SCENE.player_idle_obj, LV_OBJ_FLAG_HIDDEN);
    }
    if(UI_SCENE.player_walk_obj != NULL) {
      lv_obj_add_flag(UI_SCENE.player_walk_obj, LV_OBJ_FLAG_HIDDEN);
    }
    lv_snprintf(UI_RT.sprite_status_text, sizeof(UI_RT.sprite_status_text), "sp:jump");
    UI_RT.player_anim_state = APP_UI_PLAYER_SPRITE_JUMP;
    return;
  }

  if(state == APP_UI_PLAYER_SPRITE_WALK) {
    if(UI_SCENE.player_walk_obj != NULL) {
      lv_obj_clear_flag(UI_SCENE.player_walk_obj, LV_OBJ_FLAG_HIDDEN);
      UI_SCENE.player_obj = UI_SCENE.player_walk_obj;
    }
    if(UI_SCENE.player_idle_obj != NULL) {
      lv_obj_add_flag(UI_SCENE.player_idle_obj, LV_OBJ_FLAG_HIDDEN);
    }
    if(UI_SCENE.player_jump_obj != NULL) {
      lv_obj_add_flag(UI_SCENE.player_jump_obj, LV_OBJ_FLAG_HIDDEN);
    }
    lv_snprintf(UI_RT.sprite_status_text, sizeof(UI_RT.sprite_status_text), "sp:walk");
    UI_RT.player_anim_state = APP_UI_PLAYER_SPRITE_WALK;
    return;
  }

  if(UI_SCENE.player_idle_obj != NULL) {
    lv_obj_clear_flag(UI_SCENE.player_idle_obj, LV_OBJ_FLAG_HIDDEN);
    UI_SCENE.player_obj = UI_SCENE.player_idle_obj;
  }
  if(UI_SCENE.player_walk_obj != NULL) {
    lv_obj_add_flag(UI_SCENE.player_walk_obj, LV_OBJ_FLAG_HIDDEN);
  }
  if(UI_SCENE.player_jump_obj != NULL) {
    lv_obj_add_flag(UI_SCENE.player_jump_obj, LV_OBJ_FLAG_HIDDEN);
  }
  lv_snprintf(UI_RT.sprite_status_text, sizeof(UI_RT.sprite_status_text), "sp:idle");
  UI_RT.player_anim_state = APP_UI_PLAYER_SPRITE_IDLE;
}

static void UI_LoadPlayerSprite(void)
{
  UI_SCENE.player_idle_draw_buf = NULL;
  UI_SCENE.player_walk_draw_buf = NULL;
  UI_SCENE.player_jump_draw_buf = NULL;
  UI_SCENE.player_idle_draw_buf_left = NULL;
  UI_SCENE.player_walk_draw_buf_left = NULL;
  UI_SCENE.player_jump_draw_buf_left = NULL;
  UI_RT.player.idle.w = 0U;
  UI_RT.player.idle.h = 0U;
  UI_RT.player.walk.w = 0U;
  UI_RT.player.walk.h = 0U;
  UI_RT.player.jump.w = 0U;
  UI_RT.player.jump.h = 0U;

  if(UI_STORAGE.ready == 0U) {
    lv_snprintf(UI_RT.sprite_status_text, sizeof(UI_RT.sprite_status_text), "sp:no sd");
    UI_RT.player.box.w = APP_UI_PLAYER_BOX_DEFAULT_W;
    UI_RT.player.box.h = APP_UI_PLAYER_BOX_DEFAULT_H;
    return;
  }

  if(AppUI_LoadBmpToDrawBuf(PLAYER_IDLE_IMAGE_FATFS_PATH,
                            NULL,
                            NULL,
                            0U,
                            &UI_SCENE.player_idle_draw_buf,
                            &UI_RT.player.idle.w,
                            &UI_RT.player.idle.h,
                            LV_COLOR_FORMAT_ARGB8888,
                            1U,
                            UI_RT.sprite_status_text,
                            sizeof(UI_RT.sprite_status_text),
                            "sp") == 0U) {
    lv_snprintf(UI_RT.sprite_status_text, sizeof(UI_RT.sprite_status_text), "sp:idle err");
  }

  (void)AppUI_LoadBmpToDrawBuf(PLAYER_WALK_IMAGE_FATFS_PATH,
                               NULL,
                               NULL,
                               0U,
                               &UI_SCENE.player_walk_draw_buf,
                               &UI_RT.player.walk.w,
                               &UI_RT.player.walk.h,
                               LV_COLOR_FORMAT_ARGB8888,
                               1U,
                               UI_RT.sprite_status_text,
                               sizeof(UI_RT.sprite_status_text),
                               "sp");

  (void)AppUI_LoadBmpToDrawBuf(PLAYER_JUMP_IMAGE_FATFS_PATH,
                               NULL,
                               NULL,
                               0U,
                               &UI_SCENE.player_jump_draw_buf,
                               &UI_RT.player.jump.w,
                               &UI_RT.player.jump.h,
                               LV_COLOR_FORMAT_ARGB8888,
                               1U,
                               UI_RT.sprite_status_text,
                               sizeof(UI_RT.sprite_status_text),
                               "sp");

  UI_RT.player.box.w = UI_RT.player.idle.w;
  if(UI_RT.player.walk.w > UI_RT.player.box.w) {
    UI_RT.player.box.w = UI_RT.player.walk.w;
  }
  if(UI_RT.player.jump.w > UI_RT.player.box.w) {
    UI_RT.player.box.w = UI_RT.player.jump.w;
  }

  UI_RT.player.box.h = UI_RT.player.idle.h;
  if(UI_RT.player.walk.h > UI_RT.player.box.h) {
    UI_RT.player.box.h = UI_RT.player.walk.h;
  }
  if(UI_RT.player.jump.h > UI_RT.player.box.h) {
    UI_RT.player.box.h = UI_RT.player.jump.h;
  }

  if((UI_SCENE.player_idle_draw_buf == NULL) && (UI_SCENE.player_walk_draw_buf == NULL) && (UI_SCENE.player_jump_draw_buf == NULL)) {
    UI_RT.player.box.w = APP_UI_PLAYER_BOX_DEFAULT_W;
    UI_RT.player.box.h = APP_UI_PLAYER_BOX_DEFAULT_H;
    return;
  }

  UI_SCENE.player_idle_draw_buf_left = UI_CreateMirroredDrawBufArgb8888(UI_SCENE.player_idle_draw_buf);
  UI_SCENE.player_walk_draw_buf_left = UI_CreateMirroredDrawBufArgb8888(UI_SCENE.player_walk_draw_buf);
  UI_SCENE.player_jump_draw_buf_left = UI_CreateMirroredDrawBufArgb8888(UI_SCENE.player_jump_draw_buf);
}

static void UI_CreatePlayerSprites(void)
{
  if(UI_SCENE.player_motion_layer == NULL) {
    return;
  }

  UI_SCENE.player_idle_obj = UI_CreatePlayerSpriteObject(UI_SCENE.player_motion_layer,
                                                  UI_SCENE.player_idle_draw_buf,
                                                  UI_RT.player.box.w,
                                                  UI_RT.player.box.h,
                                                  0U);
  UI_SCENE.player_walk_obj = UI_CreatePlayerSpriteObject(UI_SCENE.player_motion_layer,
                                                  UI_SCENE.player_walk_draw_buf,
                                                  UI_RT.player.box.w,
                                                  UI_RT.player.box.h,
                                                  1U);
  UI_SCENE.player_jump_obj = UI_CreatePlayerSpriteObject(UI_SCENE.player_motion_layer,
                                                  UI_SCENE.player_jump_draw_buf,
                                                  UI_RT.player.box.w,
                                                  UI_RT.player.box.h,
                                                  1U);

  if((UI_SCENE.player_idle_obj == NULL) && (UI_SCENE.player_walk_obj == NULL) && (UI_SCENE.player_jump_obj == NULL)) {
    lv_snprintf(UI_RT.sprite_status_text, sizeof(UI_RT.sprite_status_text), "sp:no sprite");
    return;
  }

  UI_ApplyPlayerFacingDirection();
  UI_SetPlayerSpriteState(APP_UI_PLAYER_SPRITE_IDLE);
}

static void UI_LoadBlockSprites(void)
{
  UI_SCENE.block_draw_buf = NULL;
  UI_SCENE.block_broken_draw_buf = NULL;
  UI_SCENE.block_ground_draw_buf = NULL;
  UI_SCENE.block_question_draw_buf = NULL;
  UI_SCENE.block_pipe_draw_buf = NULL;
  UI_SCENE.block_goomba_draw_buf = NULL;
  UI_SCENE.level_flag_draw_buf = NULL;
  UI_SCENE.level_fortress_draw_buf = NULL;

  if(UI_STORAGE.ready == 0U) {
    lv_snprintf(UI_RT.block_status_text, sizeof(UI_RT.block_status_text), "bl:no sd");
    return;
  }

  if(AppUI_LoadBmpToDrawBuf(BLOCK_IMAGE_FATFS_PATH,
                            NULL,
                            NULL,
                            0U,
                            &UI_SCENE.block_draw_buf,
                            &UI_RT.block.normal.w,
                            &UI_RT.block.normal.h,
                            LV_COLOR_FORMAT_RGB565,
                            0U,
                            UI_RT.block_status_text,
                            sizeof(UI_RT.block_status_text),
                            "bl") == 0U) {
    return;
  }

  (void)AppUI_LoadBmpToDrawBuf(BLOCK_BROKEN_IMAGE_FATFS_PATH,
                               NULL,
                               NULL,
                               0U,
                               &UI_SCENE.block_broken_draw_buf,
                               &UI_RT.block.broken.w,
                               &UI_RT.block.broken.h,
                               LV_COLOR_FORMAT_RGB565,
                               0U,
                               UI_RT.block_status_text,
                               sizeof(UI_RT.block_status_text),
                               "bl");

  (void)AppUI_LoadBmpToDrawBuf(BLOCK_GROUND_IMAGE_FATFS_PATH,
                               NULL,
                               NULL,
                               0U,
                               &UI_SCENE.block_ground_draw_buf,
                               &UI_RT.block.ground.w,
                               &UI_RT.block.ground.h,
                               LV_COLOR_FORMAT_RGB565,
                               0U,
                               UI_RT.block_status_text,
                               sizeof(UI_RT.block_status_text),
                               "bl");

  (void)AppUI_LoadBmpToDrawBuf(BLOCK_QUESTION_IMAGE_FATFS_PATH,
                               NULL,
                               NULL,
                               0U,
                               &UI_SCENE.block_question_draw_buf,
                               &UI_RT.block.question.w,
                               &UI_RT.block.question.h,
                               LV_COLOR_FORMAT_RGB565,
                               0U,
                               UI_RT.block_status_text,
                               sizeof(UI_RT.block_status_text),
                               "bl");

  (void)AppUI_LoadBmpToDrawBuf(BLOCK_PIPE_IMAGE_FATFS_PATH,
                               NULL,
                               NULL,
                               0U,
                               &UI_SCENE.block_pipe_draw_buf,
                               &UI_RT.block.pipe.w,
                               &UI_RT.block.pipe.h,
                               LV_COLOR_FORMAT_ARGB8888,
                               1U,
                               UI_RT.block_status_text,
                               sizeof(UI_RT.block_status_text),
                               "bl");

  (void)AppUI_LoadBmpToDrawBuf(BLOCK_GOOMBA_IMAGE_FATFS_PATH,
                               NULL,
                               NULL,
                               0U,
                               &UI_SCENE.block_goomba_draw_buf,
                               &UI_RT.block.goomba.w,
                               &UI_RT.block.goomba.h,
                               LV_COLOR_FORMAT_ARGB8888,
                               1U,
                               UI_RT.block_status_text,
                               sizeof(UI_RT.block_status_text),
                               "bl");

  (void)AppUI_LoadBmpToDrawBuf(LEVEL_FLAG_IMAGE_FATFS_PATH,
                               NULL,
                               NULL,
                               0U,
                               &UI_SCENE.level_flag_draw_buf,
                               &UI_RT.level_flag.w,
                               &UI_RT.level_flag.h,
                               LV_COLOR_FORMAT_ARGB8888,
                               1U,
                               UI_RT.block_status_text,
                               sizeof(UI_RT.block_status_text),
                               "bl");

  (void)AppUI_LoadBmpToDrawBuf(LEVEL_FORTRESS_IMAGE_FATFS_PATH,
                               NULL,
                               NULL,
                               0U,
                               &UI_SCENE.level_fortress_draw_buf,
                               &UI_RT.level_fortress.w,
                               &UI_RT.level_fortress.h,
                               LV_COLOR_FORMAT_ARGB8888,
                               1U,
                               UI_RT.block_status_text,
                               sizeof(UI_RT.block_status_text),
                               "bl");

  lv_snprintf(UI_RT.block_status_text,
              sizeof(UI_RT.block_status_text),
              "bl:%ux%u",
              (unsigned int)UI_RT.block.normal.w,
              (unsigned int)UI_RT.block.normal.h);
}

// Supprime les objets d'affichage des blocs du niveau, réinitialise la grille de solidité et les compteurs associés.
static void UI_ClearLevelBlocks(void)
{
  uint16_t i;
  uint16_t gx;
  uint16_t gy;

  for(i = 0U; i < UI_LEVEL.level_block_obj_count; i++) {
    if(UI_LEVEL.level_block_objs[i] != NULL) {
      lv_obj_delete(UI_LEVEL.level_block_objs[i]);
      UI_LEVEL.level_block_objs[i] = NULL;
    }
  }

  if(UI_SCENE.level_flag_obj != NULL) {
    lv_obj_delete(UI_SCENE.level_flag_obj);
    UI_SCENE.level_flag_obj = NULL;
  }

  if(UI_SCENE.level_fortress_obj != NULL) {
    lv_obj_delete(UI_SCENE.level_fortress_obj);
    UI_SCENE.level_fortress_obj = NULL;
  }

  UI_LEVEL.level_block_obj_count = 0U;
  UI_LEVEL.solid_grid_max_x = 0U;
  UI_LEVEL.solid_grid_max_y = 0U;

  for(gy = 0U; gy < APP_UI_SOLID_GRID_H; gy++) {
    for(gx = 0U; gx < APP_UI_SOLID_GRID_W; gx++) {
      UI_LEVEL.solid_grid[gy][gx] = 0U;
    }
  }
}


// Reconstruit la grille de solidité du niveau à partir des blocs actuellement chargés, et met à jour les compteurs de dimensions
static void UI_RebuildSolidGrid(void)
{
  uint16_t i;

  UI_LEVEL.solid_grid_max_x = 0U;
  UI_LEVEL.solid_grid_max_y = 0U;

  for(i = 0U; i < UI_LEVEL.level_block_count; i++) {
    int32_t gx;
    int32_t gy;

    if(UI_IsSolidBlock(&UI_LEVEL.level_blocks[i]) == 0U) {
      continue;
    }

    gx = (int32_t)UI_LEVEL.level_blocks[i].grid_x;
    gy = (int32_t)UI_LEVEL.level_blocks[i].grid_y;
    if((gx < 0) || (gy < 0) || (gx >= (int32_t)APP_UI_SOLID_GRID_W) || (gy >= (int32_t)APP_UI_SOLID_GRID_H)) {
      continue;
    }

    UI_LEVEL.solid_grid[gy][gx] = 1U;
    if((uint16_t)gx > UI_LEVEL.solid_grid_max_x) {
      UI_LEVEL.solid_grid_max_x = (uint16_t)gx;
    }
    if((uint16_t)gy > UI_LEVEL.solid_grid_max_y) {
      UI_LEVEL.solid_grid_max_y = (uint16_t)gy;
    }
  }
}


// Créé les objets d'affichage des blocs du niveau à partir de la liste de blocs chargés, et positionne ces objets en fonction de la grille.
static lv_obj_t * UI_CreateStyledImageObject(lv_obj_t * parent,
                                             lv_draw_buf_t * src,
                                             uint16_t w,
                                             uint16_t h,
                                             int32_t x,
                                             int32_t y)
{
  lv_obj_t * obj;

  obj = lv_image_create(parent);
  if(obj == NULL) {
    return NULL;
  }

  lv_obj_remove_style_all(obj);
  lv_image_set_src(obj, src);
  lv_obj_set_style_opa(obj, LV_OPA_COVER, 0);
  lv_obj_set_style_image_opa(obj, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_opa(obj, LV_OPA_TRANSP, 0);
  lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_size(obj, w, h);
  lv_image_set_inner_align(obj, LV_IMAGE_ALIGN_CENTER);
  lv_obj_set_pos(obj, x, y);
  return obj;
}


// Calcule les dimensions du monde en fonction de la position des blocs du niveau, et retourne ces dimensions ainsi que la coordonnée de la grille la plus à droite.
static AppUiWorldBounds_t UI_ComputeWorldBounds(const AppUI_LevelBlock * blocks, uint16_t count)
{
  AppUiWorldBounds_t bounds;
  uint16_t i;

  bounds.world_w = (int32_t)LCD_HOR_RES;
  bounds.world_h = (int32_t)LCD_VER_RES;
  bounds.max_grid_x = 0;

  for(i = 0U; (i < count) && (i < APP_UI_MAX_LEVEL_BLOCKS); i++) {
    int32_t right;
    int32_t bottom;

    right = ((int32_t)blocks[i].grid_x + 2) * (int32_t)APP_UI_BLOCK_GRID_PX;
    bottom = ((int32_t)blocks[i].grid_y + 2) * (int32_t)APP_UI_BLOCK_GRID_PX;
    if(right > bounds.world_w) {
      bounds.world_w = right;
    }
    if(bottom > bounds.world_h) {
      bounds.world_h = bottom;
    }
    if((int32_t)blocks[i].grid_x > bounds.max_grid_x) {
      bounds.max_grid_x = (int32_t)blocks[i].grid_x;
    }
  }

  return bounds;
}
// Retourne les données d'affichage (buffer, dimensions) correspondant à un type de bloc donné, ou des valeurs nulles si le type n'est pas reconnu ou si les données ne sont pas chargées.
static AppUiBlockRenderInfo_t UI_GetBlockRenderInfo(uint8_t block_type)
{
  AppUiBlockRenderInfo_t info;

  info.src = NULL;
  info.w = 0U;
  info.h = 0U;

  switch(block_type) {
    case APP_UI_BLOCK_NORMAL:
      info.src = UI_SCENE.block_draw_buf;
      info.w = UI_RT.block.normal.w;
      info.h = UI_RT.block.normal.h;
      break;
    case APP_UI_BLOCK_BROKEN:
      info.src = UI_SCENE.block_broken_draw_buf;
      info.w = UI_RT.block.broken.w;
      info.h = UI_RT.block.broken.h;
      break;
    case APP_UI_BLOCK_GROUND:
      info.src = UI_SCENE.block_ground_draw_buf;
      info.w = UI_RT.block.ground.w;
      info.h = UI_RT.block.ground.h;
      break;
    case APP_UI_BLOCK_QUESTION:
      info.src = UI_SCENE.block_question_draw_buf;
      info.w = UI_RT.block.question.w;
      info.h = UI_RT.block.question.h;
      break;
    case APP_UI_BLOCK_PIPE:
      info.src = UI_SCENE.block_pipe_draw_buf;
      info.w = UI_RT.block.pipe.w;
      info.h = UI_RT.block.pipe.h;
      break;
    case APP_UI_BLOCK_GOOMBA:
      info.src = UI_SCENE.block_goomba_draw_buf;
      info.w = UI_RT.block.goomba.w;
      info.h = UI_RT.block.goomba.h;
      break;
    default:
      break;
  }

  return info;
}


// Rend les blocs du niveau en créant des objets d'affichage pour chaque bloc de la liste, et positionne ces objets en fonction de la grille. Met également à jour les dimensions du monde et la position du flag de fin de niveau.
static void UI_RenderLevelBlocks(const AppUI_LevelBlock * blocks, uint16_t count)
{
  uint16_t i;
  AppUiWorldBounds_t bounds;

  if((UI_SCENE.blocks_layer == NULL) || (UI_SCENE.block_draw_buf == NULL) || (UI_SCENE.block_broken_draw_buf == NULL)) {
    return;
  }

  UI_ClearLevelBlocks();

  if(blocks == NULL) {
    return;
  }

  bounds = UI_ComputeWorldBounds(blocks, count);

  UI_RT.level_end_world_x = (bounds.max_grid_x + 3) * (int32_t)APP_UI_BLOCK_GRID_PX;
  if((UI_RT.level_end_world_x + (int32_t)UI_RT.level_flag.w) > bounds.world_w) {
    bounds.world_w = UI_RT.level_end_world_x + (int32_t)UI_RT.level_flag.w + (int32_t)APP_UI_BLOCK_GRID_PX;
  }
  if((UI_RT.level_end_world_x + (int32_t)UI_RT.level_fortress.w) > bounds.world_w) {
    bounds.world_w = UI_RT.level_end_world_x + (int32_t)UI_RT.level_fortress.w + (int32_t)APP_UI_BLOCK_GRID_PX;
  }

  lv_obj_set_size(UI_SCENE.blocks_layer, bounds.world_w, bounds.world_h);
  lv_obj_set_pos(UI_SCENE.blocks_layer, 0, 0);

  for(i = 0U; (i < count) && (UI_LEVEL.level_block_obj_count < APP_UI_MAX_LEVEL_BLOCKS); i++) {
    AppUiBlockRenderInfo_t info;
    int32_t px;
    int32_t py;

    info = UI_GetBlockRenderInfo(blocks[i].type);
    if(info.src == NULL) {
      continue;
    }

    px = (int32_t)blocks[i].grid_x * (int32_t)APP_UI_BLOCK_GRID_PX;
    py = (int32_t)blocks[i].grid_y * (int32_t)APP_UI_BLOCK_GRID_PX;
    UI_LEVEL.level_block_objs[UI_LEVEL.level_block_obj_count] = UI_CreateStyledImageObject(UI_SCENE.blocks_layer,
                                                                                             info.src,
                                                                                             info.w,
                                                                                             info.h,
                                                                                             px,
                                                                                             py);
    if(UI_LEVEL.level_block_objs[UI_LEVEL.level_block_obj_count] == NULL) {
      continue;
    }
    UI_LEVEL.level_block_obj_count++;
  }

  UI_RebuildSolidGrid();

  if(UI_SCENE.level_flag_draw_buf != NULL) {
    int32_t flag_y;

    flag_y = (8 * (int32_t)APP_UI_BLOCK_GRID_PX) - (int32_t)UI_RT.level_flag.h;
    if(flag_y < 0) {
      flag_y = 0;
    }
    UI_SCENE.level_flag_obj = UI_CreateStyledImageObject(UI_SCENE.blocks_layer,
                                                         UI_SCENE.level_flag_draw_buf,
                                                         UI_RT.level_flag.w,
                                                         UI_RT.level_flag.h,
                                                         UI_RT.level_end_world_x,
                                                         flag_y);
  }

  if(UI_SCENE.level_fortress_draw_buf != NULL) {
    int32_t fortress_x;
    int32_t fortress_y;

    fortress_x = UI_RT.level_end_world_x + (((int32_t)UI_RT.level_flag.w - (int32_t)UI_RT.level_fortress.w) / 2);
    if(fortress_x < 0) {
      fortress_x = 0;
    }
    fortress_y = (9 * (int32_t)APP_UI_BLOCK_GRID_PX) - (int32_t)UI_RT.level_fortress.h;
    if(fortress_y < 0) {
      fortress_y = 0;
    }
    UI_SCENE.level_fortress_obj = UI_CreateStyledImageObject(UI_SCENE.blocks_layer,
                                                             UI_SCENE.level_fortress_draw_buf,
                                                             UI_RT.level_fortress.w,
                                                             UI_RT.level_fortress.h,
                                                             fortress_x,
                                                             fortress_y);
  }

  UI_UpdateWorldScroll();
}


// Met à jour la position de la couche d'affichage des blocs en fonction du décalage de scroll du monde, pour créer l'effet de déplacement du monde lorsque le joueur se déplace.
void UI_UpdateWorldScroll(void)
{
  if(UI_SCENE.blocks_layer == NULL) {
    return;
  }

  if(UI_RT.world_scroll_applied_x != UI_RT.world_scroll_x) {
    lv_obj_set_pos(UI_SCENE.blocks_layer, -UI_RT.world_scroll_x, 0);
    UI_RT.world_scroll_applied_x = UI_RT.world_scroll_x;
  }
}

// Retourne 1 si le bloc est solide ou 0 sinon.
static uint8_t UI_IsSolidBlock(const AppUI_LevelBlock * block)
{
  if(block == NULL) {
    return 0U;
  }

  return (block->type != APP_UI_BLOCK_BROKEN) ? 1U : 0U;
}

// Initialise la scène LVGL en créant les objets d'affichage pour le background, le joueur et les blocs du niveau, et en positionnant ces objets sur l'écran.
static void UI_InitLvglScene(void)
{
  uint32_t * layer1_fb;
  uint32_t layer1_px_count;
  uint32_t idx;
  lv_obj_t * bg_screen;
  lv_obj_t * fg_screen;
  int32_t player_x;
  int32_t player_y;

  lv_init();

  layer1_fb = (uint32_t *)FRAMEBUFFER_LAYER1_ADDR;
  layer1_px_count = FRAMEBUFFER_LAYER1_SIZE / sizeof(uint32_t);
  for(idx = 0U; idx < layer1_px_count; idx++) {
    layer1_fb[idx] = 0x00000000U;
  }

  UI_SCENE.background_display = lv_st_ltdc_create_direct((void *)FRAMEBUFFER_LAYER0_ADDR, NULL, 0U);
  UI_SCENE.foreground_display = lv_st_ltdc_create_direct((void *)FRAMEBUFFER_LAYER1_ADDR, NULL, 1U);
  if((UI_SCENE.background_display == NULL) || (UI_SCENE.foreground_display == NULL)) {
    Error_Handler();
  }

  lv_display_set_rotation(UI_SCENE.background_display, LV_DISPLAY_ROTATION_0);
  lv_display_set_color_format(UI_SCENE.background_display, LV_COLOR_FORMAT_RGB565);
  lv_display_set_rotation(UI_SCENE.foreground_display, LV_DISPLAY_ROTATION_0);
  lv_display_set_color_format(UI_SCENE.foreground_display, LV_COLOR_FORMAT_ARGB8888);
  lv_display_set_default(UI_SCENE.foreground_display);

  bg_screen = lv_display_get_screen_active(UI_SCENE.background_display);
  fg_screen = lv_display_get_screen_active(UI_SCENE.foreground_display);

  lv_obj_set_style_bg_opa(bg_screen, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(bg_screen, lv_color_white(), 0);
  lv_obj_set_style_bg_opa(fg_screen, LV_OPA_TRANSP, 0);

  UI_SCENE.background_layer = lv_obj_create(bg_screen);
  lv_obj_remove_style_all(UI_SCENE.background_layer);
  lv_obj_set_size(UI_SCENE.background_layer, LCD_HOR_RES, LCD_VER_RES);
  lv_obj_set_pos(UI_SCENE.background_layer, 0, 0);
  lv_obj_set_style_bg_opa(UI_SCENE.background_layer, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(UI_SCENE.background_layer, lv_color_white(), 0);
  lv_obj_clear_flag(UI_SCENE.background_layer, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_move_background(UI_SCENE.background_layer);
  UI_CreateBackgroundObject(UI_SCENE.background_layer);

  UI_LoadPlayerSprite();
  UI_LoadBlockSprites();

  UI_SCENE.player_layer = lv_obj_create(fg_screen);
  lv_obj_remove_style_all(UI_SCENE.player_layer);
  lv_obj_set_size(UI_SCENE.player_layer, LCD_HOR_RES, LCD_VER_RES);
  lv_obj_set_pos(UI_SCENE.player_layer, 0, 0);
  lv_obj_set_style_bg_opa(UI_SCENE.player_layer, LV_OPA_TRANSP, 0);
  lv_obj_set_style_opa(UI_SCENE.player_layer, LV_OPA_COVER, 0);
  lv_obj_clear_flag(UI_SCENE.player_layer, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_move_foreground(UI_SCENE.player_layer);

  UI_SCENE.player_motion_layer = lv_obj_create(UI_SCENE.player_layer);
  lv_obj_remove_style_all(UI_SCENE.player_motion_layer);
  lv_obj_set_size(UI_SCENE.player_motion_layer, UI_RT.player.box.w, UI_RT.player.box.h);
  lv_obj_set_pos(UI_SCENE.player_motion_layer, 0, 0);
  lv_obj_set_style_bg_opa(UI_SCENE.player_motion_layer, LV_OPA_TRANSP, 0);
  lv_obj_set_style_opa(UI_SCENE.player_motion_layer, LV_OPA_COVER, 0);
  lv_obj_clear_flag(UI_SCENE.player_motion_layer, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_move_foreground(UI_SCENE.player_motion_layer);

  UI_SCENE.blocks_layer = lv_obj_create(UI_SCENE.player_layer);
  lv_obj_remove_style_all(UI_SCENE.blocks_layer);
  lv_obj_set_size(UI_SCENE.blocks_layer, LCD_HOR_RES, LCD_VER_RES);
  lv_obj_set_pos(UI_SCENE.blocks_layer, 0, 0);
  lv_obj_set_style_bg_opa(UI_SCENE.blocks_layer, LV_OPA_TRANSP, 0);
  lv_obj_set_style_opa(UI_SCENE.blocks_layer, LV_OPA_COVER, 0);
  lv_obj_clear_flag(UI_SCENE.blocks_layer, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_move_background(UI_SCENE.blocks_layer);

  UI_RenderLevelBlocks(g_default_level_blocks, g_default_level_block_count);

  UI_CreatePlayerSprites();

  player_x = ((int32_t)LCD_HOR_RES - (int32_t)UI_RT.player.box.w) / 2;
  if(player_x < 0) {
    player_x = 0;
  }

  player_y = (int32_t)LCD_VER_RES - (int32_t)UI_RT.player.box.h - 30;
  if(player_y < 0) {
    player_y = 0;
  }

  lv_obj_set_pos(UI_SCENE.player_motion_layer, player_x, player_y);
  UI_RT.player_x = player_x;
  UI_RT.player_y = player_y;
  UI_RT.world_scroll_applied_x = UI_RT.world_scroll_x - 1;
  UI_UpdateWorldScroll();

  UI_SCENE.debug_label = NULL;

  UI_SCENE.game_over_label = lv_label_create(UI_SCENE.player_layer);
  lv_obj_set_style_text_color(UI_SCENE.game_over_label, lv_color_white(), 0);
  lv_obj_set_style_bg_color(UI_SCENE.game_over_label, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(UI_SCENE.game_over_label, LV_OPA_80, 0);
  lv_obj_set_style_pad_hor(UI_SCENE.game_over_label, 10, 0);
  lv_obj_set_style_pad_ver(UI_SCENE.game_over_label, 6, 0);
  lv_obj_set_style_border_opa(UI_SCENE.game_over_label, LV_OPA_TRANSP, 0);
  lv_label_set_text(UI_SCENE.game_over_label, "GAME OVER\nBP1/BP2 pour reprendre");
  lv_obj_center(UI_SCENE.game_over_label);
  lv_obj_add_flag(UI_SCENE.game_over_label, LV_OBJ_FLAG_HIDDEN);

  UI_SCENE.level_complete_label = lv_label_create(UI_SCENE.player_layer);
  lv_obj_set_style_text_color(UI_SCENE.level_complete_label, lv_color_white(), 0);
  lv_obj_set_style_bg_color(UI_SCENE.level_complete_label, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(UI_SCENE.level_complete_label, LV_OPA_80, 0);
  lv_obj_set_style_pad_hor(UI_SCENE.level_complete_label, 10, 0);
  lv_obj_set_style_pad_ver(UI_SCENE.level_complete_label, 6, 0);
  lv_obj_set_style_border_opa(UI_SCENE.level_complete_label, LV_OPA_TRANSP, 0);
  lv_label_set_text(UI_SCENE.level_complete_label, "Bravo ! Niveau termine\nAppuie sur BP1 pour redemarrer");
  lv_obj_center(UI_SCENE.level_complete_label);
  lv_obj_add_flag(UI_SCENE.level_complete_label, LV_OBJ_FLAG_HIDDEN);

  lv_obj_invalidate(bg_screen);
  lv_obj_invalidate(fg_screen);
  lv_timer_handler();
}

void AppUI_Init(void)
{
  Storage_Init();
  UI_InitLvglScene();
}

void AppUI_SetLevelBlocks(const AppUI_LevelBlock * blocks, uint16_t count)
{
  uint16_t i;

  UI_LEVEL.level_block_count = 0U;
  if(blocks != NULL) {
    for(i = 0U; (i < count) && (i < APP_UI_MAX_LEVEL_BLOCKS); i++) {
      UI_LEVEL.level_blocks[UI_LEVEL.level_block_count] = blocks[i];
      UI_LEVEL.level_block_count++;
    }
  }

  UI_RT.level_completed = 0U;
  AppUI_ShowLevelComplete(0U);

  UI_RenderLevelBlocks(blocks, count);
}

void AppUI_PlayerSetAnimation(uint8_t moving, uint8_t jumping, uint8_t anim_phase)
{
  (void)anim_phase;

  if(jumping != 0U) {
    UI_SetPlayerSpriteState(APP_UI_PLAYER_SPRITE_JUMP);
    return;
  }

  if(moving != 0U) {
    UI_SetPlayerSpriteState(APP_UI_PLAYER_SPRITE_WALK);
    return;
  }

  UI_SetPlayerSpriteState(APP_UI_PLAYER_SPRITE_IDLE);
}
