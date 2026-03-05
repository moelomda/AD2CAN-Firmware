#ifndef DEBUG_PRINTF_H_
#define DEBUG_PRINTF_H_

#include <stdio.h>

/* ITM printf redirect - output goes to SWV ITM Data Console in STM32CubeIDE */
#define DEBUG_ENABLE 1

#if DEBUG_ENABLE
  #define DBG_PRINT(...) printf(__VA_ARGS__)
#else
  #define DBG_PRINT(...)
#endif

#endif /* DEBUG_PRINTF_H_ */
