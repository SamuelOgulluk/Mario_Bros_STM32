/*
 * app_ui.h
 * Public UI API for the game screen.
 *
 * This module owns LVGL scene creation, background/sprite loading,
 * and runtime player/UI updates used by the game loop.
 */

#ifndef APP_UI_H
#define APP_UI_H

#include <stdint.h>
#include "ff.h"
#include "lvgl.h"

#define APP_UI_BLOCK_GRID_PX 31

#define LCD_HOR_RES 480U
#define LCD_VER_RES 272U
#define PLAYER_SCROLL_MIN_X ((int32_t)(LCD_HOR_RES / 4U))
#define PLAYER_SCROLL_MAX_X ((int32_t)((LCD_HOR_RES * 3U) / 4U))
#define PLAYER_SCROLL_STEP_PX (8 * (int32_t)APP_UI_BLOCK_GRID_PX)
#define PLAYER_COLLIDER_MARGIN_X 2U
#define PLAYER_COLLIDER_MARGIN_TOP 2U
#define PLAYER_COLLIDER_MARGIN_BOTTOM 1U
#define PLAYER_HORIZONTAL_COLLISION_LIFT_PX 4

#define FRAMEBUFFER_LAYER0_ADDR 0xC0000000U
#define FRAMEBUFFER_LAYER1_ADDR 0xC0080000U
#define FRAMEBUFFER_LAYER1_SIZE (LCD_HOR_RES * LCD_VER_RES * 4U)
#define BACKGROUND_DRAW_BUF_ADDR 0xC0100000U
#define BACKGROUND_DRAW_BUF_SIZE (4U * 1024U * 1024U)

#define BACKGROUND_IMAGE_FATFS_PATH "0:/sprites/mario_bg.bmp"
#define PLAYER_IDLE_IMAGE_FATFS_PATH "0:/sprites/Super_Mario_Sprite.bmp"
#define PLAYER_WALK_IMAGE_FATFS_PATH "0:/sprites/Mario_walking_sprite.bmp"
#define PLAYER_JUMP_IMAGE_FATFS_PATH "0:/sprites/Super_Mario_Jumping.bmp"
#define BLOCK_IMAGE_FATFS_PATH "0:/sprites/Brick_Block_Sprite.bmp"
#define BLOCK_BROKEN_IMAGE_FATFS_PATH "0:/sprites/SMB1_Empty_Block.bmp"

#define BLOCK_GROUND_IMAGE_FATFS_PATH "0:/sprites/Ground.bmp"
#define BLOCK_QUESTION_IMAGE_FATFS_PATH "0:/sprites/Qblock.bmp"
#define BLOCK_PIPE_IMAGE_FATFS_PATH "0:/sprites/Warp_Pipe.bmp"
#define BLOCK_GOOMBA_IMAGE_FATFS_PATH "0:/sprites/Goomba.bmp"
#define LEVEL_FLAG_IMAGE_FATFS_PATH "0:/sprites/fortress_flag.bmp"
#define LEVEL_FORTRESS_IMAGE_FATFS_PATH "0:/sprites/fortress.bmp"
#define APP_UI_MAX_LEVEL_BLOCKS 512U
#define APP_UI_SOLID_GRID_W 512U
#define APP_UI_SOLID_GRID_H 64U
#define APP_UI_PLAYER_BOX_DEFAULT_W 32U
#define APP_UI_PLAYER_BOX_DEFAULT_H 32U

typedef enum {
    APP_UI_BLOCK_NORMAL = 0,
    APP_UI_BLOCK_BROKEN = 1,
    APP_UI_BLOCK_GROUND = 2,
    APP_UI_BLOCK_QUESTION = 3,
    APP_UI_BLOCK_PIPE = 4,
    APP_UI_BLOCK_GOOMBA = 5
} AppUI_BlockType;

typedef struct {
    int16_t grid_x;
    int16_t grid_y;
    uint8_t type;
} AppUI_LevelBlock;

typedef enum {
    APP_UI_PLAYER_SPRITE_IDLE = 0U,
    APP_UI_PLAYER_SPRITE_WALK = 1U,
    APP_UI_PLAYER_SPRITE_JUMP = 2U,
} AppUI_PlayerSpriteState;

typedef struct {
    lv_display_t * background_display;
    lv_display_t * foreground_display;
    lv_obj_t * background_layer;
    lv_obj_t * background_image;
    lv_draw_buf_t background_draw_buf;
    lv_obj_t * player_layer;
    lv_obj_t * player_motion_layer;
    lv_obj_t * blocks_layer;
    lv_obj_t * player_idle_obj;
    lv_obj_t * player_walk_obj;
    lv_obj_t * player_jump_obj;
    lv_obj_t * player_obj;
    lv_obj_t * debug_label;
    lv_obj_t * game_over_label;
    lv_obj_t * level_complete_label;
    lv_obj_t * level_flag_obj;
    lv_obj_t * level_fortress_obj;
    lv_draw_buf_t * player_idle_draw_buf;
    lv_draw_buf_t * player_walk_draw_buf;
    lv_draw_buf_t * player_jump_draw_buf;
    lv_draw_buf_t * player_idle_draw_buf_left;
    lv_draw_buf_t * player_walk_draw_buf_left;
    lv_draw_buf_t * player_jump_draw_buf_left;
    lv_draw_buf_t * block_draw_buf;
    lv_draw_buf_t * block_broken_draw_buf;
    lv_draw_buf_t * block_ground_draw_buf;
    lv_draw_buf_t * block_question_draw_buf;
    lv_draw_buf_t * block_pipe_draw_buf;
    lv_draw_buf_t * block_goomba_draw_buf;
    lv_draw_buf_t * level_flag_draw_buf;
    lv_draw_buf_t * level_fortress_draw_buf;
} AppUiScene_t;

typedef struct {
    lv_obj_t * level_block_objs[APP_UI_MAX_LEVEL_BLOCKS];
    AppUI_LevelBlock level_blocks[APP_UI_MAX_LEVEL_BLOCKS];
    uint8_t solid_grid[APP_UI_SOLID_GRID_H][APP_UI_SOLID_GRID_W];
    uint16_t level_block_count;
    uint16_t level_block_obj_count;
    uint16_t solid_grid_max_x;
    uint16_t solid_grid_max_y;
} AppUiLevel_t;

typedef struct {
    uint16_t w;
    uint16_t h;
} AppUiDim_t;

typedef struct {
    AppUiDim_t box;
    AppUiDim_t idle;
    AppUiDim_t walk;
    AppUiDim_t jump;
} AppUiPlayerMetrics_t;

typedef struct {
    AppUiDim_t normal;
    AppUiDim_t broken;
    AppUiDim_t ground;
    AppUiDim_t question;
    AppUiDim_t pipe;
    AppUiDim_t goomba;
} AppUiBlockMetrics_t;

typedef struct {
    char debug_text[2][128];
    char bg_status_text[24];
    char sprite_status_text[24];
    char block_status_text[24];
    uint8_t debug_text_index;
    AppUiDim_t bg_size;
    AppUiPlayerMetrics_t player;
    AppUiBlockMetrics_t block;
    AppUiDim_t level_flag;
    AppUiDim_t level_fortress;
    uint8_t player_anim_state;
    uint8_t player_facing_left;
    uint8_t level_completed;
    int32_t world_scroll_x;
    int32_t world_scroll_applied_x;
    int32_t level_end_world_x;
    int32_t player_x;
    int32_t player_y;
} AppUiRuntime_t;

typedef struct {
    FATFS fatfs;
    char path[4];
    uint8_t ready;
} AppUiStorage_t;

typedef struct {
    AppUiScene_t scene;
    AppUiLevel_t level;
    AppUiRuntime_t runtime;
    AppUiStorage_t storage;
} AppUiContext_t;

typedef struct {
    int32_t world_w;
    int32_t world_h;
    int32_t max_grid_x;
} AppUiWorldBounds_t;

typedef struct {
    lv_draw_buf_t * src;
    uint16_t w;
    uint16_t h;
} AppUiBlockRenderInfo_t;

void AppUI_Init(void);
void AppUI_SetLevelBlocks(const AppUI_LevelBlock * blocks, uint16_t count);
void UI_SetDebugInput(int16_t dx, int16_t vy, uint8_t jump,
                      uint32_t a0, uint32_t a8, uint32_t a6, uint32_t a7);

void AppUI_PlayerMoveBy(int16_t dx, int16_t dy);
void AppUI_PlayerGetPosition(int16_t * x, int16_t * y);
int16_t AppUI_PlayerGetGroundY(void);
uint8_t AppUI_PlayerIsOnGround(void);
void AppUI_PlayerSetAnimation(uint8_t moving, uint8_t jumping, uint8_t anim_phase);
void AppUI_ResetPlayerPosition(void);
void AppUI_ShowGameOver(uint8_t visible);
uint8_t AppUI_IsLevelCompleted(void);
void AppUI_ShowLevelComplete(uint8_t visible);
void AppUI_ResetLevelState(void);

#endif /* APP_UI_H */
