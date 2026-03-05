# Compilation Error Fixes

## Date: 2025-01-24

## Problem Summary
The project had **multiple definition errors** during linking, specifically:
1. `HAL_ADC_ConvCpltCallback` defined in multiple locations
2. `HAL_FDCAN_RxFifo0Callback` defined in multiple locations  
3. `HAL_TIM_IC_CaptureCallback` defined in multiple locations

## Root Cause
The HAL callback functions were defined in **both**:
- The new modular HAL layer (`App/HAL/adc_hal.c`, `App/HAL/can_hal.c`, `App/HAL/tim_hal.c`)
- The legacy application files (`App/Source/ad2can_hw.c`, `App/Source/ad2can_can.c`)

This caused the linker to find duplicate symbol definitions, resulting in compilation errors.

## Fixes Applied

### 1. Removed Duplicate HAL Callbacks from `ad2can_hw.c`
**File**: `App/Source/ad2can_hw.c`

**Removed**:
- `HAL_ADC_ConvCpltCallback()` function (lines 102-137)
- `HAL_TIM_IC_CaptureCallback()` function (lines 139-148)
- Global variable `uint16_t test_copy[ADC_NUMBER_OF_CHANNELS]`
- Global variables `g_rpm_left_ticks` and `g_rpm_right_ticks`
- Global variables `g_adcDataQueueHandle` and `g_adcBuffer`
- Extern declarations for `hadc1`, `hdma_adc1`, `htim3`, `htim15`

**Replaced with**:
- Calls to HAL layer functions:
  - `ADC_HAL_Init()` instead of direct queue creation
  - `ADC_HAL_Start()` instead of direct HAL calls
  - `TIM_HAL_Init()` instead of direct timer initialization
  - `ADC_HAL_GetQueueHandle()` to get queue handle
  - `TIM_HAL_GetRPM_Left()` and `TIM_HAL_GetRPM_Right()` for RPM values

**Added includes**:
```c
#include "adc_hal.h"
#include "tim_hal.h"
```

### 2. Removed Duplicate HAL Callback from `ad2can_can.c`
**File**: `App/Source/ad2can_can.c`

**Removed**:
- `HAL_FDCAN_RxFifo0Callback()` function (lines 148-168)
- Global variable `g_brakeCommandQueueHandle`

**Replaced with**:
- `CAN_HAL_Init()` call instead of direct queue creation
- `CAN_HAL_GetBrakeQueueHandle()` to get brake queue handle in `BrakeLightTask`

**Added includes**:
```c
#include "can_hal.h"
```

## Architecture Improvement

The fixes properly implement the **layered architecture**:

```
┌─────────────────────────────────────┐
│     Application Layer               │
│  (ad2can.c, ad2can_can.c,          │
│   ad2can_hw.c)                      │
└─────────────┬───────────────────────┘
              │ Uses clean API
              ▼
┌─────────────────────────────────────┐
│     HAL Abstraction Layer           │
│  (adc_hal.c, can_hal.c, tim_hal.c) │
│  - Contains HAL callbacks           │
│  - Manages hardware resources       │
└─────────────┬───────────────────────┘
              │ Uses STM32 HAL
              ▼
┌─────────────────────────────────────┐
│     STM32 HAL Layer                 │
│  (HAL drivers)                      │
└─────────────────────────────────────┘
```

### Benefits:
1. **No duplicate symbol definitions** - Each callback defined in exactly one place
2. **Better separation of concerns** - Hardware specifics isolated in HAL layer
3. **Easier maintenance** - Changes to hardware handling only affect HAL layer
4. **Cleaner application code** - Application layer uses simple, well-defined API

## Testing Recommendations

After applying these fixes:

1. **Clean the project**:
   ```
   Project → Clean... → Clean all projects
   ```

2. **Rebuild the project**:
   ```
   Project → Build All
   ```

3. **Verify no linker errors** - Check the Console and Problems tabs

4. **Test runtime behavior**:
   - ADC data acquisition
   - CAN message transmission/reception
   - RPM measurements
   - Brake light control

## Files Modified

1. `App/Source/ad2can_hw.c` - Removed HAL callbacks, added HAL layer calls
2. `App/Source/ad2can_can.c` - Removed HAL callback, added CAN HAL usage
3. This document created: `COMPILATION_FIXES.md`

## Next Steps

If compilation is successful:
1. Flash the firmware to the board
2. Test all functionality (ADC, CAN, RPM, brake light)
3. Monitor for any runtime issues
4. Consider further refactoring to use the new HAL layer consistently throughout

If there are still errors:
1. Check the Problems tab for specific error messages
2. Verify all files are saved
3. Try a clean rebuild
4. Check for any other duplicate definitions
