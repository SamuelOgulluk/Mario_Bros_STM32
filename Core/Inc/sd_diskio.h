/**
  ******************************************************************************
  * @file    sd_diskio.h
  * @author  MCD Application Team
  * @brief   Header for sd_diskio.c module.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2017 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __SD_DISKIO_H
#define __SD_DISKIO_H

/* Includes ------------------------------------------------------------------*/
#include "ff_gen_drv.h"
#include "stm32746g_discovery_sd.h"

/* Private constants shared with sd_diskio.c */
#if defined(SDMMC_DATATIMEOUT)
#define SD_TIMEOUT SDMMC_DATATIMEOUT
#elif defined(SD_DATATIMEOUT)
#define SD_TIMEOUT SD_DATATIMEOUT
#else
#define SD_TIMEOUT (30U * 1000U)
#endif

#define SD_DEFAULT_BLOCK_SIZE 512U
#define SD_IO_RETRY_COUNT 3U
#define SD_IO_READY_TIMEOUT_MS 200U
#define DISABLE_SD_INIT

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
extern const Diskio_drvTypeDef  SD_Driver;

#endif /* __SD_DISKIO_H */


