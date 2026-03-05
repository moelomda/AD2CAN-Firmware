# Documentation Index

## Start Here

1. **README.md** - Complete project overview, program flow, pin mapping, CAN messages
2. **IDE_CONFIGURATION.md** - STM32CubeIDE include paths and source folders setup
3. **PROJECT_STRUCTURE.md** - Architecture layers and module responsibilities

## All Documentation Files

| File | Purpose |
|------|---------|
| **README.md** | Main reference: program flow, pin map, CAN protocol, clock config, RTOS config |
| **PROJECT_STRUCTURE.md** | Architecture layers, data flow, build configuration |
| **IDE_CONFIGURATION.md** | STM32CubeIDE setup: include paths, source folders, troubleshooting |
| **ACTION_CHECKLIST.md** | Setup steps for a fresh checkout (IDE config, build, test) |
| **COMPILATION_FIXES.md** | Technical context: what ADC/callback issues were fixed and why |
| **VISUAL_SUMMARY.md** | Before/after diagrams of the restructuring |
| **RESTRUCTURING_COMPLETE.md** | Detailed changelog of the v1 to v2 restructuring |
| **COMPLETION_SUMMARY.md** | Executive summary of the restructuring project |
| **MIGRATION_GUIDE.md** | Maps old ad2can file structure to new modular structure |

## By Role

### New Developer
1. README.md (full overview + program flow)
2. IDE_CONFIGURATION.md (setup your IDE)
3. ACTION_CHECKLIST.md (build and test)

### Maintaining / Extending
1. README.md (CAN message formats, sensor formulas)
2. PROJECT_STRUCTURE.md (where to add new code)
3. app_config.h (all tunable constants)

### Understanding History
1. RESTRUCTURING_COMPLETE.md (what changed)
2. COMPILATION_FIXES.md (why certain patterns were used)
3. MIGRATION_GUIDE.md (old file to new file mapping)

---

**Last Updated:** February 7, 2026
**Project Version:** 2.1.0
