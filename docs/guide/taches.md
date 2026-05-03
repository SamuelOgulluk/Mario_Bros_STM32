# Tâches FreeRTOS

3 tâches principales implémentées dans [freertos.c](../../Core/Src/freertos.c) :

## Audio Task (StartAudioTask)

Priorité basse (1536 words), période 5 ms.

Charge WAV depuis SD et gère DAC + DMA. Retry chaque seconde en cas d'erreur.

```c
AppAudio_IsRunning() ? 
  → Skip : déjà en lecteur
  → OUI : lancer AppAudio_StartFromFile("theme.wav")

AppAudio_Process()  // Refill DMA buffer
vTaskDelay(5 ms)
```

Config : `AUDIO_TASK_PERIOD_MS = 5`

## Default Task (StartDefaultTask)

Priorité normale (2048 words), période 5 ms.

Cœur du jeu : physique joueur, collision, LVGL render, debug output.

Sections :
1. **Physique** (5 ms) : gravité, input, mouvement, collision, animation sprite
2. **Debug** (tous 25 ms) : display position/velocity/state  
3. **LVGL** (5 ms) : lv_timer_handler() pour UI render
4. **LED** (tous 500 ms) : toggle LED heartbeat

```c
Player_Update()        // Gravité, input, collision
UI_SetDebugInput()     // Debug display (25 ms)
lv_timer_handler()     // LVGL UI (5 ms)
HAL_GPIO_TogglePin()   // LED toggle (500 ms)
vTaskDelay(5 ms)
```

Config :
- `MAIN_LOOP_PERIOD_MS = 5`
- `CONTROL_UPDATE_LOOP_DIV = 1` (5ms)
- `DEBUG_UPDATE_LOOP_DIV = 5` (25ms)
- `LVGL_UPDATE_LOOP_DIV = 1` (5ms)
- `LED_TOGGLE_LOOP_DIV = 100` (500ms)

## Idle Task

Priorité très basse (256 bytes), s'exécute quand autres tâches bloquées.

Power management, watchdog feed, nettoyage ressources.

```c
void vApplicationGetIdleTaskMemory(...) {
    // Allouer mémoire statique pour idle task
}

// Optionnel : vApplicationIdleHook()
void vApplicationIdleHook(void) {
    __WFE();           // Sleep faible puissance
    IWDG_Refresh();    // Watchdog
}
```

## Résumé

| Tâche | Priorité | Période | Stack | Rôle |
|-------|----------|---------|-------|------|
| Audio | Basse | 5 ms | 1536 W | WAV player |
| Default | Normale | 5 ms | 2048 W | Game logic |
| Idle | Très basse | Continu | 256 B | Power mgmt |

--- 
**Prochaine lecture :** [Perspectives futures](perspectives.md)