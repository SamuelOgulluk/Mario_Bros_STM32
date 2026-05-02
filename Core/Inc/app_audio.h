/*
 * app_audio.h
 * WAV player API backed by DAC + DMA.
 */

#ifndef APP_AUDIO_H
#define APP_AUDIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define APP_AUDIO_DMA_SAMPLES        4096U
#define APP_AUDIO_HALF_SAMPLES       (APP_AUDIO_DMA_SAMPLES / 2U)
#define APP_AUDIO_IO_BUFFER_SIZE     2048U
#define APP_AUDIO_VOLUME_ADC_CHANNEL ADC_CHANNEL_0
#define APP_AUDIO_VOLUME_Q8_MIN       8U
#define APP_AUDIO_VOLUME_Q8_MAX      256U
#define APP_AUDIO_TONE_FREQ_HZ       1000U
#define APP_AUDIO_TONE_HIGH_SAMPLE    3600U
#define APP_AUDIO_TONE_LOW_SAMPLE     496U

uint8_t AppAudio_StartFromFile(const char * path);
void AppAudio_Process(void);
uint8_t AppAudio_IsRunning(void);
const char * AppAudio_GetStatus(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_AUDIO_H */
