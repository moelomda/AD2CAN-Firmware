# 🎯 QUICK BREAKPOINT REFERENCE CARD

## Copy this into your debugger session notes

---

## **BREAKPOINT #1: ADC Callback Entry**
📁 File: `App/Source/ad2can_hw.c`
🔧 Function: `HAL_ADC_ConvCpltCallback`
📍 Line: `if (hadc->Instance == ADC1)`
⏰ Expected: Hits 6 times per ADC sequence
🔎 Watch: None needed

---

## **BREAKPOINT #2: Conversion Counter** ⭐ MOST IMPORTANT
📁 File: `App/Source/ad2can_hw.c`
🔧 Function: `HAL_ADC_ConvCpltCallback`
📍 Line: `if (conversion_count < ADC_NUMBER_OF_CHANNELS)`
⏰ Expected: Hits 5 times, skips on 6th
🔎 Watch:
   - `conversion_count` → should be 1, 2, 3, 4, 5 (then resets to 0)
   - `ADC_NUMBER_OF_CHANNELS` → should be 6

---

## **BREAKPOINT #3: Queue Put**
📁 File: `App/Source/ad2can_hw.c`
🔧 Function: `HAL_ADC_ConvCpltCallback`
📍 Line: `osStatus_t queue_status = osMessageQueuePut(...)`
⏰ Expected: Hits once every 6 callbacks (~every 10ms)
🔎 Watch:
   - `queue_status` → should be `osOK` (0)
   - `g_adcBuffer[0]` → Brake Front (PA0) - should be ~400-3610
   - `g_adcBuffer[1]` → Brake Rear (PA1) - should be ~400-3610
   - `g_adcBuffer[2]` → Steering (PA5) - should be ~1736-3561
   - `g_adcBuffer[3]` → APPS1 (PB11) - should be variable
   - `g_adcBuffer[4]` → APPS2 (PB12) - should be variable
   - `g_adcBuffer[5]` → Channel 14 (PB14) - value depends on sensor

---

## **BREAKPOINT #4: Task Receives Data** ⭐ CRITICAL
📁 File: `App/Source/ad2can.c`
🔧 Function: `SensorProcessingTask`
📍 Line: `if (osMessageQueueGet(adcQueue, received_adc_data, NULL, osWaitForever) == osOK)`
⏰ Expected: Hits shortly after Breakpoint #3
🔎 Watch:
   - `received_adc_data[0]` → Should match `g_adcBuffer[0]`
   - `received_adc_data[1]` → Should match `g_adcBuffer[1]`
   - `received_adc_data[2]` → Should match `g_adcBuffer[2]`
   - `received_adc_data[3]` → Should match `g_adcBuffer[3]`
   - `received_adc_data[4]` → Should match `g_adcBuffer[4]`
   - `received_adc_data[5]` → Should match `g_adcBuffer[5]`
   - `adcQueue` → Should not be NULL

---

## **BREAKPOINT #5: CAN Gets Processed Data**
📁 File: `App/Source/ad2can_can.c`
🔧 Function: `CanTxTask`
📍 Line: `if (osMutexAcquire(g_sensorDataMutexHandle, 10) == osOK)`
⏰ Expected: Hits every ~10ms
🔎 Watch:
   - `local_sensor_data.brake_pressure_front` → Calculated pressure (0-20000)
   - `local_sensor_data.brake_pressure_rear` → Calculated pressure (0-20000)
   - `local_sensor_data.steering_angle` → Angle in degrees (-900 to +900)
   - `local_sensor_data.apps1` → Raw ADC value
   - `local_sensor_data.apps2` → Raw ADC value

---

## 🚨 TROUBLESHOOTING DECISION TREE

### **If Breakpoint #1 NEVER hits:**
❌ Problem: ADC+DMA not starting
✅ Solution:
   1. Check `HW_StartADC()` is called in main.c
   2. Verify DMA is enabled in CubeMX
   3. Check ADC clock is enabled

### **If Breakpoint #2 hits but counter stays at 1:**
❌ Problem: Callback firing per conversion, not per sequence
✅ Solution: This is NORMAL - the fix handles it!
   - Just verify counter increments: 1→2→3→4→5→6→0

### **If Breakpoint #3 never hits:**
❌ Problem: Conversion counter not reaching 6
✅ Solution:
   1. Verify `ADC_NUMBER_OF_CHANNELS` is defined as 6
   2. Check ADC is configured for 6 channels in CubeMX
   3. Inspect `conversion_count` at Breakpoint #2

### **If Breakpoint #3 hits but queue_status ≠ osOK:**
❌ Problem: Queue full
✅ Solution:
   1. Increase queue size in `HW_Init_ADC_RTOS()` from 4 to 16
   2. Check if sensor task is stuck (set Breakpoint #4)

### **If Breakpoint #4 NEVER hits:**
❌ Problem: Task not starting or queue empty
✅ Solution:
   1. Verify task was created in `App_Init_RTOS()`
   2. Check stack size (should be at least 512*4)
   3. Verify queue handle is not NULL

### **If data at Breakpoint #4 is all zeros:**
❌ Problem: ADC not reading sensors
✅ Solution:
   1. Check sensor connections
   2. Verify ADC pins are configured as analog inputs
   3. Check ADC reference voltage (VREF+)

### **If Breakpoint #5 never hits:**
❌ Problem: CAN task not starting or mutex issue
✅ Solution:
   1. Verify CAN task created in `CAN_Init_RTOS()`
   2. Check if mutex is stuck (someone holding it?)
   3. Verify `g_sensorDataMutexHandle` is valid

---

## 📝 DEBUGGING SESSION TEMPLATE

Copy this and fill it out during debugging:

```
=== DEBUGGING SESSION ===
Date: ______________
Time: ______________

[ ] Breakpoint #1 hits? YES / NO
    - If YES, how often? _____ times/second

[ ] Breakpoint #2 hits? YES / NO
    - conversion_count values seen: _________________

[ ] Breakpoint #3 hits? YES / NO
    - queue_status value: _____
    - g_adcBuffer[0] value: _____ (should be 400-3610)
    - g_adcBuffer[1] value: _____ (should be 400-3610)
    - g_adcBuffer[2] value: _____ (should be 1736-3561)

[ ] Breakpoint #4 hits? YES / NO
    - received_adc_data matches g_adcBuffer? YES / NO

[ ] Breakpoint #5 hits? YES / NO
    - local_sensor_data looks correct? YES / NO

LED Status:
[ ] Green LED: BLINKING / OFF / STUCK ON
[ ] Red LED: BLINKING / OFF / STUCK ON

Conclusion:
_________________________________________________
_________________________________________________
_________________________________________________
```

---

## 🎓 WHAT EACH VALUE MEANS

### **ADC Raw Values (0-4095)**
- **0**: Sensor at minimum (0V input)
- **2048**: Sensor at mid-range (~1.65V input)
- **4095**: Sensor at maximum (3.3V input)

### **Brake Pressure (0-20000)**
After processing via `HW_GetPressure()`:
- **0**: No pressure or sensor disconnected
- **10000**: ~50% brake pressure (~10 bar)
- **20000**: Maximum pressure (~20 bar)

### **Steering Angle (-900 to +900)**
After processing via `HW_GetSteeringAngle()`:
- **-900**: Full left turn
- **0**: Straight ahead
- **+900**: Full right turn

---

## 🏁 SUCCESS MARKERS

You'll know everything works when:
1. ✅ Breakpoint #1 hits 6 times rapidly
2. ✅ Breakpoint #2 shows counter: 1→2→3→4→5→(skip)→0
3. ✅ Breakpoint #3 hits once per 6 callbacks
4. ✅ Breakpoint #4 hits immediately after #3
5. ✅ Breakpoint #5 hits every ~10ms
6. ✅ Green LED blinks steadily
7. ✅ Red LED stays OFF
8. ✅ CAN messages appear on bus analyzer

---

**NOW GO DEBUG!** 🚀
