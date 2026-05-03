# Perspectives futures

Des améliorations possibles pour le projet sont :

- **Optimisation mémoire** : réduire l'empreinte SDRAM en améliorant le cache d'images (taille LRU adaptative, compression en RAM pour assets peu utilisés).

- **Streaming d'assets** : implémenter un loader asynchrone pour précharger les zones de niveau à la demande (double-buffering d'assets depuis la SD).

- **Amélioration audio** : ajouter décodage léger (ADPCM/IMA) ou support d'un petit lecteur OGG pour réduire l'utilisation d'espace et CPU.

- **Mixage et effets** : introduire un petit moteur de mixage PCM pour volumes/voix multiples et effets (fade, pitch shift basique).

- **Refactorisation des tâches RTOS** : séparer clairement logique jeu / rendu / audio en queues et événements, réduire les sections critiques protégées.

- **Développement du Gameplay** : Ajout de niveaux, d'une gestion entre niveaux

- **Ajout d'ennemis** : Développement des ennemis et de leurs déplacements

- **Amélioration du défilement** : Le défilement est actuellement sacadé à cause de la fréquence d'actualisation de l'écran