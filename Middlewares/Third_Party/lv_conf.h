#ifndef LV_CONF_H
#define LV_CONF_H

/* Écran LCD RGB565. */
#define LV_COLOR_DEPTH 16

/* Désactivation de l'OS (tâche unique). */
#define LV_USE_OS LV_OS_NONE

/* Tas en mémoire interne. */
#define LV_MEM_SIZE (128U * 1024U)

/* Cache pour les images. */
#define LV_CACHE_DEF_SIZE (32U * 1024U)
#define LV_IMAGE_HEADER_CACHE_DEF_CNT 8

/* Diagnostics désactivés. */
#define LV_USE_LOG 0
#define LV_USE_SYSMON 0
#define LV_USE_PERF_MONITOR 0

/* Pilote d'affichage LTDC. */
#define LV_USE_ST_LTDC 1

/* FatFs pour la lecture sur carte SD. */
#define LV_USE_FS_FATFS 1
#define LV_FS_FATFS_LETTER 'S'
#define LV_FS_FATFS_PATH "0:/"

/* Décodeur BMP désactivé. */
#define LV_USE_BMP 0

#endif /* LV_CONF_H */