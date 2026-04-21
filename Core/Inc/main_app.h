#ifndef MAIN_APP_H
#define MAIN_APP_H

#include "app_ui.h"

typedef struct {
  const AppUI_LevelBlock * level_blocks;
  uint16_t level_block_count;
} AppBootConfig_t;

#endif /* MAIN_APP_H */
