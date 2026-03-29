#include <FastLED.h>

// APA102 strip pins (SPI: data + clock per strip)
const int PIN1_CLOCK = 9;
const int PIN1_DATA  = 10;
const int PIN2_CLOCK = 11;
const int PIN2_DATA  = 12;

// Pattern change button (active HIGH, external pull-down)
const int PIN_CHANGE_PATTERN = 8;

#define LED_COUNT 22                   // LEDs per strip
const uint8_t GLOBAL_BRIGHTNESS = 10; // master brightness 0-255

CRGB leds1[LED_COUNT]; // strip 1: data pin 10, clock pin 9
CRGB leds2[LED_COUNT]; // strip 2: data pin 12, clock pin 11

const int PATTERN_MAX           = 6;   // highest pattern index
const int TIMOUT_BUTTON_CYCLES  = 200; // window for double-press detection
const int STARTUP_DELAY_MS      = 40;  // let external pull-down settle
const int DEBOUNCE_DELAY_MS     = 50;  // button debounce duration

// Effect speed parameters (loop iterations per tick)
const int KNIGHT_SPEED      = 15;
const int POLICE_SPEED      = 25;
const int RAINBOW_SPEED     = 15;
const int BREATH_SPEED      = 25;
const int STROBE_SPEED      = 20;
const int METEOR_SPEED      = 8;

// Effect shape/color parameters
const int   KNIGHT_BEAM_LEN   = 14;
const int   RAINBOW_COLOR_LEN = 10;
const CRGB  STROBE_COLOR      = CRGB(200, 200, 200);
const int   POLICE_SPLIT      = 11; // LED index dividing left/right half in split phase

// Hall sensor — speed measurement (Meepo Mini 2 ER, LingYi hub motor)
// Wiring: motor JST pin 1 (+5V) → 5V, pin 2 (GND) → GND, pin 3 (Hall A) → D2
const uint8_t  HALL_PIN      = 2;     // D2 = INT0
const uint8_t  POLE_PAIRS    = 14;    // motor pole pairs (Meepo 540W = 14)
const float    WHEEL_MM      = 100.0; // wheel diameter [mm]
const uint32_t CALC_PERIOD   = 500;   // speed recalculation interval [ms]
const uint32_t ZERO_THRESH   = 3000;  // no pulse for this long → 0 km/h [ms]
const float    WHEEL_CIRC_M  = (WHEEL_MM * PI) / 1000.0; // wheel circumference [m]

// Shared timing state — written by loopCounter(), read by all effects
bool loopPulse = false; // true for one iteration when the tick fires
bool flipFlop  = false; // toggles each tick

int patternIdx = 0; // active effect (0 = off, 1-PATTERN_MAX = effects)

// Hall sensor ISR state — volatile because modified inside interrupt
volatile uint32_t hallPulseCount = 0; // total pulse count since boot
volatile uint32_t hallLastPulseUs = 0; // timestamp of last pulse [µs]


void setup()
{
#ifdef WOKWI_SIM
  FastLED.addLeds<WS2812B, PIN1_DATA, GRB>(leds1, LED_COUNT);
  FastLED.addLeds<WS2812B, PIN2_DATA, GRB>(leds2, LED_COUNT);
#else
  FastLED.addLeds<APA102, PIN1_DATA, PIN1_CLOCK, BGR>(leds1, LED_COUNT);
  FastLED.addLeds<APA102, PIN2_DATA, PIN2_CLOCK, BGR>(leds2, LED_COUNT);
#endif
  FastLED.setBrightness(GLOBAL_BRIGHTNESS);

  pinMode(PIN_CHANGE_PATTERN, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  delay(STARTUP_DELAY_MS);

  fillStrip(1, 0, LED_COUNT, 0, 0, 0); // start with all LEDs off
  fillStrip(2, 0, LED_COUNT, 0, 0, 0);
  FastLED.show();

  setupHallSensor();
}


void loop()
{
  if (readButton()) {
    // clear strips before switching so old pixels don't bleed into new effect
    fillStrip(1, 0, LED_COUNT, 0, 0, 0);
    fillStrip(2, 0, LED_COUNT, 0, 0, 0);
    patternIdx = (patternIdx < PATTERN_MAX) ? patternIdx + 1 : 0;
  }

  switch (patternIdx) {
    case 0: fillStrip(1, 0, LED_COUNT, 0, 0, 0);
            fillStrip(2, 0, LED_COUNT, 0, 0, 0);                    break;
    case 1: knightScanner(KNIGHT_BEAM_LEN, KNIGHT_SPEED);           break;
    case 2: policeLights(POLICE_SPEED);                             break;
    case 3: rainbow(RAINBOW_SPEED, RAINBOW_COLOR_LEN);              break;
    case 4: breathEffect(BREATH_SPEED);                             break;
    case 5: strobe(STROBE_SPEED, STROBE_COLOR);                     break;
    case 6: meteorEffect(METEOR_SPEED);                             break;
    default: patternIdx = 0;                                        break;
  }

  FastLED.show(); // send frame to hardware

  float speed = getSpeed(); // TODO: use speed to drive effects
  (void)speed;              // suppress unused variable warning
}


// Returns true when a double-press is detected within TIMOUT_BUTTON_CYCLES iterations.
// Mirrors button state onto the built-in LED for visual feedback.
bool readButton()
{
  static int lastButtonState = 1;
  static int buttonCounter   = 0;
  static int buttonTimer     = 0;

  const int buttonState = digitalRead(PIN_CHANGE_PATTERN);

  digitalWrite(LED_BUILTIN, buttonState ? HIGH : LOW);

  if (buttonState != lastButtonState) {
    if (buttonState == HIGH) {
      buttonCounter++; // count rising edges
    }
    delay(DEBOUNCE_DELAY_MS);
  }
  lastButtonState = buttonState;

  if (buttonCounter > 0) {
    buttonTimer++; // counts iterations since first press
  }

  // timeout — discard incomplete press sequence
  if (buttonTimer > TIMOUT_BUTTON_CYCLES) {
    buttonCounter = 0;
    buttonTimer   = 0;
  }

  if ((buttonTimer < TIMOUT_BUTTON_CYCLES) && (buttonCounter > 1)) {
    buttonCounter = 0;
    buttonTimer   = 0;
    return true;
  }

  return false;
}


// Rapid on/off flash at the given speed.
void strobe(const int loopDelay, const CRGB color)
{
  loopCounter(loopDelay);

  // flipFlop toggles each tick — use it directly for on/off
  if (!flipFlop) {
    fillStrip(1, 0, LED_COUNT, color.r, color.g, color.b);
    fillStrip(2, 0, LED_COUNT, color.r, color.g, color.b);
  } else {
    fillStrip(1, 0, LED_COUNT, 0, 0, 0);
    fillStrip(2, 0, LED_COUNT, 0, 0, 0);
  }
}


// Scrolling rainbow: inserts a new color at index 0 each tick and shifts the rest down.
void rainbow(const int loopDelay, const byte colorLen)
{
  static byte step             = 0;
  static int  transitionCounter = 0;

  // clamp colorLen to a valid range without modifying the input parameter
  const byte effectiveLen = constrain(colorLen, 1, LED_COUNT - 1);

  loopCounter(loopDelay);

  if (loopPulse) {

    switch (step) {
      case 0: leds1[0] = CRGB(148, 0, 211); break; // violet
      case 1: leds1[0] = CRGB(75, 0, 130);  break; // indigo
      case 2: leds1[0] = CRGB(0, 0, 255);   break; // blue
      case 3: leds1[0] = CRGB(0, 255, 0);   break; // green
      case 4: leds1[0] = CRGB(255, 255, 0); break; // yellow
      case 5: leds1[0] = CRGB(255, 127, 0); break; // orange
      case 6: leds1[0] = CRGB(255, 0, 0);   break; // red
      default: step = 0;                    break;
    }

    // hold each color for effectiveLen ticks before advancing
    if (transitionCounter < effectiveLen) {
      transitionCounter++;
    } else {
      step++;
      transitionCounter = 0;
    }

    // shift strip towards the end
    for (int idx = LED_COUNT - 1; idx > 0; idx--) {
      leds1[idx] = leds1[idx - 1];
    }

    memcpy(leds2, leds1, sizeof(leds2)); // mirror strip 1 to strip 2
  }
}


// Red scanning beam that bounces end-to-end, Knight Rider style.
void knightScanner(const int beamLen, const int loopMax)
{
  static int  positionIdx      = 0;
  static bool positiveDirection = true;

  CRGB beamShape[beamLen];
  const int BEAM_OFFSET = beamLen - 1; // extra range so beam fully enters/exits the strip

  // build brightness gradient: full at head, zero at tail
  const float beamSpread = 255.0 / (beamLen - 1);
  for (int idx = 0; idx < beamLen; idx++) {
    const int beamStep = (idx < beamLen - 1) ? 255 - (idx * beamSpread) : 0;
    beamShape[idx] = CRGB(beamStep, 0, 0);
  }

  loopCounter(loopMax);

  // advance position each tick, reverse at strip boundaries
  if ((positionIdx < LED_COUNT + BEAM_OFFSET - 1) && positiveDirection) {
    if (loopPulse) { positionIdx++; }
  } else {
    positiveDirection = false;
  }

  if ((positionIdx > -BEAM_OFFSET) && !positiveDirection) {
    if (loopPulse) { positionIdx--; }
  } else {
    positiveDirection = true;
  }

  // paint beam onto LED arrays (skip out-of-range indices)
  if (positiveDirection) {
    for (int idx = 0; idx < beamLen; idx++) {
      const int copyPos = positionIdx - idx;
      if (copyPos >= 0 && copyPos < LED_COUNT) {
        leds1[copyPos] = beamShape[idx];
        leds2[copyPos] = beamShape[idx];
      }
    }
  } else {
    for (int idx = 0; idx < beamLen; idx++) {
      const int copyPos = positionIdx + idx;
      if (copyPos >= 0 && copyPos < LED_COUNT) {
        leds1[copyPos] = beamShape[idx];
        leds2[copyPos] = beamShape[idx];
      }
    }
  }
}


// Alternating red/blue police-style flash sequence.
// Sequence: red solo → blue solo (repeated TRANSITION_COUNT times) → blue split → red split → repeat.
void policeLights(const int loopMax)
{
  static byte step             = 0;
  static int  flashCounter     = 0;
  static int  transitionCounter = 0;

  const int FLASH_COUNT      = 8;  // flashes per phase before advancing
  const int TRANSITION_COUNT = 10; // red/blue alternation cycles before split phase

  loopCounter(loopMax);

  switch (step) {

  case 0: // flash strip 1 RED
    fillStrip(1, 0, LED_COUNT, flipFlop ? 255 : 0, 0, 0);
    if (loopPulse) { flashCounter++; }
    if (flashCounter == FLASH_COUNT) { flashCounter = 0; step = 1; }
    break;

  case 1: // flash strip 2 BLUE
    fillStrip(2, 0, LED_COUNT, 0, 0, flipFlop ? 255 : 0);
    if (loopPulse) { flashCounter++; }
    if (flashCounter == FLASH_COUNT) {
      flashCounter = 0;
      if (transitionCounter == TRANSITION_COUNT) {
        transitionCounter = 0;
        step = 2;
      } else {
        transitionCounter++;
        step = 0;
      }
    }
    break;

  case 2: // flash left half of both strips BLUE
    if (flipFlop) {
      fillStrip(1, 0, POLICE_SPLIT, 0, 0, 255);
      fillStrip(2, 0, POLICE_SPLIT, 0, 0, 255);
    } else {
      fillStrip(1, 0, LED_COUNT, 0, 0, 0);
      fillStrip(2, 0, LED_COUNT, 0, 0, 0);
    }
    if (loopPulse) { flashCounter++; }
    if (flashCounter == FLASH_COUNT) { flashCounter = 0; step = 3; }
    break;

  case 3: // flash right half of both strips RED
    if (flipFlop) {
      fillStrip(1, POLICE_SPLIT + 1, LED_COUNT, 255, 0, 0);
      fillStrip(2, POLICE_SPLIT + 1, LED_COUNT, 255, 0, 0);
    } else {
      fillStrip(1, 0, LED_COUNT, 0, 0, 0);
      fillStrip(2, 0, LED_COUNT, 0, 0, 0);
    }
    if (loopPulse) { flashCounter++; }
    if (flashCounter == FLASH_COUNT) {
      flashCounter = 0;
      if (transitionCounter == TRANSITION_COUNT) {
        transitionCounter = 0;
        step = 0;
      } else {
        transitionCounter++;
        step = 2;
      }
    }
    break;

  default:
    break;
  }
}


// Purple sine-wave breathing: intensity oscillates between MIN and MAX using sin8.
void breathEffect(const int loopMax)
{
  const uint8_t MIN_INTENSITY = 10; // color component 0-255 at breath valley
  const uint8_t MAX_INTENSITY = 80; // color component 0-255 at breath peak

  static uint8_t theta = 0;

  loopCounter(loopMax);

  if (loopPulse) {
    theta++;
    const int intensity = map(sin8(theta), 0, 255, MIN_INTENSITY, MAX_INTENSITY);
    fillStrip(1, 0, LED_COUNT, intensity, 0, intensity);
    fillStrip(2, 0, LED_COUNT, intensity, 0, intensity);
  }
}


// Blue-white shooting star: bright head with a fading trail that decays via nscale8.
void meteorEffect(const int loopMax)
{
  const int     METEOR_SIZE = 5;   // number of LEDs in the meteor body
  const uint8_t TRAIL_DECAY = 192; // nscale8 factor per tick: higher = longer trail

  static int pos = 0;

  loopCounter(loopMax);

  if (loopPulse) {

    // decay all pixels to extend the fading trail
    for (int i = 0; i < LED_COUNT; i++) {
      leds1[i].nscale8(TRAIL_DECAY);
      leds2[i].nscale8(TRAIL_DECAY);
    }

    // draw meteor: full brightness at head, dimming toward tail
    for (int j = 0; j < METEOR_SIZE; j++) {
      const int idx = pos - j;
      if (idx >= 0 && idx < LED_COUNT) {
        const uint8_t bright = 255 - (j * (255 / METEOR_SIZE));
        leds1[idx] = CRGB(bright, bright, 255);
        leds2[idx] = CRGB(bright, bright, 255);
      }
    }

    if (++pos >= LED_COUNT + METEOR_SIZE) {
      pos = 0; // reset once meteor fully exits the strip
    }
  }
}


// ISR — called on every rising edge of the Hall signal.
void onHallPulse()
{
  hallPulseCount++;
  hallLastPulseUs = micros();
}


// Attach interrupt to HALL_PIN. Call once from setup() when speed reading is needed.
void setupHallSensor()
{
  pinMode(HALL_PIN, INPUT); // ESC provides pull-up via sensor connector
  attachInterrupt(digitalPinToInterrupt(HALL_PIN), onHallPulse, RISING);
}


// Returns current speed in km/h.
// Recalculates every CALC_PERIOD ms; returns cached value between periods.
// Returns 0.0 if no pulse received within ZERO_THRESH ms.
float getSpeed()
{
  static uint32_t prevCalcTime   = 0;
  static uint32_t prevPulseSnap  = 0;
  static float    cachedSpeedKmh = 0.0;

  const uint32_t now = millis();
  if (now - prevCalcTime < CALC_PERIOD) {
    return cachedSpeedKmh;
  }

  // Atomically snapshot and reset pulse counter
  noInterrupts();
  const uint32_t pulses      = hallPulseCount - prevPulseSnap;
  prevPulseSnap              = hallPulseCount;
  const uint32_t lastPulseUs = hallLastPulseUs;
  interrupts();

  const float    dt_s    = (now - prevCalcTime) / 1000.0;
  prevCalcTime           = now;

  // No pulse received within ZERO_THRESH → board is stopped
  const bool stopped = ((now - (lastPulseUs / 1000)) > ZERO_THRESH);

  if (stopped || pulses == 0) {
    cachedSpeedKmh = 0.0;
  } else {
    // rps = pulses / (POLE_PAIRS * dt)
    // speed [km/h] = rps * circumference [m] * 3.6
    const float rps = (float)pulses / ((float)POLE_PAIRS * dt_s);
    cachedSpeedKmh  = rps * WHEEL_CIRC_M * 3.6;
  }

  return cachedSpeedKmh;
}


// Fill LEDs [from, to) on the given strip (1 or 2) with a solid color.
void fillStrip(const byte stripNo, const int from, const int to, const uint8_t r, const uint8_t g, const uint8_t b)
{
  for (int i = from; i < to; i++) {
    if (stripNo == 1) leds1[i] = CRGB(r, g, b);
    if (stripNo == 2) leds2[i] = CRGB(r, g, b);
  }
}


// Tick divider: sets loopPulse=true and toggles flipFlop every loopMax calls.
void loopCounter(const int loopMax)
{
  static int loopCnt = 0;

  if (loopCnt < loopMax) {
    loopCnt++;
    loopPulse = false;
  } else {
    loopCnt   = 0;
    loopPulse = true;
    flipFlop  = !flipFlop;
  }
}
