/* Redirect: stubs for FreeRTOS.h */
#ifndef MOCK_FREERTOS_H
#define MOCK_FREERTOS_H

#include "mock_cmsis_os.h"

/* FreeRTOS heap size query - used in app_init.c */
uint32_t xPortGetFreeHeapSize(void);

#endif
