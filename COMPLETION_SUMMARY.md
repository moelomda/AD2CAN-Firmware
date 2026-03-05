# Project Status - Complete

## Overview

The AD2CAN sensor node firmware for the FAU Formula Student race car has been restructured from a monolithic codebase into a modular, layered RTOS architecture and is ready for handover.

## What This Node Does

- Reads 5 analog sensors (2x brake pressure, 1x steering, 2x APPS) via ADC + DMA
- Measures 2 wheel speed RPMs via timer input capture
- Transmits sensor data over CAN bus at 100 Hz (0x510, 0x511) and 10 Hz (0x512)
- Receives brake light commands (0x200) and heartbeat (0x100) via CAN
- Controls brake light GPIO and heartbeat LED

## Architecture

```
main.c -> App_Init() -> ADC_HAL / CAN_HAL / TIM_HAL (hardware)
                     -> SensorTask / CanTxTask / CanRxTask / BrakeLightTask (RTOS tasks)
                     -> sensors.c (calculation/filter logic)
       -> App_Start() -> FDCAN_Start + ADC DMA Start
       -> osKernelStart() -> scheduler runs 6 tasks
```

## Key Technical Details

| Parameter | Value |
|-----------|-------|
| MCU | STM32G473CE (Cortex-M4F) |
| SYSCLK | 64 MHz (HSE 16 MHz + PLL) |
| CAN | FDCAN1 @ 1 Mbps |
| RTOS | FreeRTOS, 16384 byte heap, 6 tasks |
| ADC | 6-channel continuous DMA scan |
| Timers | TIM3 + TIM15 input capture (64 MHz) |

## Files Changed (v2.1.0 - February 7, 2026)

| File | Change |
|------|--------|
| FreeRTOSConfig.h | Heap 10240 -> 16384 |
| High_VoltageTeam.ioc | FDCAN timing, NVIC, heap synced with code |
| can_tx_task.c | WS status read via IDR register (not HAL_GPIO_ReadPin) |
| main.c | Simplified to use App_Init() / App_Start() |
| app_init.c | Now handles FDCAN start + ADC start |

## Documentation

See **README.md** for complete program flow, CAN message formats, pin mapping, and all technical details.

See **DOCUMENTATION_INDEX.md** for a guide to all documentation files.

---

**Status:** Production Ready
**Version:** 2.1.0
**Date:** February 7, 2026
