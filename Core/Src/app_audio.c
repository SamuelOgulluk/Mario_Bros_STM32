/*
 * app_audio.c
 * Minimal WAV player (PCM 8/16-bit mono/stereo) -> DAC + DMA + TIM2 trigger.
 */

#include "app_audio.h"
#include "main.h"
#include "adc.h"
#include "dac.h"
#include "tim.h"
#include "app_audio_types.h"
#include "freertos_app.h"
#include <stdio.h>
#include <string.h>

#define APP_AUDIO_FORCE_TONE_TEST 0U
#define APP_AUDIO_FORCE_DIRECT_DAC_TEST 0U
#define APP_AUDIO_FORCE_DMA_STATIC_TONE_TEST 1U

static AppAudioContext_t g_audio = {
  .runtime = {
    .status = "au:init",
    .volume_q8 = APP_AUDIO_VOLUME_Q8_MAX
  }
};

/* DMA-accessible buffer (not DTCM), aligned for D-Cache maintenance. */
static uint16_t g_dac_dma_buffer[APP_AUDIO_DMA_SAMPLES]
  __attribute__((section(".dma_buffer"), aligned(32)));

static void Audio_SetStatus(const char * text)
{
  (void)snprintf(g_audio.runtime.status, sizeof(g_audio.runtime.status), "%s", text);
}

static void Audio_CleanDCache(const void * addr, uint32_t size_bytes)
{
#if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
  uintptr_t start;
  uintptr_t end;
  uint32_t len;

  if((addr == NULL) || (size_bytes == 0U)) {
    return;
  }

  start = ((uintptr_t)addr) & ~((uintptr_t)31U);
  end = ((uintptr_t)addr + (uintptr_t)size_bytes + (uintptr_t)31U) & ~((uintptr_t)31U);
  len = (uint32_t)(end - start);
  SCB_CleanDCache_by_Addr((uint32_t *)start, (int32_t)len);
#else
  (void)addr;
  (void)size_bytes;
#endif
}

static uint16_t ReadU16LE(const uint8_t * p)
{
  return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t ReadU32LE(const uint8_t * p)
{
  return (uint32_t)p[0] |
         ((uint32_t)p[1] << 8) |
         ((uint32_t)p[2] << 16) |
         ((uint32_t)p[3] << 24);
}

static uint16_t ReadVolumeQ8FromPot(void)
{
  /* Keep audio path strictly DAC+TIM+DMA and avoid mute due to pot wiring/config. */
  return APP_AUDIO_VOLUME_Q8_MAX;
}

static uint16_t ApplyVolume(uint16_t sample)
{
  int32_t centered;

  if(g_audio.runtime.volume_q8 >= APP_AUDIO_VOLUME_Q8_MAX) {
    return sample;
  }

  centered = (int32_t)sample - 2048;
  centered = (centered * (int32_t)g_audio.runtime.volume_q8) / (int32_t)APP_AUDIO_VOLUME_Q8_MAX;
  centered += 2048;

  if(centered < 0) {
    return 0U;
  }
  if(centered > 4095) {
    return 4095U;
  }
  return (uint16_t)centered;
}

static void StopPlayback(void)
{
  (void)HAL_DAC_Stop_DMA(&hdac, DAC_CHANNEL_1);
  (void)HAL_TIM_Base_Stop(&htim2);
  if(g_audio.runtime.running != 0U) {
    (void)f_close(&g_audio.wav_file);
  }
  g_audio.runtime.running = 0U;
  g_audio.runtime.need_fill_half0 = 0U;
  g_audio.runtime.need_fill_half1 = 0U;
}

static void ConfigureTim2SampleRate(uint32_t sample_rate)
{
  RCC_ClkInitTypeDef clk_cfg;
  uint32_t latency;
  uint32_t pclk1;
  uint32_t timclk;
  uint32_t psc;
  uint32_t arr;

  if(sample_rate == 0U) {
    sample_rate = 8000U;
  }

  HAL_RCC_GetClockConfig(&clk_cfg, &latency);
  pclk1 = HAL_RCC_GetPCLK1Freq();
  timclk = (clk_cfg.APB1CLKDivider == RCC_HCLK_DIV1) ? pclk1 : (pclk1 * 2U);

  psc = (timclk / (sample_rate * 65536U));
  if(psc > 0xFFFFU) {
    psc = 0xFFFFU;
  }

  arr = timclk / ((psc + 1U) * sample_rate);
  if(arr == 0U) {
    arr = 1U;
  }
  arr -= 1U;

  __HAL_TIM_DISABLE(&htim2);
  __HAL_TIM_SET_PRESCALER(&htim2, psc);
  __HAL_TIM_SET_AUTORELOAD(&htim2, arr);
  __HAL_TIM_SET_COUNTER(&htim2, 0U);
  htim2.Instance->EGR = TIM_EGR_UG;
  __HAL_TIM_CLEAR_FLAG(&htim2, TIM_FLAG_UPDATE);
}

static uint8_t ParseWavHeader(FIL * file)
{
  uint8_t head[12];
  uint8_t chunk[8];
  uint8_t fmt[16];
  UINT n;
  uint8_t has_fmt = 0U;
  uint8_t has_data = 0U;

  if((file == NULL) || (f_lseek(file, 0U) != FR_OK)) {
    return 0U;
  }

  if((f_read(file, head, sizeof(head), &n) != FR_OK) || (n != sizeof(head))) {
    return 0U;
  }

  if((memcmp(head, "RIFF", 4U) != 0) || (memcmp(&head[8], "WAVE", 4U) != 0)) {
    return 0U;
  }

  while((has_fmt == 0U) || (has_data == 0U)) {
    uint32_t chunk_size;

    if((f_read(file, chunk, sizeof(chunk), &n) != FR_OK) || (n != sizeof(chunk))) {
      return 0U;
    }
    chunk_size = ReadU32LE(&chunk[4]);

    if(memcmp(chunk, "fmt ", 4U) == 0) {
      if((chunk_size < sizeof(fmt)) || (f_read(file, fmt, sizeof(fmt), &n) != FR_OK) || (n != sizeof(fmt))) {
        return 0U;
      }

      g_audio.wav.num_channels = ReadU16LE(&fmt[2]);
      g_audio.wav.sample_rate_hz = ReadU32LE(&fmt[4]);
      g_audio.wav.bits_per_sample = ReadU16LE(&fmt[14]);
      g_audio.wav.block_align = ReadU16LE(&fmt[12]);

      if((ReadU16LE(&fmt[0]) != 1U) ||
         ((g_audio.wav.num_channels != 1U) && (g_audio.wav.num_channels != 2U)) ||
         ((g_audio.wav.bits_per_sample != 8U) && (g_audio.wav.bits_per_sample != 16U)) ||
         (g_audio.wav.sample_rate_hz == 0U)) {
        return 0U;
      }

      if(chunk_size > sizeof(fmt)) {
        if(f_lseek(file, f_tell(file) + (chunk_size - sizeof(fmt))) != FR_OK) {
          return 0U;
        }
      }
      has_fmt = 1U;
    }
    else if(memcmp(chunk, "data", 4U) == 0) {
      g_audio.wav.data_offset = f_tell(file);
      g_audio.wav.data_size = chunk_size;
      g_audio.wav.data_remaining = chunk_size;
      has_data = 1U;
      if(has_fmt == 0U) {
        if(f_lseek(file, f_tell(file) + chunk_size) != FR_OK) {
          return 0U;
        }
      }
    }
    else {
      if(f_lseek(file, f_tell(file) + chunk_size) != FR_OK) {
        return 0U;
      }
    }

    if((chunk_size & 1U) != 0U) {
      if(f_lseek(file, f_tell(file) + 1U) != FR_OK) {
        return 0U;
      }
    }
  }

  if((g_audio.wav.data_size == 0U) || (f_lseek(file, g_audio.wav.data_offset) != FR_OK)) {
    return 0U;
  }
  g_audio.wav.data_remaining = g_audio.wav.data_size;
  g_audio.wav.io_index = 0U;
  g_audio.wav.io_count = 0U;
  return 1U;
}

static uint8_t ReadAudioBytes(uint8_t * dst, uint32_t len)
{
  UINT n;

  while(len > 0U) {
    uint32_t avail;
    uint32_t copy_len;

    if(g_audio.wav.io_index >= g_audio.wav.io_count) {
      uint32_t wanted;

      if(g_audio.wav.data_remaining == 0U) {
        if(f_lseek(&g_audio.wav_file, g_audio.wav.data_offset) != FR_OK) {
          return 0U;
        }
        g_audio.wav.data_remaining = g_audio.wav.data_size;
      }

      wanted = APP_AUDIO_IO_BUFFER_SIZE;
      if(wanted > g_audio.wav.data_remaining) {
        wanted = g_audio.wav.data_remaining;
      }

      if((wanted == 0U) ||
         (f_read(&g_audio.wav_file, g_audio.io_buffer, wanted, &n) != FR_OK) ||
         (n == 0U)) {
        return 0U;
      }

      g_audio.wav.data_remaining -= n;
      g_audio.wav.io_count = (uint16_t)n;
      g_audio.wav.io_index = 0U;
    }

    avail = (uint32_t)g_audio.wav.io_count - (uint32_t)g_audio.wav.io_index;
    copy_len = (len < avail) ? len : avail;
    (void)memcpy(dst, &g_audio.io_buffer[g_audio.wav.io_index], copy_len);

    g_audio.wav.io_index = (uint16_t)(g_audio.wav.io_index + copy_len);
    dst += copy_len;
    len -= copy_len;
  }
  return 1U;
}

static uint16_t DecodeFrameToDac(const uint8_t * frame)
{
  uint16_t sample_u16;
  int16_t sample_s16;

  if(g_audio.wav.bits_per_sample == 8U) {
    uint32_t left = frame[0];
    uint32_t right = (g_audio.wav.num_channels == 2U) ? frame[1] : left;
    return (uint16_t)(((left + right) / 2U) << 4);
  }

  sample_u16 = ReadU16LE(frame);
  if(g_audio.wav.num_channels == 2U) {
    sample_u16 = (uint16_t)((ReadU16LE(frame) + ReadU16LE(&frame[2])) / 2U);
  }
  sample_s16 = (int16_t)sample_u16;
  return (uint16_t)(((int32_t)sample_s16 + 32768) >> 4);
}

static void FillDacHalf(uint16_t * dst, uint32_t count)
{
  uint8_t frame[4];
  uint32_t i;
  uint32_t frame_bytes;

  frame_bytes = ((uint32_t)g_audio.wav.bits_per_sample / 8U) * (uint32_t)g_audio.wav.num_channels;
  if((frame_bytes == 0U) || (frame_bytes > sizeof(frame))) {
    for(i = 0U; i < count; i++) {
      dst[i] = 2048U;
    }
    Audio_CleanDCache(dst, count * sizeof(uint16_t));
    return;
  }

  for(i = 0U; i < count; i++) {
    if(ReadAudioBytes(frame, frame_bytes) == 0U) {
      dst[i] = 2048U;
    }
    else {
      dst[i] = ApplyVolume(DecodeFrameToDac(frame));
    }
  }
  Audio_CleanDCache(dst, count * sizeof(uint16_t));
}

static uint8_t StartDmaPlayback(void)
{
  DAC_ChannelConfTypeDef ch_cfg = {0};

  /* Re-arm DAC channel config explicitly to avoid silent bad peripheral state. */
  (void)HAL_DAC_Stop_DMA(&hdac, DAC_CHANNEL_1);
  (void)HAL_DAC_Stop(&hdac, DAC_CHANNEL_1);

  if(HAL_DAC_DeInit(&hdac) != HAL_OK) {
    return 0U;
  }
  if(HAL_DAC_Init(&hdac) != HAL_OK) {
    return 0U;
  }

  ch_cfg.DAC_Trigger = DAC_TRIGGER_T2_TRGO;
  ch_cfg.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
  if(HAL_DAC_ConfigChannel(&hdac, &ch_cfg, DAC_CHANNEL_1) != HAL_OK) {
    return 0U;
  }

  (void)HAL_TIM_Base_Stop(&htim2);
  __HAL_TIM_SET_COUNTER(&htim2, 0U);
  htim2.Instance->EGR = TIM_EGR_UG;
  __HAL_TIM_CLEAR_FLAG(&htim2, TIM_FLAG_UPDATE);
  __HAL_DAC_CLEAR_FLAG(&hdac, DAC_FLAG_DMAUDR1);

  if(HAL_DAC_Start_DMA(&hdac,
                       DAC_CHANNEL_1,
                       (uint32_t *)g_dac_dma_buffer,
                       APP_AUDIO_DMA_SAMPLES,
                       DAC_ALIGN_12B_R) != HAL_OK) {
    return 0U;
  }
  if(HAL_TIM_Base_Start(&htim2) != HAL_OK) {
    (void)HAL_DAC_Stop_DMA(&hdac, DAC_CHANNEL_1);
    return 0U;
  }
  return 1U;
}

static void FillToneHalf(uint16_t * dst, uint32_t count)
{
  uint32_t i;
  uint32_t period = g_audio.runtime.tone_period_samples;

  if(period < 2U) {
    period = 2U;
  }

  for(i = 0U; i < count; i++) {
    uint16_t sample;
    if(g_audio.runtime.tone_sample_index < (period / 2U)) {
      sample = APP_AUDIO_TONE_HIGH_SAMPLE;
    }
    else {
      sample = APP_AUDIO_TONE_LOW_SAMPLE;
    }

    dst[i] = ApplyVolume(sample);
    g_audio.runtime.tone_sample_index++;
    if(g_audio.runtime.tone_sample_index >= period) {
      g_audio.runtime.tone_sample_index = 0U;
    }
  }

  Audio_CleanDCache(dst, count * sizeof(uint16_t));
}

static void FillStaticDmaToneBuffer(void)
{
  uint32_t i;
  uint32_t phase;
  const uint32_t period_samples = 24U; /* 12 kHz / 24 = 500 Hz */

  for(i = 0U; i < APP_AUDIO_DMA_SAMPLES; i++) {
    phase = i % period_samples;
    if(phase < (period_samples / 2U)) {
      g_dac_dma_buffer[i] = APP_AUDIO_TONE_HIGH_SAMPLE;
    }
    else {
      g_dac_dma_buffer[i] = APP_AUDIO_TONE_LOW_SAMPLE;
    }
  }

  Audio_CleanDCache(g_dac_dma_buffer, sizeof(g_dac_dma_buffer));
}

uint8_t AppAudio_StartFromFile(const char * path)
{
#if (APP_AUDIO_FORCE_DIRECT_DAC_TEST == 1U)
  DAC_ChannelConfTypeDef ch_cfg = {0};
#endif

  if(path == NULL) {
    Audio_SetStatus("au:null");
    return 0U;
  }

  StopPlayback();
  g_audio.runtime.dma_error_pending = 0U;
  g_audio.runtime.volume_q8 = ReadVolumeQ8FromPot();

#if (APP_AUDIO_FORCE_DMA_STATIC_TONE_TEST == 1U)
  (void)path;
  g_audio.runtime.tone_mode = 3U;
  ConfigureTim2SampleRate(12000U);
  FillStaticDmaToneBuffer();

  if(StartDmaPlayback() == 0U) {
    Audio_SetStatus("au:dma static err");
    return 0U;
  }

  g_audio.runtime.running = 1U;
  Audio_SetStatus("au:dma static 500hz");
  return 1U;
#endif

#if (APP_AUDIO_FORCE_DIRECT_DAC_TEST == 1U)
  (void)path;
  g_audio.runtime.tone_mode = 2U;
  g_audio.runtime.tone_sample_index = 0U;
  /* AppAudio_Process runs every 5 ms -> period=2 gives a 100 Hz square wave. */
  g_audio.runtime.tone_period_samples = 2U;

  (void)HAL_DAC_Stop_DMA(&hdac, DAC_CHANNEL_1);
  (void)HAL_DAC_Stop(&hdac, DAC_CHANNEL_1);

  if(HAL_DAC_DeInit(&hdac) != HAL_OK) {
    Audio_SetStatus("au:dir deinit err");
    return 0U;
  }
  if(HAL_DAC_Init(&hdac) != HAL_OK) {
    Audio_SetStatus("au:dir init err");
    return 0U;
  }

  ch_cfg.DAC_Trigger = DAC_TRIGGER_NONE;
  ch_cfg.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
  if(HAL_DAC_ConfigChannel(&hdac, &ch_cfg, DAC_CHANNEL_1) != HAL_OK) {
    Audio_SetStatus("au:dir cfg err");
    return 0U;
  }
  if(HAL_DAC_Start(&hdac, DAC_CHANNEL_1) != HAL_OK) {
    Audio_SetStatus("au:dir start err");
    return 0U;
  }

  (void)HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, 2048U);
  g_audio.runtime.running = 1U;
  Audio_SetStatus("au:dir 100hz");
  return 1U;
#endif

#if (APP_AUDIO_FORCE_TONE_TEST == 1U)
  (void)path;
  g_audio.runtime.tone_mode = 1U;
  g_audio.runtime.tone_sample_index = 0U;
  g_audio.runtime.tone_period_samples = 22050U / APP_AUDIO_TONE_FREQ_HZ;
  if(g_audio.runtime.tone_period_samples < 2U) {
    g_audio.runtime.tone_period_samples = 2U;
  }

  ConfigureTim2SampleRate(22050U);
  FillToneHalf(&g_dac_dma_buffer[0], APP_AUDIO_HALF_SAMPLES);
  FillToneHalf(&g_dac_dma_buffer[APP_AUDIO_HALF_SAMPLES], APP_AUDIO_HALF_SAMPLES);

  if(StartDmaPlayback() == 0U) {
    Audio_SetStatus("au:tone dma err");
    return 0U;
  }

  g_audio.runtime.running = 1U;
  Audio_SetStatus("au:tone");
  return 1U;
#endif

  if(f_open(&g_audio.wav_file, path, FA_READ) != FR_OK) {
    Audio_SetStatus("au:open err");
    return 0U;
  }
  if(ParseWavHeader(&g_audio.wav_file) == 0U) {
    (void)f_close(&g_audio.wav_file);
    Audio_SetStatus("au:wav err");
    return 0U;
  }

  ConfigureTim2SampleRate(g_audio.wav.sample_rate_hz);
  FillDacHalf(&g_dac_dma_buffer[0], APP_AUDIO_HALF_SAMPLES);
  FillDacHalf(&g_dac_dma_buffer[APP_AUDIO_HALF_SAMPLES], APP_AUDIO_HALF_SAMPLES);

  if(StartDmaPlayback() == 0U) {
    (void)f_close(&g_audio.wav_file);
    Audio_SetStatus("au:dma err");
    return 0U;
  }

  g_audio.runtime.running = 1U;
  Audio_SetStatus("au:ok");
  return 1U;
}

void AppAudio_Process(void)
{
  static uint8_t volume_tick = 0U;

  volume_tick++;
  if(volume_tick >= 8U) {
    volume_tick = 0U;
    g_audio.runtime.volume_q8 = ReadVolumeQ8FromPot();
  }

  if(g_audio.runtime.dma_error_pending != 0U) {
    g_audio.runtime.dma_error_pending = 0U;
    StopPlayback();
    Audio_SetStatus("au:dma runtime err");
    return;
  }
  if(g_audio.runtime.running == 0U) {
    return;
  }

  if(g_audio.runtime.tone_mode == 2U) {
    uint16_t sample;

    g_audio.runtime.tone_sample_index++;
    if(g_audio.runtime.tone_sample_index >= g_audio.runtime.tone_period_samples) {
      g_audio.runtime.tone_sample_index = 0U;
    }

    if(g_audio.runtime.tone_sample_index < (g_audio.runtime.tone_period_samples / 2U)) {
      sample = APP_AUDIO_TONE_HIGH_SAMPLE;
    }
    else {
      sample = APP_AUDIO_TONE_LOW_SAMPLE;
    }

    (void)HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, ApplyVolume(sample));
    return;
  }

  if(g_audio.runtime.tone_mode != 0U) {
    if(g_audio.runtime.tone_mode == 3U) {
      return;
    }

    if(g_audio.runtime.need_fill_half0 != 0U) {
      g_audio.runtime.need_fill_half0 = 0U;
      FillToneHalf(&g_dac_dma_buffer[0], APP_AUDIO_HALF_SAMPLES);
    }
    if(g_audio.runtime.need_fill_half1 != 0U) {
      g_audio.runtime.need_fill_half1 = 0U;
      FillToneHalf(&g_dac_dma_buffer[APP_AUDIO_HALF_SAMPLES], APP_AUDIO_HALF_SAMPLES);
    }
    return;
  }

  if(g_audio.runtime.need_fill_half0 != 0U) {
    g_audio.runtime.need_fill_half0 = 0U;
    FillDacHalf(&g_dac_dma_buffer[0], APP_AUDIO_HALF_SAMPLES);
  }
  if(g_audio.runtime.need_fill_half1 != 0U) {
    g_audio.runtime.need_fill_half1 = 0U;
    FillDacHalf(&g_dac_dma_buffer[APP_AUDIO_HALF_SAMPLES], APP_AUDIO_HALF_SAMPLES);
  }
}

uint8_t AppAudio_IsRunning(void)
{
  return g_audio.runtime.running;
}

const char * AppAudio_GetStatus(void)
{
  return g_audio.runtime.status;
}

void HAL_DAC_ConvHalfCpltCallbackCh1(DAC_HandleTypeDef * hdac_handle)
{
  if((hdac_handle != NULL) && (hdac_handle->Instance == DAC)) {
    g_audio.runtime.need_fill_half0 = 1U;
  }
}

void HAL_DAC_ConvCpltCallbackCh1(DAC_HandleTypeDef * hdac_handle)
{
  if((hdac_handle != NULL) && (hdac_handle->Instance == DAC)) {
    g_audio.runtime.need_fill_half1 = 1U;
  }
}

void HAL_DAC_ErrorCallbackCh1(DAC_HandleTypeDef * hdac_handle)
{
  if((hdac_handle != NULL) && (hdac_handle->Instance == DAC)) {
    g_audio.runtime.dma_error_pending = 1U;
  }
}
