#ifndef APP_AUDIO_TYPES_H
#define APP_AUDIO_TYPES_H

#include "ff.h"
#include <stdint.h>

typedef struct {
  uint32_t data_offset;
  uint32_t data_size;
  uint32_t data_remaining;
  uint32_t sample_rate_hz;
  uint16_t io_index;
  uint16_t io_count;
} AppAudioWavState_t;

typedef struct {
  uint8_t running;
  uint8_t tone_mode;
  uint8_t need_fill_half0;
  uint8_t need_fill_half1;
  volatile uint8_t dma_error_pending;
  uint32_t tone_period_samples;
  uint32_t tone_sample_index;
  char status[32];
} AppAudioRuntime_t;

typedef struct {
  FIL wav_file;
  uint16_t dac_dma_buffer[APP_AUDIO_DMA_SAMPLES];
  uint8_t io_buffer[APP_AUDIO_IO_BUFFER_SIZE];
  AppAudioWavState_t wav;
  AppAudioRuntime_t runtime;
} AppAudioContext_t;

#endif /* APP_AUDIO_TYPES_H */
