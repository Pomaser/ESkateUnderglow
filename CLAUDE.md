# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Arduino firmware for APA102 LED underglow on an electric skateboard. Uses the **APA102** + **FastGPIO** libraries (not FastLED). Two independent 22-LED strips with 6 switchable patterns via a physical button.

## Hardware

- **MCU:** Arduino-compatible board
- **Strip 1:** DATA pin 10, CLOCK pin 9
- **Strip 2:** DATA pin 12, CLOCK pin 11
- **Button:** pin 8 (pattern change, active HIGH)
- **LEDs per strip:** 22× APA102
- **Wiring:** `doc/LightControlWiring.svg`
- **Switch datasheet:** `doc/Switch_MR5110f.pdf`

## Build & Flash

Uses **arduino-cli** (installed at `/usr/local/bin/arduino-cli`). Board: `arduino:avr:nano`.

```bash
# Compile
arduino-cli compile -b arduino:avr:nano eskate_glow/

# Upload (check port with: arduino-cli board list)
arduino-cli upload -p /dev/ttyUSB0 -b arduino:avr:nano eskate_glow/

# Compile + upload in one step
arduino-cli compile -b arduino:avr:nano -p /dev/ttyUSB0 --upload eskate_glow/
```

If port is not accessible without sudo: `sudo usermod -aG dialout $USER` (requires re-login).

## Code Architecture

All firmware is in `eskate_glow/eskate_glow.ino`.

### Pattern switching

The button on pin 8 uses a two-press detection within `TIMOUT_BUTTON_CYCLES` (500) loop iterations to advance `patternIdx` (0–`PATTERN_MAX`=5). Patterns cycle: 0 → 1 → … → 5 → 0.

### Patterns (`patternIdx`)

| Index | Effect | Key params |
|-------|--------|------------|
| 0 | Off | — |
| 1 | `knightScanner` | beam length 14, loopMax 15 |
| 2 | `policeLights` | loopMax 25 |
| 3 | `rainbow` | loopMax 15, colorLen 10 |
| 4 | `breathEffect` | loopMax 25 |
| 5 | `strobe` | loopMax 20, white 200,200,200 |

### Timing mechanism (`loopCounter`)

All effects use `loopCounter(loopMax)` — a simple loop-count divider. Every `loopMax` iterations it sets `loopPulse=true` and toggles `flipFlop`. Effects use `loopPulse` as a tick to advance animation state.

### Shared state variables

Effects share global state: `step`, `positionIdx`, `loopCnt`, `flashCounter`, `transitionCounter`, `intensity`, `flipFlop`, `positiveDirection`. These are reset implicitly when `fillStrip` clears the buffers on pattern change.

### Output

Both strips write from `buffer1[LED_COUNT]` / `buffer2[LED_COUNT]` each loop iteration:
```cpp
ledStrip1.write(buffer1, LED_COUNT, GLOBAL_BRIGHTNESS);  // GLOBAL_BRIGHTNESS = 10
ledStrip2.write(buffer2, LED_COUNT, GLOBAL_BRIGHTNESS);
```

## Repository Layout

```
eskate_glow/   Arduino sketch
cad/           FreeCAD part files (*.FCStd); backups (*.FCBak) are git-ignored
doc/           Wiring SVG and component datasheet
```
