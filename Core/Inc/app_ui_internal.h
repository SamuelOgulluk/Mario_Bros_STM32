#ifndef APP_UI_INTERNAL_H
#define APP_UI_INTERNAL_H

#include "app_ui.h"
#include "app_ui_private.h"

extern AppUiContext_t g_ui;
extern const AppUI_LevelBlock g_default_level_blocks[];
extern const uint16_t g_default_level_block_count;

void UI_UpdateWorldScroll(void);
void UI_ApplyPlayerFacingDirection(void);

#endif /* APP_UI_INTERNAL_H */
