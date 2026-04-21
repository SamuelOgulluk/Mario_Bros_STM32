/*
 * app_audio.c
 *
 * - 8-bit unsigned
 */

#include "app_audio.h"
#include "main.h"
#include "dac.h"
#include "tim.h"
#include "app_audio_types.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>


static AppAudioContext_t g_audio = {
  .runtime = {
    .status = "au:init"
  }
};

static void FillDacHalf(uint16_t * dst, uint32_t sample_count);
static void ConfigureTim2SampleRate(uint32_t sample_rate);

static void Audio_SetStatus(const char * fmt, ...)
{
  va_list args;

  va_start(args, fmt);
  (void)vsnprintf(g_audio.runtime.status, sizeof(g_audio.runtime.status), fmt, args);
  va_end(args);
}

/* Lit un entier 32 bits little-endian depuis un buffer d'octets. */
static uint32_t ReadU32LE(const uint8_t * data)
{
  return (uint32_t)data[0] |
         ((uint32_t)data[1] << 8) |
         ((uint32_t)data[2] << 16) |
         ((uint32_t)data[3] << 24);
}

/* Lit un entier 16 bits little-endian depuis un buffer d'octets. */
static uint16_t ReadU16LE(const uint8_t * data)
{
  return (uint16_t)data[0] | ((uint16_t)data[1] << 8);
}

/* Lit des octets dans la section data du WAV avec buffering circulaire. */
static uint8_t WavReadBytesBuffered(uint8_t * dst, uint16_t len)
{
  uint16_t remaining;
  uint16_t chunk;
  UINT bytes_read;
  uint32_t wanted;

  remaining = len;
  while(remaining > 0U) {
    if(g_audio.wav.io_index >= g_audio.wav.io_count) {
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
      if(wanted == 0U) {
        continue;
      }

      bytes_read = 0U;
      if(f_read(&g_audio.wav_file, g_audio.io_buffer, wanted, &bytes_read) != FR_OK) {
        return 0U;
      }
      if(bytes_read == 0U) {
        return 0U;
      }

      g_audio.wav.data_remaining -= bytes_read;
      g_audio.wav.io_count = (uint16_t)bytes_read;
      g_audio.wav.io_index = 0U;
    }

    chunk = (uint16_t)(g_audio.wav.io_count - g_audio.wav.io_index);
    if(chunk > remaining) {
      chunk = remaining;
    }

    (void)memcpy(dst, &g_audio.io_buffer[g_audio.wav.io_index], chunk);
    dst += chunk;
    g_audio.wav.io_index = (uint16_t)(g_audio.wav.io_index + chunk);
    remaining = (uint16_t)(remaining - chunk);
  }

  return 1U;
}

static uint8_t StartDacPlayback(void)
{
  if(HAL_DAC_Start_DMA(&hdac,
                       DAC_CHANNEL_1,
                       (uint32_t *)g_audio.dac_dma_buffer,
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
/* Decode un echantillon WAV et le convertit en valeur DAC 12 bits. */
static uint8_t WavReadOneDacSample(uint16_t * out_sample)
{
  uint8_t sample_u8;

  if(out_sample == NULL) {
    return 0U;
  }

  if(WavReadBytesBuffered(&sample_u8, 1U) == 0U) {
    return 0U;
  }

  *out_sample = (uint16_t)((uint16_t)sample_u8 << 4);
  return 1U;
}

/* Remplit une demi-zone DMA avec des echantillons DAC decodes. */
static void FillDacHalf(uint16_t * dst, uint32_t sample_count)
{
  uint32_t i;
  uint16_t dac_sample;

  for(i = 0U; i < sample_count; i++) {
    if(WavReadOneDacSample(&dac_sample) == 0U) {
      dac_sample = 2048U;
    }
    dst[i] = dac_sample;
  }
}

/* Configure la frequence d'update de TIM2 selon le sample rate WAV. */
static void ConfigureTim2SampleRate(uint32_t sample_rate)
{
  RCC_ClkInitTypeDef clk_cfg;
  uint32_t latency;
  uint32_t pclk1;
  uint32_t timclk;
  uint32_t psc;
  uint32_t arr;
  uint64_t denom;

  HAL_RCC_GetClockConfig(&clk_cfg, &latency);
  pclk1 = HAL_RCC_GetPCLK1Freq();
  if(clk_cfg.APB1CLKDivider == RCC_HCLK_DIV1) {
    timclk = pclk1;
  }
  else {
    timclk = pclk1 * 2U;
  }

  if(sample_rate == 0U) {
    sample_rate = 8000U;
  }

  denom = (uint64_t)sample_rate * 65536ULL;
  psc = (uint32_t)(timclk / denom);
  if(psc > 0xFFFFU) {
    psc = 0xFFFFU;
  }

  arr = (uint32_t)(timclk / ((psc + 1U) * sample_rate));
  if(arr == 0U) {
    arr = 1U;
  }
  arr -= 1U;

  __HAL_TIM_DISABLE(&htim2);
  __HAL_TIM_SET_PRESCALER(&htim2, psc);
  __HAL_TIM_SET_AUTORELOAD(&htim2, arr);
  __HAL_TIM_SET_COUNTER(&htim2, 0U);
  __HAL_TIM_CLEAR_FLAG(&htim2, TIM_FLAG_UPDATE);
  __HAL_TIM_ENABLE(&htim2);
}

/* Parse l'en-tete RIFF/WAV et localise le chunk PCM "data". */
static uint8_t ParseWavHeader(FIL * file)
{
  uint8_t riff[12];
  uint8_t chunk_hdr[8];
  uint8_t fmt_data[16];
  UINT bytes_read;
  uint32_t chunk_size;
  uint32_t fmt;
  uint16_t num_channels;
  uint16_t bits_per_sample;
  uint8_t found_fmt;
  uint8_t found_data;

  found_fmt = 0U;
  found_data = 0U;

  if(f_lseek(file, 0U) != FR_OK) {
    return 0U;
  }

  bytes_read = 0U;
  if((f_read(file, riff, sizeof(riff), &bytes_read) != FR_OK) || (bytes_read != sizeof(riff))) {
    return 0U;
  }

  if((memcmp(&riff[0], "RIFF", 4U) != 0) || (memcmp(&riff[8], "WAVE", 4U) != 0)) {
    return 0U;
  }

  while((found_fmt == 0U) || (found_data == 0U)) {
    bytes_read = 0U;
    if((f_read(file, chunk_hdr, sizeof(chunk_hdr), &bytes_read) != FR_OK) || (bytes_read != sizeof(chunk_hdr))) {
      return 0U;
    }

    chunk_size = ReadU32LE(&chunk_hdr[4]);

    if(memcmp(&chunk_hdr[0], "fmt ", 4U) == 0) {
      if(chunk_size < sizeof(fmt_data)) {
        return 0U;
      }

      bytes_read = 0U;
      if((f_read(file, fmt_data, sizeof(fmt_data), &bytes_read) != FR_OK) || (bytes_read != sizeof(fmt_data))) {
        return 0U;
      }

      fmt = ReadU16LE(&fmt_data[0]);
      num_channels = ReadU16LE(&fmt_data[2]);
      g_audio.wav.sample_rate_hz = ReadU32LE(&fmt_data[4]);
      bits_per_sample = ReadU16LE(&fmt_data[14]);

      if((fmt != 1U) ||
        (num_channels != 1U) ||
        (bits_per_sample != 8U) ||
        (g_audio.wav.sample_rate_hz == 0U)) {
        return 0U;
      }

      if(chunk_size > sizeof(fmt_data)) {
        if(f_lseek(file, f_tell(file) + (chunk_size - sizeof(fmt_data))) != FR_OK) {
          return 0U;
        }
      }

      found_fmt = 1U;
    }
    else if(memcmp(&chunk_hdr[0], "data", 4U) == 0) {
      g_audio.wav.data_offset = f_tell(file);
      g_audio.wav.data_size = chunk_size;
      g_audio.wav.data_remaining = chunk_size;
      found_data = 1U;

      if(found_fmt == 0U) {
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

    if((found_fmt == 1U) && (found_data == 1U)) {
      break;
    }

    if(f_eof(file) != 0) {
      return 0U;
    }
  }

  if((f_lseek(file, g_audio.wav.data_offset) != FR_OK) || (g_audio.wav.data_size == 0U)) {
    return 0U;
  }

  g_audio.wav.data_remaining = g_audio.wav.data_size;
  g_audio.wav.io_index = 0U;
  g_audio.wav.io_count = 0U;
  return 1U;
}

/* Demarre le streaming WAV fichier -> DAC via DMA et trigger TIM2. */
uint8_t AppAudio_StartFromFile(const char * path)
{
  FILINFO fno;
  FRESULT res;

  if(path == NULL) {
    Audio_SetStatus("au:null");
    return 0U;
  }

  g_audio.runtime.running = 0U;
  g_audio.runtime.need_fill_half0 = 0U;
  g_audio.runtime.need_fill_half1 = 0U;
  g_audio.runtime.dma_error_pending = 0U;
  Audio_SetStatus("au:check %s", path);

  (void)HAL_DAC_Stop_DMA(&hdac, DAC_CHANNEL_1);
  (void)HAL_TIM_Base_Stop(&htim2);

  res = f_stat(path, &fno);
  if(res != FR_OK) {
    Audio_SetStatus("au:retry");
    HAL_Delay(200U);
    res = f_stat(path, &fno);
    if(res != FR_OK) {
      Audio_SetStatus("au:file err");
      return 0U;
    }
  }

  if(f_open(&g_audio.wav_file, path, FA_READ) != FR_OK) {
    Audio_SetStatus("au:open err");
    return 0U;
  }

  Audio_SetStatus("au:parse");
  if(ParseWavHeader(&g_audio.wav_file) == 0U) {
    (void)f_close(&g_audio.wav_file);
    Audio_SetStatus("au:wav err");
    return 0U;
  }

  Audio_SetStatus("au:tim cfg");
  ConfigureTim2SampleRate(g_audio.wav.sample_rate_hz);

  Audio_SetStatus("au:fill buf");
  FillDacHalf(&g_audio.dac_dma_buffer[0], APP_AUDIO_HALF_SAMPLES);
  FillDacHalf(&g_audio.dac_dma_buffer[APP_AUDIO_HALF_SAMPLES], APP_AUDIO_HALF_SAMPLES);

  Audio_SetStatus("au:dma st");
  if(StartDacPlayback() == 0U) {
    (void)f_close(&g_audio.wav_file);
    Audio_SetStatus("au:dma err");
    return 0U;
  }

  g_audio.runtime.running = 1U;
  Audio_SetStatus("au:OK %uHz", (unsigned int)g_audio.wav.sample_rate_hz);
  return 1U;
}

/* Recharge les demi-buffers DMA demandes par callbacks et met a jour le debug. */
void AppAudio_Process(void)
{
  if(g_audio.runtime.dma_error_pending != 0U) {
    g_audio.runtime.dma_error_pending = 0U;
    g_audio.runtime.running = 0U;
    g_audio.runtime.need_fill_half0 = 0U;
    g_audio.runtime.need_fill_half1 = 0U;
    (void)HAL_DAC_Stop_DMA(&hdac, DAC_CHANNEL_1);
    (void)HAL_TIM_Base_Stop(&htim2);
    (void)f_close(&g_audio.wav_file);
    Audio_SetStatus("au:dma runtime err");
  }

  if(g_audio.runtime.running == 0U) {
    return;
  }

  if(g_audio.runtime.need_fill_half0 != 0U) {
    g_audio.runtime.need_fill_half0 = 0U;
    FillDacHalf(&g_audio.dac_dma_buffer[0], APP_AUDIO_HALF_SAMPLES);
  }

  if(g_audio.runtime.need_fill_half1 != 0U) {
    g_audio.runtime.need_fill_half1 = 0U;
    FillDacHalf(&g_audio.dac_dma_buffer[APP_AUDIO_HALF_SAMPLES], APP_AUDIO_HALF_SAMPLES);
  }
}

/* Retourne si le streaming audio est actif. */
uint8_t AppAudio_IsRunning(void)
{
  return g_audio.runtime.running;
}

/* Retourne la derniere chaine de statut audio pour l'affichage debug. */
const char * AppAudio_GetStatus(void)
{
  return g_audio.runtime.status;
}

/* Callback demi-transfert DMA: programme le refill de la premiere moitie. */
void HAL_DAC_ConvHalfCpltCallbackCh1(DAC_HandleTypeDef * hdac_handle)
{
  if((hdac_handle != NULL) && (hdac_handle->Instance == DAC)) {
    g_audio.runtime.need_fill_half0 = 1U;
  }
}

/* Callback transfert DMA complet: programme le refill de la seconde moitie. */
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
