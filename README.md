# projetDemo2026

Projet embarqué STM32F746 orienté affichage et interaction graphique, construit autour de LVGL, FreeRTOS et du HAL STM32Cube.

## Vue d'ensemble

Ce projet cible la carte STM32F746G-DISCO et combine :

- une interface graphique LVGL pour la scène de jeu,
- un rendu d'assets stockés sur carte SD via FatFs,
- une boucle temps réel pilotée par FreeRTOS,
- des périphériques STM32 pour l'affichage, le tactile, l'audio et le stockage.

## Stack technique

- Microcontrôleur : STM32F746NGHx
- Framework bas niveau : STM32Cube HAL / CMSIS
- Interface graphique : LVGL
- RTOS : FreeRTOS
- Système de fichiers : FatFs
- Affichage : LTDC / RGB565
- Mémoire externe : SDRAM
- Stockage : carte SD

## Structure du projet

- `Core/Src` et `Core/Inc` : logique applicative, UI, audio, gestion du jeu
- `Middlewares/Third_Party` : LVGL, FreeRTOS, FatFs
- `Drivers` : périphériques HAL STM32
- `Fonts` : polices et ressources associées
- `build` : artefacts de compilation CMake/Ninja

## Fonctionnalités principales

- scène LVGL avec background, joueur et blocs de niveau,
- chargement d'images BMP depuis la carte SD,
- rendu d'assets en mémoire tampon LVGL,
- audio et boucle de mise à jour intégrés au runtime,
- architecture pensée pour l'exécution embarquée temps réel.

## Format des assets

Le projet est configuré pour travailler avec des assets BMP. Le support JPG/PNG a été retiré du pipeline LVGL du projet.

## Build

Le projet est configuré avec CMake et les presets fournis dans `CMakePresets.json`.

Exemple de flux de travail :

1. configurer le preset Debug,
2. générer la chaîne de build,
3. compiler la cible `projetDemo2026`.

## Remarques

- Les chemins d'assets sont gérés côté application et chargés depuis la carte SD.
- Le projet contient des éléments générés par STM32CubeMX, donc certaines parties peuvent être régénérées à partir du fichier `.ioc`.

## Objectif

Servir de base à une application embarquée avec UI graphique et logique temps réel sur STM32F746.
