# ESkateUnderglow

Arduino firmware for addressable LED underglow on a **Meepo Mini 2 ER** electric skateboard. Two APA102 strips run 7 switchable light effects, controlled by a single push button. Speed is read from the hub motor Hall sensor.

## Hardware

| Component | Detail |
|-----------|--------|
| MCU | Arduino Nano |
| LED strips | 2× 22 APA102 (DotStar) |
| Strip 1 | DATA pin 10, CLOCK pin 9 |
| Strip 2 | DATA pin 12, CLOCK pin 11 |
| Button | Pin 8, active HIGH, external pull-down |
| Hall sensor | Motor JST Hall A → D2 (INT0) |

Wiring diagram: `doc/LightControlWiring.svg`

## Effects

Press the button **twice quickly** to advance to the next pattern.

| # | Effect | Description |
|---|--------|-------------|
| 0 | Off | All LEDs off |
| 1 | Knight Scanner | Red beam bouncing end-to-end with gradient tail |
| 2 | Police Lights | Alternating red/blue flashes, then split-half strobes |
| 3 | Rainbow | Scrolling rainbow colors shifting along the strip |
| 4 | Breath | Purple sine-wave fade in/out |
| 5 | Strobe | Rapid white flash |
| 6 | Meteor | Blue-white shooting star with fading trail |

On startup a 2-second meteor animation plays to confirm the system booted.

## Build

Requires **arduino-cli** and the **FastLED** library.

```bash
# Install FastLED (once)
arduino-cli lib install FastLED

# Compile
arduino-cli compile -b arduino:avr:nano eskate_glow/

# Compile + upload
arduino-cli compile -b arduino:avr:nano -p /dev/ttyUSB0 --upload eskate_glow/
```

Check available port: `arduino-cli board list`

If upload requires root: `sudo usermod -aG dialout $USER` (re-login required)

## Simulation (Wokwi)

The sketch supports Wokwi simulation via the `WOKWI_SIM` preprocessor flag, which switches from APA102 to WS2812B (the only NeoPixel type Wokwi supports).

```bash
# Compile for simulation
arduino-cli compile -b arduino:avr:nano --build-property "build.extra_flags=-DWOKWI_SIM" --build-path eskate_glow/build-sim eskate_glow/
```

Open `eskate_glow/diagram.json` in the Wokwi VS Code extension to run the simulation.

## Speed Reading

The firmware reads speed from the hub motor Hall sensor using an interrupt on D2. `getSpeed()` returns km/h and is called every loop iteration (recalculates every 500 ms). Speed is not yet wired to any effect — this is a planned feature.

**Calibration constants** (in `eskate_glow.ino`):
- `POLE_PAIRS = 14` — pole pairs of the Meepo 540W hub motor
- `WHEEL_MM = 100.0` — wheel diameter in mm (measure the actual PU sleeve)

## Repository Layout

```
eskate_glow/   Arduino sketch, Wokwi config (wokwi.toml, diagram.json)
cad/           FreeCAD enclosure parts (*.FCStd)
doc/           Wiring diagram (SVG) and button datasheet (PDF)
```
