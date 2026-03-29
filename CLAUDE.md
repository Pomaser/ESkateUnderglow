# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Arduino firmware for APA102 LED underglow on a Meepo Mini 2 electric skateboard. Uses the **FastLED** library. Two independent 22-LED strips with 7 switchable patterns (0 = off, 1–6 = effects) via a physical button. Speed is read from the hub motor Hall sensor.

## Hardware

- **MCU:** Arduino Nano (arduino:avr:nano)
- **Strip 1:** DATA pin 10, CLOCK pin 9
- **Strip 2:** DATA pin 12, CLOCK pin 11
- **Button:** pin 8 (pattern change, active HIGH, external pull-down)
- **LEDs per strip:** 22× APA102
- **Hall sensor:** Motor JST pin 3 (Hall A) → D2 (INT0); motor provides 5V pull-up via sensor connector
- **Wiring diagram:** `doc/LightControlWiring.svg`
- **Switch datasheet:** `doc/Switch_MR5110f.pdf`

## Build & Flash

Uses **arduino-cli** (installed at `/usr/local/bin/arduino-cli`). Required library: FastLED.

```bash
# Compile for hardware
arduino-cli compile -b arduino:avr:nano eskate_glow/

# Upload (check port with: arduino-cli board list)
arduino-cli upload -p /dev/ttyUSB0 -b arduino:avr:nano eskate_glow/

# Compile for Wokwi simulation (adds -DWOKWI_SIM, output in build-sim/)
arduino-cli compile -b arduino:avr:nano --build-property "build.extra_flags=-DWOKWI_SIM" --build-path eskate_glow/build-sim eskate_glow/
```

If port is not accessible without sudo: `sudo usermod -aG dialout $USER` (requires re-login).

## Wokwi Simulation

The sketch switches LED type at compile time:
- **Hardware:** `APA102` (data + clock, BGR)
- **Simulation (`-DWOKWI_SIM`):** `WS2812B` (single data pin, GRB)

`wokwi.toml` points to `build-sim/`. `diagram.json` contains 44 individual `wokwi-neopixel` components (22 per strip) chained via DOUT→DIN.

## Code Architecture

All firmware is in `eskate_glow/eskate_glow.ino`.

### Pattern switching

`readButton()` detects a **double-press** within `TIMOUT_BUTTON_CYCLES` (200) loop iterations. On detection it advances `patternIdx` (0–`PATTERN_MAX`=6). Cycles: 0 → 1 → … → 6 → 0. Pattern 0 is "off".

### Patterns (`patternIdx`)

| Index | Effect | Key params |
|-------|--------|------------|
| 0 | Off | — |
| 1 | `knightScanner` | beamLen 14, loopMax 15 |
| 2 | `policeLights` | loopMax 25 |
| 3 | `rainbow` | loopMax 15, colorLen 10 |
| 4 | `breathEffect` | loopMax 25 |
| 5 | `strobe` | loopMax 20, CRGB(200,200,200) |
| 6 | `meteorEffect` | loopMax 8 |

### Timing mechanism (`loopCounter`)

Every effect calls `loopCounter(loopMax)` each loop iteration. Every `loopMax` calls it sets the global `loopPulse = true` and toggles `flipFlop`. Effects gate animation advances behind `if (loopPulse)`. `flipFlop` is used directly for binary on/off flashing (strobe, police).

### Per-effect state

All animation state (`step`, `pos`, `theta`, `positiveDirection`, etc.) lives as `static` locals inside each effect function. No global effect state exists — only `loopPulse`, `flipFlop`, and `patternIdx` are global.

### Output

Each frame: effects write to `leds1[LED_COUNT]` / `leds2[LED_COUNT]` (FastLED `CRGB` arrays), then `loop()` calls `FastLED.show()` once. `GLOBAL_BRIGHTNESS = 10`.

### Hall sensor speed

`setupHallSensor()` attaches `onHallPulse()` ISR to D2 (RISING). `getSpeed()` recalculates every `CALC_PERIOD` (500 ms) using atomic snapshot of `hallPulseCount` / `hallLastPulseUs`. Returns cached km/h between periods. Returns 0.0 if no pulse within `ZERO_THRESH` (3000 ms). Currently called in `loop()` but result is unused (TODO: drive effects with speed).

### Startup

On boot, `meteorEffect` runs for 2 s as a visual indicator that the system started, then LEDs clear before the main loop begins.

## Repository Layout

```
eskate_glow/   Arduino sketch + wokwi.toml + diagram.json
cad/           FreeCAD part files (*.FCStd); backups (*.FCBak) are git-ignored
doc/           Wiring SVG and component datasheet
```
