# LVGL et son Utilisation

LVGL est une bibliothèque graphique pour systèmes embarqués (~150 KB ROM, ~15 KB RAM). Performance 30+ FPS, support multi-résolution, animations intégrées.

Afin d'utiliser cette librairie, on commence par initialiser LVGL et le pilote d'affichage puis fournir un "tick" régulier à la librairie (ex. `lv_tick_inc`) depuis une tâche ou un timer. Créer les écrans et objets via l'API LVGL (images, labels, conteneurs), gérer les descriptors d'images pour les assets chargés depuis la SD, et appeler périodiquement `lv_timer_handler()` ou `lv_refr_now()` depuis la tâche d'affichage pour exécuter les timers et rafraîchir l'écran. Charger les images de façon paresseuse (lazy load) et utiliser un cache LRU pour limiter l'usage SDRAM.

## Architecture Dual Display

Le projet utilise deux framebuffers LVGL :

- Background display : images statiques 
- Foreground display : sprites dynamiques 
- Résolution : 480x272 RGB565
- LTDC : compositing matériel + blending

Avantages : séparation du fond et du personnage, réduction bande passante, latence réduite.

## Chargement Sprites BMP

Sprites BMP chargées depuis SD via FatFs, parsage header, allocation SDRAM.

Cache LRU (16 entrées) avec éviction automatique si la limite mémoire est atteinte.

## Configuration LVGL

lv_conf.h : 480x272, RGB565 (16-bit), animations activées.

Widgets activés : label, image, button, screen.

BMP decoder actif, PNG/JPG désactivés.

Refresh period : 33 ms (~30 Hz).

## Rendu et Animations

Cycle rendu : invalidate zones modifiées -> lv_refr_now -> LTDC flush -> flip buffers.

Animations LVGL : framework intégré avec interpolation et callbacks.

Styles : configuration par thèmes avec couleurs/opacity.

## Optimisations

Dirty region tracking : redessine uniquement zones modifiées.

Lazy loading : sprites chargées à la demande.

Memory pooling : pré-allocation objets si nécessaire.

Monitoring : FPS counter, memory usage tracking.


---

**Prochaine lecture** : [Tâches en cours](taches.md)
