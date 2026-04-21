#ifndef APP_UI_PLAYER_BACKEND_H
#define APP_UI_PLAYER_BACKEND_H

#include <stdint.h>

void AppUI_PlayerSetAnimation(uint8_t moving, uint8_t jumping, uint8_t anim_phase);
void AppUI_PlayerMoveBy(int16_t dx, int16_t dy);
void AppUI_PlayerGetPosition(int16_t * x, int16_t * y);
int16_t AppUI_PlayerGetGroundY(void);
uint8_t AppUI_PlayerIsOnGround(void);
void AppUI_ResetPlayerPosition(void);
void AppUI_ShowGameOver(uint8_t visible);
uint8_t AppUI_IsLevelCompleted(void);
void AppUI_ShowLevelComplete(uint8_t visible);
void AppUI_ResetLevelState(void);

#endif /* APP_UI_PLAYER_BACKEND_H */
