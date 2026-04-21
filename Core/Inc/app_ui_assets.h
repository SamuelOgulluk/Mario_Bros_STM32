#ifndef APP_UI_ASSETS_H
#define APP_UI_ASSETS_H

#include <stdint.h>
#include <stddef.h>

#include "lvgl.h"

uint8_t AppUI_LoadBmpToDrawBuf(const char * path,
                               lv_draw_buf_t * fixed_buf,
                               void * fixed_data,
                               uint32_t fixed_data_size,
                               lv_draw_buf_t ** out_buf,
                               uint16_t * out_width,
                               uint16_t * out_height,
                               lv_color_format_t out_color_format,
                               uint8_t black_is_transparent,
                               char * status_text,
                               size_t status_text_len,
                               const char * status_prefix);

#endif /* APP_UI_ASSETS_H */
