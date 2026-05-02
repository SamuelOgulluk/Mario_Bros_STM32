#ifndef APP_AUDIO_TYPES_H
#define APP_AUDIO_TYPES_H

#include "ff.h"
#include <stdint.h>

typedef struct {
  uint32_t data_offset;
  uint32_t data_size;
  uint32_t data_remaining;
  uint32_t sample_rate_hz;
  uint16_t num_channels;
  uint16_t bits_per_sample;
  uint16_t block_align;
  uint16_t io_index;
  uint16_t io_count;
} AppAudioWavState_t;

typedef struct {
  uint8_t running;
  uint8_t tone_mode;
  volatile uint8_t need_fill_half0;
  volatile uint8_t need_fill_half1;
  volatile uint8_t dma_error_pending;
  uint16_t volume_q8;
  uint32_t tone_period_samples;
  uint32_t tone_sample_index;
  char status[32];
} AppAudioRuntime_t;

typedef struct {
  FIL wav_file;
  uint8_t io_buffer[APP_AUDIO_IO_BUFFER_SIZE];
  AppAudioWavState_t wav;
  AppAudioRuntime_t runtime;
} AppAudioContext_t;

#endif /* APP_AUDIO_TYPES_H */
