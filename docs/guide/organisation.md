# Organisation du Projet

## Structure

```
projetDemo2026/
├── CMakeLists.txt              # Build configuration
├── CMakePresets.json           # Build presets
├── Core/
│   ├── Inc/                    # Headers
│   │   ├── app_ui.h            # UI API (LVGL)
│   │   ├── app_audio.h         # Audio API (WAV)
│   │   ├── player.h            # Physique/collision
│   │   ├── freertos_app.h      # FreeRTOS config
│   │   └── *_hal.h             # HAL (adc, dac, gpio, i2c, etc)
│   └── Src/
│       ├── main.c              # Point d'entrée
│       ├── app_ui.c            # Rendu LVGL
│       ├── app_audio.c         # Lecteur WAV
│       ├── player.c            # Physique
│       ├── freertos.c          # FreeRTOS tasks
│       └── *_hal.c             # HAL implementations
├── Drivers/
│   ├── CMSIS/                  # ARM CMSIS
│   └── STM32F7xx_HAL_Driver/   # ST HAL
├── Middlewares/Third_Party/
│   ├── LVGL/                   # Librairie graphique
│   ├── FreeRTOS/               # RTOS
│   └── FatFs/                  # Filesystem
├── Fonts/                      # Bitmap fonts
└── cmake/                      # CMake scripts
```


## Modules Clés

### Module Application (`Core/Src/app_*.c`)
**app_ui.c** — rôle et fonctions

- Rôle principal : interface entre la logique du jeu et LVGL. Gère la construction des écrans, l'affichage des images et sprites, et l'actualisation framebuffer.
- Fonctions clés :
  - initialisation de l'UI et des layers (background / foreground)
  - gestion du cache d'assets (chargement BMP, eviction LRU)
  - mise à jour des positions de sprites et des labels (score, FPS)
  - orchestration du cycle de rendu (invalidation, lv_timer_handler, swap buffers)

**player.c** — rôle et fonctions

- Rôle principal : simulation physique et états du joueur (position, vitesse, collisions, animations).
- Fonctions clés :
  - initialisation et réinitialisation du joueur
  - `Player_Update()` : appliquer gravité, lire entrée, déplacer, détecter collisions
  - gestion des états d'animation (IDLE, WALK, JUMP)
  - helpers collision AABB et résolution de pénétration

**app_audio.c** — rôle et fonctions

- Rôle principal : pipeline de lecture audio WAV via FatFs → ring buffer → DAC + DMA.
- Fonctions clés :
  - découverte et ouverture de fichiers WAV sur la carte SD
  - parsing header WAV et configuration du ring buffer
  - démarrage/arrêt du DAC + configuration DMA
  - `AppAudio_Process()` : remplissage périodique du buffer, gestion volume et état
  - hooks pour callbacks DMA / TIM (synchronisation audio)

### Module Matériel (`Core/Src/*_hal.c`)

Chaque périphérique a son module :

- **gpio.c** : Configuration GPIO pour contrôle
- **ltdc.c** : LTDC display controller (480x272 RGB565)
- **dac.c** : Digital-Analog Converter pour audio
- **tim.c** : Timers (TIM2 pour sync audio)
- **dma.c** : Direct Memory Access (transfers parallèles)
- **i2c.c** : I2C (écran tactile FT5336)
- **spi.c** : SPI (optionnel, SD card)
- **fmc.c** : Flexible Memory Controller (SDRAM)

### FreeRTOS (`Core/Src/freertos.c`)

```c
// Création des tâches
xTaskCreate(render_task, "render", 2048, NULL, 2, NULL);
xTaskCreate(game_logic_task, "game", 2048, NULL, 2, NULL);
xTaskCreate(audio_task, "audio", 1024, NULL, 1, NULL);

// Synchronisation
xSemaphore vsync_semaphore;  // Signalé au vsync
xMutex sdram_mutex;          // Accès SDRAM thread-safe
```

## 🔄 Flux de Données

```
    SD Card (BMP + WAV)
           ↓
       FatFs
           ↓
    ┌──────┴──────┐
    ↓             ↓
  Images        Audio
    ↓             ↓
App UI        App Audio
    ↓             ↓
  LVGL → LTDC    DAC
    ↓             ↓
  Display      Speaker
```


## Layers Architecturaux

```
┌─────────────────────────────────────────┐
│  Gameplay Layer                         │
│  (player.c, collision, logic)           │
├─────────────────────────────────────────┤
│  Application Layer                      │
│  (app_ui.c, app_audio.c, main.c)       │
├─────────────────────────────────────────┤
│  Middleware Layer                       │
│  (LVGL, FreeRTOS, FatFs)                │
├─────────────────────────────────────────┤
│  HAL/Driver Layer                       │
│  (Peripheral abstractions)              │
├─────────────────────────────────────────┤
│  Hardware Layer                         │
│  (STM32F746 + peripherals)              │
└─────────────────────────────────────────┘
```

---

**Prochaine lecture** : [LVGL et son utilisation](lvgl.md)
