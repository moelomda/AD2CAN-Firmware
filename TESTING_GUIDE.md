# AD2CAN Sensor Node — Complete Hardware Testing Guide

**Project:** High Voltage Team — FAU Racing Formula Student  
**MCU:** STM32G473CE (Cortex-M4F, 64 MHz)  
**Date:** March 2026  

> **Note:** Before running hardware tests, all firmware logic has been validated
> with 120 automated software tests (unit + integration + acceptance).
> See `Tests/TEST_RESULTS.md` for full results.
> Run `cd Tests && run_tests.bat` to re-verify after any code changes.

---

## Table of Contents

1. [Equipment Required](#1-equipment-required)
2. [Wiring Setup](#2-wiring-setup)
3. [Debugger Configuration (STM32CubeIDE)](#3-debugger-configuration-stm32cubeide)
4. [Phase 1: Power-On & Boot Verification](#4-phase-1-power-on--boot-verification)
5. [Phase 2: RTOS Health Check](#5-phase-2-rtos-health-check)
6. [Phase 3: ADC & DMA Testing](#6-phase-3-adc--dma-testing)
7. [Phase 4: Sensor Calibration Testing](#7-phase-4-sensor-calibration-testing)
8. [Phase 5: CAN TX Testing](#8-phase-5-can-tx-testing)
9. [Phase 6: CAN RX Testing](#9-phase-6-can-rx-testing)
10. [Phase 7: Wheel Speed Sensor Testing](#10-phase-7-wheel-speed-sensor-testing)
11. [Phase 8: Brake Light Testing](#11-phase-8-brake-light-testing)
12. [Phase 9: Heartbeat & LED Testing](#12-phase-9-heartbeat--led-testing)
13. [Phase 10: Stress & Endurance Testing](#13-phase-10-stress--endurance-testing)
14. [Phase 11: Error Handling & Fault Injection](#14-phase-11-error-handling--fault-injection)
15. [Phase 12: Full Integration Test](#15-phase-12-full-integration-test)
16. [Test Results Log Template](#16-test-results-log-template)
17. [Troubleshooting](#17-troubleshooting)

---

## 1. Equipment Required

### Mandatory

| Item | Purpose |
|------|---------|
| STM32G473CE board | Target MCU |
| ST-Link V2/V3 debugger | Flash & debug via SWD (PA13/PA14) |
| CAN transceiver module (e.g., MCP2551, SN65HVD230, TJA1050) | CAN physical layer |
| CAN bus analyzer (e.g., PCAN-USB, Kvaser Leaf, CANable, USB2CAN) | Monitor & send CAN frames |
| 2x Brake pressure sensors (0–20 bar, 0.5–4.5V output) | Analog inputs PA0, PA1 |
| 1x Steering angle potentiometer / sensor | Analog input PB14 |
| 2x APPS (throttle pedal position sensors) | Analog inputs PB12, PB1 |
| 2x Wheel speed sensors (hall effect / inductive, 25 teeth) | Digital pulse inputs PA2, PB0 |
| Multimeter | Verify voltage levels |
| Oscilloscope (optional but recommended) | Verify CAN bus signals, WS pulses, ADC signals |
| Power supply (3.3V for MCU, 5V/12V for sensors as needed) | Power |
| 120Ω termination resistor | CAN bus termination |
| Breadboard + jumper wires | Connections |

### Optional (for Advanced Testing)

| Item | Purpose |
|------|---------|
| Function/signal generator | Simulate wheel speed pulses |
| Potentiometers (10kΩ) | Simulate analog sensors |
| Second STM32 + CAN transceiver | Simulate CAN master node |
| Current meter | Measure total board consumption |
| Logic analyzer | Debug timing issues |

---

## 2. Wiring Setup

### 2.1 CAN Bus Wiring

```
STM32 Board              CAN Transceiver              CAN Analyzer
+-----------+           +---------------+             +----------+
| PA12 (TX) |------>----|  TXD      CANH|-----+-------|  CANH    |
| PA11 (RX) |------<----|  RXD      CANL|-----+-------|  CANL    |
| 3.3V      |-----------|  VCC          |     |       |          |
| GND       |-----------|  GND          |     |       |  GND     |
+-----------+           +---------------+   [120Ω]    +----------+
                                              |
                                             GND
```

> **IMPORTANT:** Place a 120Ω termination resistor between CANH and CANL at each end of the bus. If only two nodes, one resistor at each end.

### 2.2 Sensor Wiring

```
Brake Pressure Front:  Sensor OUT → PA0  (ADC1_IN1)
Brake Pressure Rear:   Sensor OUT → PA1  (ADC1_IN2)
Steering Angle:        Sensor OUT → PB14 (ADC1_IN5)
APPS1:                 Sensor OUT → PB12 (ADC1_IN11)
APPS2:                 Sensor OUT → PB1  (ADC1_IN12)

Left Wheel Speed:      Sensor OUT → PA2  (TIM15_CH1)
Right Wheel Speed:     Sensor OUT → PB0  (TIM3_CH3)
```

> **Note:** All ADC inputs expect 0–3.3V. If sensors output 0–5V, use a voltage divider or level shifter. Do NOT exceed 3.3V on any MCU pin.

### 2.3 Output Wiring

```
Brake Light:  PA8 (BL_ENABLE) → MOSFET gate or relay → Brake light
LED Green:    PB6 (on-board or external LED)
LED Red:      PB7 (on-board or external LED)
```

### 2.4 Debug Wiring

```
SWD:  PA13 = SWDIO
      PA14 = SWCLK
      GND  = GND
      3.3V = VCC (if powering from debugger)
```

---

## 3. Debugger Configuration (STM32CubeIDE)

### 3.1 Flash & Debug Launch

1. Open STM32CubeIDE, import the project.
2. Build: **Project → Build All** (Ctrl+B). Verify 0 errors.
3. Debug: **Run → Debug As → STM32 Cortex-M C/C++ Application**.
4. Select the `.elf` from `Debug/` folder.

### 3.2 Live Expressions (Watch Variables)

Add these to the **Live Expressions** window for real-time monitoring:

| Variable | What It Shows |
|----------|---------------|
| `dbg_init_stage` | Init progress (should reach 9) |
| `dbg_free_heap` | Free RTOS heap in bytes after init |
| `dbg_adc_callback_count` | Number of ADC DMA completions |
| `dbg_adc_queue_fail` | Number of ADC queue overflow events |
| `dbg_sensor_count` | Number of SensorTask processing cycles |
| `dbg_cantx_count` | Number of CanTxTask cycles |
| `dbg_canrx_count` | Number of CanRxTask cycles (1/sec) |

### 3.3 SWV ITM Console (printf output)

1. In Debug Configuration → **Debugger tab** → enable **SWV** → set Core Clock to **64000000**.
2. Open **Window → Show View → SWV → SWV ITM Data Console**.
3. Click **Configure** (gear icon) → enable **Port 0**.
4. Click **Start Trace** (red circle).
5. You should see messages like:
   ```
   [START] App_Start...
   [START] FDCAN started OK
   [START] ADC started OK
   [MAIN] About to start kernel. Free heap: XXXXX
   ```

---

## 4. Phase 1: Power-On & Boot Verification

**Goal:** Confirm the MCU boots, clock is correct, and reaches the RTOS scheduler.

### Test 1.1: Basic Power-On

| Step | Action | Expected Result | Pass/Fail |
|------|--------|-----------------|-----------|
| 1 | Connect ST-Link, flash firmware | Flash succeeds, no errors | |
| 2 | Release reset / start debug | Green LED starts blinking (~1 Hz) | |
| 3 | Check red LED | OFF (steady, not blinking) | |
| 4 | Read `dbg_init_stage` in debugger | Value = **9** | |

### Test 1.2: Clock Verification

| Step | Action | Expected Result | Pass/Fail |
|------|--------|-----------------|-----------|
| 1 | Measure green LED toggle period | ~1 second ON, ~1 second OFF (±50ms) | |
| 2 | Read `SystemCoreClock` in debugger | Value = **64000000** | |
| 3 | (Optional) Measure MCO output or SysTick with scope | 64 MHz derived timing | |

### Test 1.3: ITM Debug Output

| Step | Action | Expected Result | Pass/Fail |
|------|--------|-----------------|-----------|
| 1 | Open SWV ITM Data Console | Console opens | |
| 2 | Reset target, start trace | See `[START] App_Start...` messages | |
| 3 | Verify "FDCAN started OK" appears | FDCAN peripheral initialized | |
| 4 | Verify "ADC started OK" appears | ADC+DMA started | |

---

## 5. Phase 2: RTOS Health Check

**Goal:** Verify all tasks are running and heap is not exhausted.

### Test 2.1: Heap Usage

| Step | Action | Expected Result | Pass/Fail |
|------|--------|-----------------|-----------|
| 1 | Read `dbg_free_heap` | Value > 5000 (sufficient headroom) | |
| 2 | Calculate used heap | 32768 - `dbg_free_heap` = used | |
| 3 | Verify no `Error_Handler()` hit | MCU not stuck in infinite loop | |

### Test 2.2: Task Activity Counters

Wait 10 seconds after boot, then check:

| Variable | Expected | Meaning | Pass/Fail |
|----------|----------|---------|-----------|
| `dbg_adc_callback_count` | > 0 and incrementing | ADC DMA interrupts firing | |
| `dbg_sensor_count` | > 0 and incrementing | SensorTask processing data | |
| `dbg_cantx_count` | > 500 (10s × 100Hz) | CanTxTask running at 100 Hz | |
| `dbg_canrx_count` | ~10 (10s × 1Hz) | CanRxTask running at 1 Hz | |
| `dbg_adc_queue_fail` | 0 | No queue overflows | |

### Test 2.3: Task Ratio Sanity

| Check | Formula | Expected | Pass/Fail |
|-------|---------|----------|-----------|
| ADC→Sensor match | `dbg_sensor_count` ≈ `dbg_adc_callback_count` - 1 | Within ±5 | |
| CAN TX rate | `dbg_cantx_count` / elapsed_seconds | ~100/sec | |
| CAN RX rate | `dbg_canrx_count` / elapsed_seconds | ~1/sec | |

---

## 6. Phase 3: ADC & DMA Testing

**Goal:** Verify all 6 ADC channels read correct values via DMA.

### Test 3.1: ADC with No Sensors (Floating Pins)

| Step | Action | Expected Result | Pass/Fail |
|------|--------|-----------------|-----------|
| 1 | Boot with no sensors connected | No crash, green LED blinks | |
| 2 | Check `dbg_adc_callback_count` | Incrementing | |
| 3 | Check `dbg_adc_queue_fail` | 0 | |

> **Note:** ADC values on floating pins will be random noise — this is normal.

### Test 3.2: ADC with Known Voltage (Potentiometer Simulation)

Use a potentiometer (10kΩ) as voltage divider on each ADC pin, or apply a known voltage:

| Step | Action | Expected Result | Pass/Fail |
|------|--------|-----------------|-----------|
| 1 | Apply 0V (GND) to PA0 | Breakpoint in SensorTask: `received_adc_data[0]` ≈ 0 | |
| 2 | Apply 1.65V (half VCC) to PA0 | `received_adc_data[0]` ≈ 2048 (midrange 12-bit) | |
| 3 | Apply 3.3V to PA0 | `received_adc_data[0]` ≈ 4095 | |
| 4 | Repeat for PA1 (ch index 1) | Same pattern | |
| 5 | Repeat for PB14 (ch index 2) | Same pattern | |
| 6 | Repeat for PB12 (ch index 3) | Same pattern | |
| 7 | Repeat for PB1 (ch index 4) | Same pattern | |
| 8 | Repeat for PB11 (ch index 5) | Same pattern (unused, but should read) | |

### Test 3.3: ADC Conversion Triggering

| Step | Action | Expected Result | Pass/Fail |
|------|--------|-----------------|-----------|
| 1 | Watch `dbg_adc_callback_count` increment | Consistent increments | |
| 2 | Watch `dbg_sensor_count` increment | Should match ADC count closely | |
| 3 | Verify ADC restarts after each SensorTask cycle | No stalling (count keeps going) | |

---

## 7. Phase 4: Sensor Calibration Testing

**Goal:** Verify raw ADC → engineering unit conversions are correct.

### Test 4.1: Brake Pressure Calculation

Formula: `pressure = (adc - 400) * 20000 / (3610 - 400)`

Set a breakpoint in `SensorTask_Run()` and inspect `local_data.brake_pressure_front`:

| Input ADC Value | Expected Pressure | Meaning | Pass/Fail |
|-----------------|-------------------|---------|-----------|
| 0 (< 400) | 0 | Below range, clamped | |
| 400 | 0 | Minimum valid | |
| 2005 | ~10000 | Midpoint (~10.0 bar) | |
| 3610 | 20000 | Maximum (~20.0 bar) | |
| 4095 (> 3610) | 0 | Above range, clamped | |

> Test both front (channel 0 → PA0) and rear (channel 1 → PA1).

### Test 4.2: Steering Angle Calculation

Formula: `angle = (1800 / (3561 - 1736)) * (adc - 1736) - 900`

Inspect `local_data.steering_angle`:

| Input ADC Value | Expected Angle | Meaning | Pass/Fail |
|-----------------|----------------|---------|-----------|
| 1736 | -900 | Full left (-90.0°) | |
| 2648 | ~0 | Center (0.0°) | |
| 3561 | +900 | Full right (+90.0°) | |

### Test 4.3: APPS Raw Values

Inspect `local_data.apps1` and `local_data.apps2`:

| Action | Expected | Pass/Fail |
|--------|----------|-----------|
| Pedal not pressed | Low ADC value (near 0 or sensor-specific idle) | |
| Pedal half pressed | Midrange value | |
| Pedal fully pressed | High ADC value (near 4095 or sensor max) | |
| APPS1 and APPS2 track together | Values should be correlated (redundancy check) | |

---

## 8. Phase 5: CAN TX Testing

**Goal:** Verify all CAN messages are transmitted correctly.

### Prerequisites
- CAN transceiver connected and powered
- CAN analyzer connected to the bus with 120Ω termination
- CAN analyzer software open and configured for **1 Mbps**, **Standard ID**, **Classic CAN**

### Test 5.1: Startup Message (0x700)

| Step | Action | Expected Result | Pass/Fail |
|------|--------|-----------------|-----------|
| 1 | Reset the MCU | Exactly 1 frame with ID 0x700 appears | |
| 2 | Check data | DLC=1, Data[0] = 0xAA | |
| 3 | Wait 10 seconds | No more 0x700 messages | |

### Test 5.2: Sensor Frame 1 (0x510, every 10ms)

| Step | Action | Expected Result | Pass/Fail |
|------|--------|-----------------|-----------|
| 1 | Watch CAN traffic | 0x510 frames at ~100 Hz (every ~10ms) | |
| 2 | Check DLC | 8 bytes | |
| 3 | Decode bytes 0-1 (LE) | APPS1 raw ADC value | |
| 4 | Decode bytes 2-3 (LE) | APPS2 raw ADC value | |
| 5 | Decode bytes 4-5 (LE, signed) | Steering angle in 0.1° units | |
| 6 | Decode bytes 6-7 (LE) | Brake pressure front (0–20000) | |
| 7 | Move APPS pedal | Bytes 0-3 change correspondingly | |
| 8 | Turn steering | Bytes 4-5 change correspondingly | |
| 9 | Press front brake | Bytes 6-7 change correspondingly | |

**Byte map for 0x510:**
```
Byte [0][1]  = APPS1          (uint16_t, little-endian)
Byte [2][3]  = APPS2          (uint16_t, little-endian)
Byte [4][5]  = Steering Angle (int16_t,  little-endian, 0.1° units)
Byte [6][7]  = Brake Front    (uint16_t, little-endian, 0–20000)
```

### Test 5.3: Sensor Frame 2 (0x511, every 10ms)

| Step | Action | Expected Result | Pass/Fail |
|------|--------|-----------------|-----------|
| 1 | Watch CAN traffic | 0x511 frames at ~100 Hz | |
| 2 | Check DLC | 7 bytes | |
| 3 | Decode bytes 0-1 (LE) | RPM left (0 if wheels not spinning) | |
| 4 | Decode bytes 2-3 (LE) | RPM right (0 if wheels not spinning) | |
| 5 | Decode bytes 4-5 (LE) | Brake pressure rear (0–20000) | |
| 6 | Decode byte 6 | WS status: bit0=left pin, bit1=right pin | |
| 7 | Press rear brake | Bytes 4-5 change | |

**Byte map for 0x511:**
```
Byte [0][1]  = RPM Left       (uint16_t, little-endian)
Byte [2][3]  = RPM Right      (uint16_t, little-endian)
Byte [4][5]  = Brake Rear     (uint16_t, little-endian, 0–20000)
Byte [6]     = WS Status      (bit0 = left pin state, bit1 = right pin state)
```

### Test 5.4: DV Sensors Frame (0x512, every 100ms)

| Step | Action | Expected Result | Pass/Fail |
|------|--------|-----------------|-----------|
| 1 | Watch CAN traffic | 0x512 frames at ~10 Hz (every ~100ms) | |
| 2 | Check DLC | 4 bytes | |
| 3 | Decode bytes 0-1 (LE) | Brake pressure rear | |
| 4 | Decode bytes 2-3 (LE) | Brake pressure front | |
| 5 | Verify rate | Exactly 1 per every 10 frames of 0x510 | |

**Byte map for 0x512:**
```
Byte [0][1]  = Brake Rear     (uint16_t, little-endian, 0–20000)
Byte [2][3]  = Brake Front    (uint16_t, little-endian, 0–20000)
```

### Test 5.5: CAN TX Timing Accuracy

| Step | Action | Expected Result | Pass/Fail |
|------|--------|-----------------|-----------|
| 1 | Record timestamps of consecutive 0x510 | Period = 10ms ± 2ms | |
| 2 | Record timestamps of consecutive 0x512 | Period = 100ms ± 10ms | |
| 3 | Run for 60 seconds, count 0x510 frames | ~6000 frames ± 100 | |
| 4 | Run for 60 seconds, count 0x512 frames | ~600 frames ± 10 | |

---

## 9. Phase 6: CAN RX Testing

**Goal:** Verify the node correctly receives and processes CAN commands.

### Prerequisites
- CAN analyzer configured to **transmit** frames at 1 Mbps

### Test 6.1: Brake Command Reception (0x200)

| Step | Action | Expected Result | Pass/Fail |
|------|--------|-----------------|-----------|
| 1 | Send CAN frame: ID=0x200, DLC=1, Data=[0x01] | PA8 goes HIGH (measure with multimeter: ~3.3V) | |
| 2 | Send CAN frame: ID=0x200, DLC=1, Data=[0x00] | PA8 goes LOW (measure: ~0V) | |
| 3 | Send 0x200 [0x01], then 0x200 [0x00] rapidly | PA8 toggles accordingly, no crash | |
| 4 | Send 10 rapid 0x200 [0x01] commands | PA8 stays HIGH (queue depth 1, last wins) | |

### Test 6.2: Heartbeat Reception (0x100)

| Step | Action | Expected Result | Pass/Fail |
|------|--------|-----------------|-----------|
| 1 | Observe green LED blinking (~1 Hz) | Baseline: no heartbeat, local toggle | |
| 2 | Send CAN frame: ID=0x100, DLC=1, Data=[0x01] | Green LED turns ON (steady) | |
| 3 | Send CAN frame: ID=0x100, DLC=1, Data=[0x00] | Green LED turns OFF (steady) | |
| 4 | Stop sending heartbeat, wait >1 second | Green LED resumes blinking | |
| 5 | Send periodic 0x100 [0x01] every 500ms | Green LED stays ON continuously | |

### Test 6.3: CAN Filter Rejection

| Step | Action | Expected Result | Pass/Fail |
|------|--------|-----------------|-----------|
| 1 | Send CAN frame: ID=0x300, DLC=1, Data=[0x01] | No effect on any output | |
| 2 | Send CAN frame: ID=0x050, DLC=8, Data=[...] | No effect | |
| 3 | Send CAN frame: ID=0x7FF, DLC=0 | No effect | |
| 4 | Send extended ID frame (29-bit, ID=0x200) | No effect (only standard IDs accepted) | |
| 5 | Verify only 0x200 and 0x100 are processed | No side effects from other IDs | |

---

## 10. Phase 7: Wheel Speed Sensor Testing

**Goal:** Verify RPM measurement from both wheel speed sensors.

### Test 7.1: Using Signal Generator (Simulated Pulses)

If using a function generator to simulate the wheel speed sensor:

**Setup:** Square wave, 3.3V amplitude, 50% duty cycle, rising-edge trigger.

| Frequency | Expected RPM | Calculation | Pass/Fail |
|-----------|-------------|-------------|-----------|
| 0 Hz (no signal) | 0 | Timeout after 500ms | |
| 100 Hz | 240 | (100 × 60) / 25 teeth | |
| 500 Hz | 1200 | (500 × 60) / 25 | |
| 1000 Hz | 2400 | (1000 × 60) / 25 | |
| 5000 Hz | 12000 | (5000 × 60) / 25 | |

Apply to **PA2** (left) and **PB0** (right) independently.

**How to verify:** Check `RPM Left` (bytes 0-1) and `RPM Right` (bytes 2-3) in CAN message **0x511**.

### Test 7.2: Using Real Wheel Speed Sensors

| Step | Action | Expected Result | Pass/Fail |
|------|--------|-----------------|-----------|
| 1 | Wheel stationary | RPM = 0 in 0x511 | |
| 2 | Slowly rotate wheel by hand | Small RPM value appears (>0) | |
| 3 | Spin wheel faster | RPM increases proportionally | |
| 4 | Stop wheel | RPM drops to 0 within 500ms | |
| 5 | Verify left vs. right independence | Only the moved wheel's RPM changes | |

### Test 7.3: RPM Timeout

| Step | Action | Expected Result | Pass/Fail |
|------|--------|-----------------|-----------|
| 1 | Apply 1 kHz signal to PA2 | RPM left shows ~2400 | |
| 2 | Disconnect signal | RPM drops to 0 within 500ms | |
| 3 | Reconnect signal | RPM returns to ~2400 within one pulse period | |

### Test 7.4: WS Pin Status (Byte 6 of 0x511)

| Step | Action | Expected Result | Pass/Fail |
|------|--------|-----------------|-----------|
| 1 | Left sensor connected, outputting HIGH | Byte 6 bit 0 = 1 | |
| 2 | Left sensor LOW | Byte 6 bit 0 = 0 | |
| 3 | Right sensor HIGH | Byte 6 bit 1 = 1 | |
| 4 | Right sensor LOW | Byte 6 bit 1 = 0 | |
| 5 | Both HIGH | Byte 6 = 0x03 | |
| 6 | Both LOW | Byte 6 = 0x00 | |

---

## 11. Phase 8: Brake Light Testing

**Goal:** Verify brake light output (PA8) responds correctly to CAN commands.

### Test 8.1: Brake Light On/Off

| Step | Action | Expected Result | Pass/Fail |
|------|--------|-----------------|-----------|
| 1 | Power on, no CAN command sent | PA8 = LOW (brake light OFF) | |
| 2 | Send 0x200 [0x01] | PA8 = HIGH (3.3V, brake light ON) | |
| 3 | Send 0x200 [0x00] | PA8 = LOW (0V, brake light OFF) | |
| 4 | Send 0x200 [0x01], measure with scope | Clean HIGH level, no glitches | |

### Test 8.2: Brake Light with Real Load

| Step | Action | Expected Result | Pass/Fail |
|------|--------|-----------------|-----------|
| 1 | Connect brake light via MOSFET/relay to PA8 | Light OFF at boot | |
| 2 | Send 0x200 [0x01] | Brake light physically turns ON | |
| 3 | Send 0x200 [0x00] | Brake light physically turns OFF | |
| 4 | Rapid on/off cycling (10 Hz) | Light toggles cleanly | |

---

## 12. Phase 9: Heartbeat & LED Testing

**Goal:** Verify green and red LED behavior under all conditions.

### Test 9.1: Green LED — Local Heartbeat Mode

| Step | Action | Expected Result | Pass/Fail |
|------|--------|-----------------|-----------|
| 1 | Boot with no CAN master | Green LED toggles every ~1 second | |
| 2 | Time 10 toggle cycles | 10 seconds ± 0.5s | |
| 3 | Verify LED is on PB6 | Correct pin | |

### Test 9.2: Green LED — Network Master Mode

| Step | Action | Expected Result | Pass/Fail |
|------|--------|-----------------|-----------|
| 1 | Send 0x100 [0x01] every 500ms | Green LED stays ON (no toggle) | |
| 2 | Switch to 0x100 [0x00] every 500ms | Green LED stays OFF | |
| 3 | Stop sending heartbeat | Green LED resumes 1 Hz toggle within 1-2s | |
| 4 | Send single 0x100 [0x01], then nothing | LED ON briefly, then resumes toggle after ~1s | |

### Test 9.3: Red LED — Error Indicator

| Step | Action | Expected Result | Pass/Fail |
|------|--------|-----------------|-----------|
| 1 | Normal operation | Red LED OFF | |
| 2 | Check `dbg_adc_queue_fail` = 0 | Confirmed no ADC overflow | |
| 3 | (Fault injection: see Phase 11) | Red LED toggles on error | |

---

## 13. Phase 10: Stress & Endurance Testing

**Goal:** Verify stability over extended operation.

### Test 10.1: Long-Run Stability (30 minutes)

| Step | Action | Expected Result | Pass/Fail |
|------|--------|-----------------|-----------|
| 1 | Start all sensors + CAN bus | Normal operation | |
| 2 | Run for 30 minutes | Green LED still blinking or controlled | |
| 3 | Check `dbg_adc_queue_fail` | Still 0 | |
| 4 | Check `dbg_cantx_count` | ~180,000 (30min × 100Hz) | |
| 5 | Check `dbg_sensor_count` | Close to `dbg_adc_callback_count` | |
| 6 | CAN analyzer: no error frames | Bus healthy | |
| 7 | No HardFault (MCU responsive) | Stable | |

### Test 10.2: CAN Bus Load Test

| Step | Action | Expected Result | Pass/Fail |
|------|--------|-----------------|-----------|
| 1 | Flood bus with unrelated CAN frames (other IDs, 100% load) | Node still transmits 0x510/0x511 | |
| 2 | Send 0x200 brake commands during bus flood | Brake light still responds | |
| 3 | Check for CAN error frames from this node | None | |

### Test 10.3: Heap Leak Check

| Step | Action | Expected Result | Pass/Fail |
|------|--------|-----------------|-----------|
| 1 | Record `dbg_free_heap` at boot | Note value: _____ | |
| 2 | Check `dbg_free_heap` after 30 min | Same value (no leak) | |

---

## 14. Phase 11: Error Handling & Fault Injection

**Goal:** Verify the system handles faults gracefully.

### Test 11.1: Sensor Disconnection

| Step | Action | Expected Result | Pass/Fail |
|------|--------|-----------------|-----------|
| 1 | Disconnect brake pressure front (PA0) | ADC reads noise, pressure = 0 (out of range clamp) | |
| 2 | Disconnect steering (PB14) | Steering angle = random but system runs | |
| 3 | Disconnect both APPS | APPS1/APPS2 = noise, no crash | |
| 4 | Reconnect all | Normal values resume | |

### Test 11.2: CAN Bus Disconnection

| Step | Action | Expected Result | Pass/Fail |
|------|--------|-----------------|-----------|
| 1 | Disconnect CAN_H wire | TX will fail (no ACK), but MCU doesn't crash | |
| 2 | Check `dbg_cantx_count` | Still incrementing (task keeps trying) | |
| 3 | Check green LED | Still blinking (CanRxTask alive) | |
| 4 | Reconnect CAN_H | CAN TX resumes successfully | |

### Test 11.3: Wheel Speed Sensor Noise

| Step | Action | Expected Result | Pass/Fail |
|------|--------|-----------------|-----------|
| 1 | Apply very fast signal (50 kHz) to PA2 | RPM saturates at 0xFFFF or reasonable max | |
| 2 | Apply very slow signal (1 Hz) | RPM = low value, timeout may kick in | |
| 3 | Remove signal while spinning | RPM drops to 0 after 500ms timeout | |

### Test 11.4: Power Cycle Recovery

| Step | Action | Expected Result | Pass/Fail |
|------|--------|-----------------|-----------|
| 1 | Normal operation confirmed | Baseline | |
| 2 | Power off abruptly (pull power) | — | |
| 3 | Power on | Green LED blinks, 0x700 startup msg sent, normal operation | |
| 4 | Repeat 5 times | Consistent boot every time | |

---

## 15. Phase 12: Full Integration Test

**Goal:** Validate complete system behavior as it would operate on the car.

### Test 12.1: Full Sensor Integration

| Step | Action | Expected Result | Pass/Fail |
|------|--------|-----------------|-----------|
| 1 | All 5 analog sensors connected | ADC values in expected ranges | |
| 2 | Both wheel speed sensors connected | RPM reads 0 (wheels stationary) | |
| 3 | CAN bus with analyzer connected | 0x510 + 0x511 at 100 Hz, 0x512 at 10 Hz | |
| 4 | All CAN data values match sensor state | Cross-check physical state vs. CAN data | |

### Test 12.2: Dynamic Drive Simulation

| Step | Action | Expected Result | Pass/Fail |
|------|--------|-----------------|-----------|
| 1 | Press accelerator pedal | APPS1, APPS2 increase in 0x510 | |
| 2 | Turn steering left | Steering angle goes negative in 0x510 | |
| 3 | Turn steering right | Steering angle goes positive | |
| 4 | Press front brake | Brake front increases in 0x510 bytes 6-7 | |
| 5 | Press rear brake | Brake rear increases in 0x511 bytes 4-5 | |
| 6 | Spin left wheel | RPM left increases in 0x511 bytes 0-1 | |
| 7 | Spin right wheel | RPM right increases in 0x511 bytes 2-3 | |
| 8 | Send brake light ON command | Brake light turns ON | |
| 9 | Release everything | All values return to idle | |

### Test 12.3: CAN Master Simulation

Simulate a full master node sending periodic heartbeat + brake commands:

| Step | Action | Expected Result | Pass/Fail |
|------|--------|-----------------|-----------|
| 1 | Send 0x100 [0x01] every 500ms | Green LED stays ON | |
| 2 | Simultaneously send 0x200 [0x01] | Brake light ON + green LED ON | |
| 3 | Stop heartbeat | Green LED resumes toggle, brake light unchanged | |
| 4 | Send 0x200 [0x00] | Brake light OFF | |

### Test 12.4: Timing Cross-Check

Use CAN analyzer timestamp log:

| Check | Method | Expected | Pass/Fail |
|-------|--------|----------|-----------|
| 0x510 period | Δt between consecutive frames | 10ms ± 2ms | |
| 0x511 period | Δt between consecutive frames | 10ms ± 2ms (same as 0x510) | |
| 0x512 period | Δt between consecutive frames | 100ms ± 10ms | |
| 0x510 and 0x511 gap | Δt between 0x510 and next 0x511 | < 1ms (sent back-to-back) | |
| Brake light latency | Time from 0x200 TX to PA8 toggle | < 5ms | |
| Heartbeat LED latency | Time from 0x100 TX to LED change | < 1ms (ISR direct) | |

---

## 16. Test Results Log Template

Copy this template for each test session:

```
==============================================================
TEST SESSION LOG
==============================================================
Date:        _______________
Tester:      _______________
FW Version:  _______________
Board ID:    _______________
CAN Analyzer: ______________

ENVIRONMENT:
- Power supply voltage:  ____ V
- CAN termination:       [ ] 120Ω at both ends
- Sensors connected:     [ ] BPF  [ ] BPR  [ ] Steering  [ ] APPS1  [ ] APPS2
- WS sensors connected:  [ ] Left [ ] Right
- CAN transceiver:       ____________

BOOT CHECK:
- dbg_init_stage:        ____ (expected: 9)
- dbg_free_heap:         ____ bytes
- SystemCoreClock:       ____ (expected: 64000000)
- Green LED blinking:    [ ] Yes [ ] No
- Red LED:               [ ] OFF [ ] Blinking (error!)
- ITM printf working:    [ ] Yes [ ] No

AFTER 60 SECONDS:
- dbg_adc_callback_count: ____
- dbg_sensor_count:       ____
- dbg_cantx_count:        ____ (expected: ~6000)
- dbg_canrx_count:        ____ (expected: ~60)
- dbg_adc_queue_fail:     ____ (expected: 0)

CAN BUS:
- 0x700 startup received: [ ] Yes [ ] No  Data: ____
- 0x510 rate:             ____ frames/sec (expected: ~100)
- 0x511 rate:             ____ frames/sec (expected: ~100)
- 0x512 rate:             ____ frames/sec (expected: ~10)
- Error frames:           ____ (expected: 0)

CAN RX TESTS:
- 0x200 [0x01] brake ON:  [ ] Pass [ ] Fail
- 0x200 [0x00] brake OFF: [ ] Pass [ ] Fail
- 0x100 [0x01] LED ON:    [ ] Pass [ ] Fail
- 0x100 [0x00] LED OFF:   [ ] Pass [ ] Fail

SENSOR VALUES (at known conditions):
- Brake Front: ADC=____ Pressure=____
- Brake Rear:  ADC=____ Pressure=____
- Steering:    ADC=____ Angle=____
- APPS1:       ADC=____
- APPS2:       ADC=____
- RPM Left:    ____ (at ____ Hz input)
- RPM Right:   ____ (at ____ Hz input)

NOTES / ISSUES:
_______________________________________________
_______________________________________________
_______________________________________________

OVERALL: [ ] ALL PASS  [ ] PARTIAL  [ ] FAIL
==============================================================
```

---

## 17. Troubleshooting

### Green LED not blinking after flash

| Cause | Solution |
|-------|----------|
| Clock config failed → stuck in `Error_Handler()` | Check HSE crystal (16 MHz) is present and soldered. Verify PF0/PF1 connections. |
| HardFault at boot | Connect debugger, check fault address. Common cause: stack overflow. |
| `dbg_init_stage` stuck at a number < 9 | Heap exhaustion during task creation. Check `configTOTAL_HEAP_SIZE`. |
| Flash failed | Verify SWD connection (PA13, PA14, GND). Check ST-Link driver. |

### Red LED blinking

| Cause | Solution |
|-------|----------|
| `dbg_adc_queue_fail` > 0 | ADC DMA fires faster than SensorTask consumes. Possible if SensorTask is blocked. |
| Mutex timeout in CanTxTask | SensorTask holds mutex too long. Should not happen in normal conditions. |

### CAN messages not appearing on analyzer

| Cause | Solution |
|-------|----------|
| No CAN transceiver | FDCAN pins output logic-level, need transceiver for differential bus |
| Wrong baud rate on analyzer | Must be exactly 1 Mbps |
| No bus termination | Add 120Ω between CANH and CANL |
| No ACK on bus (only 1 node) | Need at least 2 nodes, or use loopback mode for testing |
| `dbg_cantx_count` = 0 | CanTxTask never ran. Check task creation in `App_Init()`. |

### ADC values always 0 or 4095

| Cause | Solution |
|-------|----------|
| Sensor not connected | Floating pin reads noise |
| Sensor voltage > 3.3V | Use voltage divider. NEVER exceed 3.3V. |
| Wrong ADC channel mapping | Verify pin↔channel in `MX_ADC1_Init()` matches physical wiring |
| DMA not running | Check `dbg_adc_callback_count` — if 0, DMA never triggered |

### RPM always 0

| Cause | Solution |
|-------|----------|
| No pulses on input pin | Verify sensor wiring, check signal with oscilloscope |
| Signal voltage too low | Timer input needs clean 0→3.3V transitions |
| 500ms timeout expired | If time between pulses > 500ms, RPM auto-zeros |
| Timer not started | Check `TIM_HAL_Init()` ran (`dbg_init_stage` ≥ 4) |
| Wrong pin AF mode | Verify PA2 = AF9 (TIM15_CH1), PB0 = AF2 (TIM3_CH3) in MSP init |

### System crashes / HardFault

| Cause | Solution |
|-------|----------|
| Stack overflow in a task | Increase stack size in `*Task_Init()` or reduce local variables |
| Heap exhaustion | Check `dbg_free_heap`. Reduce heap objects or increase `configTOTAL_HEAP_SIZE` |
| ISR priority conflict | All FreeRTOS-safe ISRs must have priority ≥ 5 (numeric). Currently all are 6. |
| Null pointer (queue/mutex) | Check init stage — if < 9, one of the object creations failed |

---

**End of Testing Guide**
