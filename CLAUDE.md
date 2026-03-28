# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Arduino firmware for APA102 (DotStar) LED underglow on an electric skateboard. Uses the [FastLED](https://fastled.io/) library.

## Hardware

- **MCU:** Arduino-compatible board
- **LEDs:** 2× APA102, DATA on pin 2, CLOCK on pin 4
- **Switch:** MR5110f (see `doc/Switch_MR5110f.pdf`)
- **Wiring:** `doc/LightControlWiring.svg`

## Build & Flash

Open `eskate_glow/eskate_glow.ino` in the **Arduino IDE**. Install the **FastLED** library via the Library Manager, then compile and upload normally. There is no Makefile or CLI build system.

## Code Architecture

All firmware lives in `eskate_glow/eskate_glow.ino`. Current behaviour:

- **First 5 minutes** (`millis() < PULSE_DELAY`): steady yellow at full brightness (100).
- **After 5 minutes**: smooth brightness breathing using `sin8(theta)` mapped to range 50–100, stepping every 10 ms.

The `theta` counter wraps naturally at 255 (uint8_t), producing a continuous sine cycle.

## Repository Layout

```
eskate_glow/   Arduino sketch
cad/           FreeCAD part files (*.FCStd); backups (*.FCBak) are git-ignored
doc/           Wiring SVG and component datasheet
```
