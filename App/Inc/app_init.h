/**
 * @file app_init.h
 * @brief Application initialization interface
 * 
 * Central initialization header for the entire application.
 * Call App_Init() from main to initialize all subsystems.
 */

#ifndef APP_INIT_H_
#define APP_INIT_H_

#include "cmsis_os.h"

/**
 * @brief Initialize all application subsystems
 * 
 * Initializes HAL layer, creates all RTOS tasks and synchronization primitives.
 * Should be called once from main() before starting the RTOS scheduler.
 */
void App_Init(void);

/**
 * @brief Start all hardware peripherals
 * 
 * Starts ADC, CAN, and Timer peripherals.
 * Should be called after App_Init() and before osKernelStart().
 */
void App_Start(void);

#endif /* APP_INIT_H_ */
