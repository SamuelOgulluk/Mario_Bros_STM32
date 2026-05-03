# Fonctionnalités Principales

## Moteur de Jeu

Collision AABB avec réaction physique (glissement horizontal, rebond vertical).

Mécanique de saut avec hauteur variable et apex detection.

Gravité constante avec terminal velocity et landing detection.

## Système Graphique LVGL

Dual framebuffer (background statique + foreground dynamique) avec double buffering au vsync.

Types de blocs : Normal (destructible), Ground (platforme), Pipe (obstacle), Question (collectible), Goomba (ennemi), Fortress (fin niveau).

États joueur : IDLE, WALK, JUMP.

## Système Audio

Lecteur WAV optimisé (unsigned 8bit, 44 kHz).

Pipeline : SD -> FatFs -> WAV Parser -> DMA Ring Buffer -> DAC + TIM2 → Speaker.

## Gestion Assets

Chargement BMP depuis SD au boot, pré-chargement en SDRAM.

Format : BMP 24/16-bit RGB565, grille 31 pixels.

Chemin : `0:/sprites/` pour images et `0:/son/` pour audio.

## FreeRTOS

Deux tâches principales (période de 5 ms chacune) :
- Audio Task (priorité basse) : charge WAV, gère DAC/DMA
- Default Task (priorité normale) : physique, LVGL, debug

Tâche Idle : power management et watchdog.

## Entrée

- Joystick sur son axe horizontal
- BP1 pour le saut du personnage
- BP2 pour le sprint


## Optimisations

DMA + double buffer audio .

Spatial hashing pour collision rapide (O(1)).

Lazy loading sprites à la demande.

-- 

**Prochaine lecture** : [Organisation](organisation.md)