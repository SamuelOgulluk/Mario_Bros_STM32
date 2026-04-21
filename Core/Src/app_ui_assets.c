#include "app_ui_assets.h"

#include "ff.h"

static uint16_t ReadU16LE(const uint8_t * data)
{
  return (uint16_t)data[0] | ((uint16_t)data[1] << 8);
}

static uint32_t ReadU32LE(const uint8_t * data)
{
  return (uint32_t)data[0] | ((uint32_t)data[1] << 8) | ((uint32_t)data[2] << 16) | ((uint32_t)data[3] << 24);
}

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
                               const char * status_prefix)
{
  uint8_t header[54];
  uint8_t * row_buffer;
  lv_draw_buf_t * bmp_buf;
  FIL file;
  uint32_t pixel_offset;
  uint32_t width;
  int32_t height;
  uint32_t abs_height;
  uint32_t row_padded_bytes;
  uint32_t dib_header_size;
  uint32_t compression;
  uint32_t palette_entries;
  uint32_t palette_offset;
  uint16_t bits_per_pixel;
  uint32_t x;
  uint32_t y;
  uint8_t palette[256U * 4U];
  const uint8_t * src_px;
  UINT bytes_read;
  FRESULT fr;

  if(((out_buf == NULL) && (fixed_buf == NULL)) || (out_width == NULL) || (out_height == NULL) || (status_text == NULL) || (status_prefix == NULL)) {
    return 0U;
  }

  if(out_buf != NULL) {
    *out_buf = NULL;
  }

  fr = f_open(&file, path, FA_READ);
  if(fr != FR_OK) {
    lv_snprintf(status_text, status_text_len, "%s:open err", status_prefix);
    return 0U;
  }

  bytes_read = 0U;
  if(f_read(&file, header, sizeof(header), &bytes_read) != FR_OK || bytes_read != sizeof(header)) {
    (void)f_close(&file);
    lv_snprintf(status_text, status_text_len, "%s:hdr err", status_prefix);
    return 0U;
  }

  if((header[0] != 'B') || (header[1] != 'M')) {
    (void)f_close(&file);
    lv_snprintf(status_text, status_text_len, "%s:not bmp", status_prefix);
    return 0U;
  }

  pixel_offset = ReadU32LE(&header[10]);
  dib_header_size = ReadU32LE(&header[14]);
  width = ReadU32LE(&header[18]);
  height = (int32_t)ReadU32LE(&header[22]);
  compression = ReadU32LE(&header[30]);
  bits_per_pixel = ReadU16LE(&header[28]);
  palette_entries = ReadU32LE(&header[46]);

  if((width == 0U) || (height == 0)) {
    (void)f_close(&file);
    lv_snprintf(status_text, status_text_len, "%s:size err", status_prefix);
    return 0U;
  }

  if((bits_per_pixel != 4U) &&
     (bits_per_pixel != 8U) &&
     (bits_per_pixel != 24U) &&
     (bits_per_pixel != 32U)) {
    (void)f_close(&file);
    lv_snprintf(status_text, status_text_len, "%s:bpp err", status_prefix);
    return 0U;
  }

  if(compression != 0U) {
    (void)f_close(&file);
    lv_snprintf(status_text, status_text_len, "%s:cmp err", status_prefix);
    return 0U;
  }

  if((bits_per_pixel == 4U) || (bits_per_pixel == 8U)) {
    if(palette_entries == 0U) {
      palette_entries = 1UL << bits_per_pixel;
    }
    if(palette_entries > 256U) {
      (void)f_close(&file);
      lv_snprintf(status_text, status_text_len, "%s:pal err", status_prefix);
      return 0U;
    }

    palette_offset = 14U + dib_header_size;
    if(f_lseek(&file, palette_offset) != FR_OK) {
      (void)f_close(&file);
      lv_snprintf(status_text, status_text_len, "%s:pal seek", status_prefix);
      return 0U;
    }

    bytes_read = 0U;
    if((f_read(&file, palette, palette_entries * 4U, &bytes_read) != FR_OK) ||
       (bytes_read != (palette_entries * 4U))) {
      (void)f_close(&file);
      lv_snprintf(status_text, status_text_len, "%s:pal read", status_prefix);
      return 0U;
    }
  }

  abs_height = (height < 0) ? (uint32_t)(-height) : (uint32_t)height;
  row_padded_bytes = ((width * (uint32_t)bits_per_pixel + 31U) / 32U) * 4U;

  if(fixed_buf != NULL) {
    uint32_t required_size;

    required_size = LV_DRAW_BUF_SIZE(width, abs_height, out_color_format);
    if(required_size > fixed_data_size) {
      (void)f_close(&file);
      lv_snprintf(status_text, status_text_len, "%s:buf err", status_prefix);
      return 0U;
    }

    if(lv_draw_buf_init(fixed_buf,
                        width,
                        abs_height,
                        out_color_format,
                        LV_STRIDE_AUTO,
                        fixed_data,
                        required_size) != LV_RESULT_OK) {
      (void)f_close(&file);
      lv_snprintf(status_text, status_text_len, "%s:buf err", status_prefix);
      return 0U;
    }

    bmp_buf = fixed_buf;
  }
  else {
    bmp_buf = lv_draw_buf_create(width, abs_height, out_color_format, LV_STRIDE_AUTO);
    if(bmp_buf == NULL) {
      (void)f_close(&file);
      lv_snprintf(status_text, status_text_len, "%s:buf err", status_prefix);
      return 0U;
    }
  }

  row_buffer = lv_malloc(row_padded_bytes);
  if(row_buffer == NULL) {
    if(fixed_buf == NULL) {
      lv_draw_buf_destroy(bmp_buf);
    }
    (void)f_close(&file);
    lv_snprintf(status_text, status_text_len, "%s:mem err", status_prefix);
    return 0U;
  }

  for(y = 0U; y < abs_height; y++) {
    uint32_t bmp_y;
    uint16_t * dst_row_565;
    lv_color32_t * dst_row_8888;

    bmp_y = (height < 0) ? y : (abs_height - 1U - y);
    if(f_lseek(&file, pixel_offset + (bmp_y * row_padded_bytes)) != FR_OK) {
      lv_free(row_buffer);
      if(fixed_buf == NULL) {
        lv_draw_buf_destroy(bmp_buf);
      }
      (void)f_close(&file);
      lv_snprintf(status_text, status_text_len, "%s:seek err", status_prefix);
      return 0U;
    }

    bytes_read = 0U;
    if(f_read(&file, row_buffer, row_padded_bytes, &bytes_read) != FR_OK || bytes_read != row_padded_bytes) {
      lv_free(row_buffer);
      if(fixed_buf == NULL) {
        lv_draw_buf_destroy(bmp_buf);
      }
      (void)f_close(&file);
      lv_snprintf(status_text, status_text_len, "%s:read err", status_prefix);
      return 0U;
    }

    dst_row_565 = NULL;
    dst_row_8888 = NULL;

    if(out_color_format == LV_COLOR_FORMAT_ARGB8888) {
      dst_row_8888 = (lv_color32_t *)lv_draw_buf_goto_xy(bmp_buf, 0U, y);
      if(dst_row_8888 == NULL) {
        lv_free(row_buffer);
        if(fixed_buf == NULL) {
          lv_draw_buf_destroy(bmp_buf);
        }
        (void)f_close(&file);
        lv_snprintf(status_text, status_text_len, "%s:dst err", status_prefix);
        return 0U;
      }
    }
    else {
      dst_row_565 = (uint16_t *)lv_draw_buf_goto_xy(bmp_buf, 0U, y);
      if(dst_row_565 == NULL) {
        lv_free(row_buffer);
        if(fixed_buf == NULL) {
          lv_draw_buf_destroy(bmp_buf);
        }
        (void)f_close(&file);
        lv_snprintf(status_text, status_text_len, "%s:dst err", status_prefix);
        return 0U;
      }
    }

    for(x = 0U; x < width; x++) {
      uint8_t b;
      uint8_t g;
      uint8_t r;
      uint8_t a;
      uint32_t pal_index;

      if(bits_per_pixel == 24U) {
        src_px = &row_buffer[x * 3U];
        b = src_px[0];
        g = src_px[1];
        r = src_px[2];
      }
      else if(bits_per_pixel == 32U) {
        src_px = &row_buffer[x * 4U];
        b = src_px[0];
        g = src_px[1];
        r = src_px[2];
      }
      else {
        if(bits_per_pixel == 8U) {
          pal_index = row_buffer[x];
        }
        else {
          uint8_t packed = row_buffer[x / 2U];
          pal_index = ((x & 1U) == 0U) ? ((packed >> 4) & 0x0FU) : (packed & 0x0FU);
        }

        if(pal_index >= palette_entries) {
          b = 0U;
          g = 0U;
          r = 0U;
        }
        else {
          b = palette[(pal_index * 4U) + 0U];
          g = palette[(pal_index * 4U) + 1U];
          r = palette[(pal_index * 4U) + 2U];
        }
      }

      if((black_is_transparent != 0U) && (r == 0U) && (g == 0U) && (b == 0U)) {
        a = 0U;
      }
      else {
        a = 255U;
      }

      if(out_color_format == LV_COLOR_FORMAT_ARGB8888) {
        dst_row_8888[x].blue = b;
        dst_row_8888[x].green = g;
        dst_row_8888[x].red = r;
        dst_row_8888[x].alpha = a;
      }
      else {
        dst_row_565[x] = (uint16_t)((((uint16_t)r & 0xF8U) << 8) |
                                    (((uint16_t)g & 0xFCU) << 3) |
                                    (((uint16_t)b & 0xF8U) >> 3));
      }
    }

  }

  lv_free(row_buffer);
  (void)f_close(&file);

  if(out_buf != NULL) {
    *out_buf = bmp_buf;
  }
  *out_width = (uint16_t)width;
  *out_height = (uint16_t)abs_height;
  lv_snprintf(status_text,
              status_text_len,
              "%s:ram %ux%u",
              status_prefix,
              (unsigned int)*out_width,
              (unsigned int)*out_height);
  return 1U;
}
