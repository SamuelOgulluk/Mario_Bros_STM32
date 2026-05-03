# Objectifs du Projet

## Principaux

1. Plateforme embarquée fonctionnelle avec graphiques, audio et temps réel
2. Maîtrise de LVGL et optimisation MCU
3. Architecture FreeRTOS robuste
4. Optimisation ressources (SDRAM, DMA, LTDC)

## Secondaires

- Chargement dynamique d'assets BMP depuis SD
- Synchronisation audio-vidéo PCM/DAC
- Pipeline complet jeu plateforme

## Critères Succès

| Critère | Cible |
|---------|-------|
| FPS | 15 minimum |
| Latence | < 50 ms |
| Audio drift | < 100 ms |
| Stockage SD | 0 corruption |

## Contraintes

- RAM : 256 KB + 64 MB SDRAM
- CPU : 216 MHz 
- Écran : 480x272 et écriture écran lente
- Formats : BMP uniquement
- Temps réel : priorités FreeRTOS strictes

--

**Prochaine lecture** : [Fonctionnalités](fonctionnalites.md)
