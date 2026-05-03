# Mario Bros STM32 - Plateforme de Jeu Embarquée STM32F746

Projet embarqué complet basé sur la carte STM32F746G combinant interface graphique, moteur de jeu et audio en temps réel.

## Vue d'ensemble

Moteur de jeu interactif utilisant :

- **STM32F746NGHx** : Microcontrôleur puissant avec écran tactile intégré
- **LVGL** : Framework d'interface graphique légère et efficace
- **FreeRTOS** : Système d'exploitation temps réel pour la gestion des tâches
- **Stockage SD** : Chargement dynamique des assets (sprites, images)
- **Audio** : Système de lecture WAV par DAC avec synchronisation DMA

Le projet implémente les bases d'un Super Mario NES avec :

- Personnage jouable avec animations
- Système de collision physique
- Blocs interactifs et obstacles
- Gestion de niveau et progression

## Objectifs

1. Plateforme embarquée complète : graphiques, audio et temps réel
2. Maîtrise de LVGL pour systèmes embarqués
3. Architecture temps réel avec FreeRTOS
4. Optimisation des ressources (SDRAM, DMA, buffering)

## Stack Technique

| Composant | Technologie | Version/Notes |
|-----------|-------------|---------------|
| **MCU** | STM32F746NGHx | ARM Cortex-M7 @216 MHz |
| **Framework bas niveau** | STM32Cube HAL | CMSIS-RTOS2 |
| **GUI** | LVGL | Graphics rendering |
| **RTOS** | FreeRTOS | Time scheduling |
| **Stockage** | FatFs + SD | Asset loading |
| **Affichage** | LTDC | RGB565, 480x272 |
| **Mémoire externe** | SDRAM 64 MB | Frame buffers |
| **Audio** | DAC + DMA + TIM2 | PCM WAV playback |

## Démarrage rapide

### Prérequis

- **CMake** >= 3.20
- Carte **STM32F746G**
- **Fichiers assets** (.bmp) sur la carte SD




## Documentation

Une documentation détaillée est disponibles aux pages suivantes :

- [Objectifs](guide/objectifs.md)
- [Fonctionnalités](guide/fonctionnalites.md)
- [Organisation](guide/organisation.md)
- [LVGL](guide/lvgl.md)
- [Tâches FreeRTOS](guide/taches.md)
- [Perspectives futures](guide/perspectives.md)


---

**Dernière mise à jour** : Mai 2026  