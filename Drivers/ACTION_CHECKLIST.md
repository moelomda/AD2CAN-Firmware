# Action Checklist - Project Setup

## Prerequisites
- STM32CubeIDE installed
- STM32G4 HAL drivers (included in project)
- ST-Link debugger

---

## Step 1: Configure STM32CubeIDE

Follow **IDE_CONFIGURATION.md** for detailed instructions.

**Quick version:**

### Include Paths (Properties -> C/C++ General -> Paths and Symbols -> Includes)
```
${workspace_loc:/${ProjName}/App/Inc}
${workspace_loc:/${ProjName}/App/Inc/HAL}
${workspace_loc:/${ProjName}/App/Inc/Tasks}
${workspace_loc:/${ProjName}/App/Inc/Drivers}
```

### Source Folders (Properties -> C/C++ General -> Paths and Symbols -> Source Location)
Verify these are present:
```
App
Core
Drivers
Middlewares
```

The `App/` entry includes all subdirectories (Src/HAL, Src/Tasks, Src/Drivers) automatically.

---

## Step 2: Clean Build

1. **Project -> Clean -> Clean all projects**
2. **Project -> Build All** (Ctrl+B)
3. Verify: **0 errors** in Console

---

## Step 3: Flash and Test

1. Connect ST-Link to board
2. **Run -> Debug** (F11)
3. Verify on hardware:

| Test | Expected Result |
|------|----------------|
| GREEN LED | Toggles every 1s (no CAN master) or controlled by heartbeat |
| RED LED | Should NOT be on (indicates queue overflow) |
| CAN Bus | 0x510 + 0x511 every 10ms, 0x512 every 100ms, 0x700 once at startup |
| Brake Light | Responds to CAN ID 0x200 byte[0] = 1 (ON) / 0 (OFF) |
| ADC values | Change when sensors are connected |
| RPM | Shows wheel speed when sensors spin |

---

## Troubleshooting

### Build Errors: "No such file or directory"
- Verify include paths are set (Step 1)
- Rebuild CDT index: Project -> C/C++ Index -> Rebuild

### Build Errors: "Multiple definition"
- Verify no old `ad2can_*.c` files exist in `App/Source/`
- Remove any stale source locations from build config

### Runtime: Tasks not running
- Check `App_Init()` and `App_Start()` are called in main.c (line ~152)
- Verify FreeRTOS heap is 16384 bytes in FreeRTOSConfig.h

### Runtime: No CAN output
- Check FDCAN1 NVIC is enabled (priority 6 in main.c MX_FDCAN1_Init)
- Check CAN transceiver is powered and bus is terminated (120 ohm)
- Verify baud rate matches other nodes (1 Mbps)

### Runtime: No RPM reading
- Check TIM3/TIM15 NVIC enabled in main.c
- Verify wheel speed sensor is producing edges
- Check RPM timeout (500ms, no pulse = RPM 0)

---

## Current Project Status

All steps below are COMPLETE:

- [x] Modular architecture restructured
- [x] Legacy ad2can files removed
- [x] main.c uses App_Init() / App_Start()
- [x] .ioc synced with code (FDCAN, NVIC, heap)
- [x] FreeRTOS heap = 16384 bytes
- [x] WS status reads via IDR register (not HAL_GPIO_ReadPin)
- [x] CAN baud rate = 1 Mbps verified
- [x] All documentation updated

---

**Last Updated:** February 7, 2026
