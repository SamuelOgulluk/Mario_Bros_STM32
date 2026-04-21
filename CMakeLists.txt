# Set the minimum version of CMake that can be used
cmake_minimum_required(VERSION 3.20)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Project definition
project(projetDemo2026)

# Enable CMake support for ASM and C/CPP languages
enable_language(C ASM)

set(BUILD_UNIT_0_NAME projetDemo2026)

# Create an executable object type
add_executable(${BUILD_UNIT_0_NAME})

# STM32CubeIDE for Visual Studio Code managed CMake file
include(cmake/vscode_generated.cmake)

# Compiler options
target_compile_options(${BUILD_UNIT_0_NAME} PRIVATE
  # User defined compiler options
)

# Linker options
target_link_options(${BUILD_UNIT_0_NAME} PRIVATE 
  # User defined linker options
)

target_compile_definitions(${BUILD_UNIT_0_NAME} PRIVATE
  USE_FREERTOS
)

# LVGL middleware sources
file(GLOB_RECURSE LVGL_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/LVGL/src/*.c"
)

list(FILTER LVGL_SOURCES EXCLUDE REGEX ".*/libs/(lodepng|libpng|tjpgd|libjpeg_turbo)/.*")

set(FREERTOS_SOURCES
  "${CMAKE_SOURCE_DIR}/Core/Src/freertos.c"
  "${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/FreeRTOS/Source/croutine.c"
  "${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/FreeRTOS/Source/event_groups.c"
  "${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/FreeRTOS/Source/list.c"
  "${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/FreeRTOS/Source/queue.c"
  "${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/FreeRTOS/Source/stream_buffer.c"
  "${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/FreeRTOS/Source/tasks.c"
  "${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/FreeRTOS/Source/timers.c"
  "${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS/cmsis_os.c"
  "${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM7/r0p1/port.c"
  "${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/FreeRTOS/Source/portable/MemMang/heap_4.c"
)

set(FATFS_SOURCES
  "${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/FatFs/src/ff.c"
  "${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/FatFs/src/diskio.c"
  "${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/FatFs/src/ff_gen_drv.c"
  "${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/FatFs/src/option/unicode.c"
)

set(SD_STORAGE_SOURCES
  "${CMAKE_SOURCE_DIR}/Core/Src/stm32746g_discovery_sd.c"
  "${CMAKE_SOURCE_DIR}/Core/Src/sd_diskio.c"
  "${CMAKE_SOURCE_DIR}/Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_sd.c"
  "${CMAKE_SOURCE_DIR}/Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_ll_sdmmc.c"
)

# Add sources to executable/library
target_sources(${BUILD_UNIT_0_NAME} PRIVATE
  # User defined sources
  "${CMAKE_SOURCE_DIR}/Core/Src/player.c"
  "${CMAKE_SOURCE_DIR}/Core/Src/app_ui.c"
  "${CMAKE_SOURCE_DIR}/Core/Src/app_ui_scene.c"
  "${CMAKE_SOURCE_DIR}/Core/Src/app_ui_player.c"
  "${CMAKE_SOURCE_DIR}/Core/Src/app_ui_assets.c"
  "${CMAKE_SOURCE_DIR}/Core/Src/app_audio.c"
  ${FREERTOS_SOURCES}
  ${FATFS_SOURCES}
  ${SD_STORAGE_SOURCES}
  ${LVGL_SOURCES}
)

# Add include directories
target_include_directories(${BUILD_UNIT_0_NAME} PRIVATE
  # User defined include directories
  "Middlewares/Third_Party/LVGL"
  "Middlewares/Third_Party/FatFs/src"
  "Middlewares/Third_Party/FreeRTOS/Source/include"
  "Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS"
  "Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM7/r0p1"
)
