#include <FastLED.h>

// APA102 strip pins (SPI: data + clock per strip)
const int PIN1_CLOCK = 9;
const int PIN1_DATA = 10;
const int PIN2_CLOCK = 11;
const int PIN2_DATA = 12;

// Pattern change button (active HIGH, external pull-down)
const int PIN_CHANGE_PATTERN = 8;

#define LED_COUNT 22
const uint8_t GLOBAL_BRIGHTNESS = 10;

CRGB leds1[LED_COUNT];
CRGB leds2[LED_COUNT];

const int PATTERN_MAX = 6;          // highest pattern index
const int TIMOUT_BUTTON_CYCLES = 500; // window for double-press detection

// Shared timing state — written by loopCounter(), read by all effects
bool loopPulse = false; // true for one iteration when the tick fires
bool flipFlop  = false; // toggles each tick

int patternIdx = 0;


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
  delay(40); // let external pull-down settle

  fillStrip(1, 0, LED_COUNT, 0, 0, 0);
  fillStrip(2, 0, LED_COUNT, 0, 0, 0);
  FastLED.show();
}


void loop()
{
  if (readButton()) {
    // clear strip before switching so old pixels don't bleed into new effect
    fillStrip(1, 0, LED_COUNT, 0, 0, 0);
    fillStrip(2, 0, LED_COUNT, 0, 0, 0);
    patternIdx = (patternIdx < PATTERN_MAX) ? patternIdx + 1 : 0;
  }

  switch (patternIdx) {
    case 0: fillStrip(1, 0, LED_COUNT, 0, 0, 0);
            fillStrip(2, 0, LED_COUNT, 0, 0, 0);  break;
    case 1: knightScanner(14, 15);                break;
    case 2: policeLights(25);                     break;
    case 3: rainbow(15, 10);                      break;
    case 4: breathEffect(25);                     break;
    case 5: strobe(20, CRGB(200, 200, 200));      break;
    case 6: meteorEffect(8);                      break;
    default: patternIdx = 0;                      break;
  }

  FastLED.show();
}


// Returns true when a double-press is detected within TIMOUT_BUTTON_CYCLES iterations.
// Mirrors button state onto the built-in LED for visual feedback.
bool readButton()
{
  static int lastButtonState = 1;
  static int buttonCounter = 0;
  static int buttonTimer = 0;

  int buttonState = digitalRead(PIN_CHANGE_PATTERN);

  digitalWrite(LED_BUILTIN, buttonState == 1 ? HIGH : LOW);

  if (buttonState != lastButtonState) {
    if (buttonState == HIGH) {
      buttonCounter++; // count rising edges
    }
    delay(50); // debounce
  }
  lastButtonState = buttonState;

  if (buttonCounter > 0) {
    buttonTimer++;
  }

  // timeout — discard incomplete press sequence
  if (buttonTimer > TIMOUT_BUTTON_CYCLES) {
    buttonCounter = 0;
    buttonTimer = 0;
  }

  if ((buttonTimer < TIMOUT_BUTTON_CYCLES) && (buttonCounter > 1)) {
    buttonCounter = 0;
    buttonTimer = 0;
    return true;
  }

  return false;
}


// Rapid on/off flash at the given speed.
void strobe(int loopDelay, CRGB color) {

  loopCounter(loopDelay);

  // flipFlop toggles each tick — use it directly for on/off
  if (flipFlop == false) {
    fillStrip(1, 0, LED_COUNT, color.r, color.g, color.b);
    fillStrip(2, 0, LED_COUNT, color.r, color.g, color.b);
  } else {
    fillStrip(1, 0, LED_COUNT, 0, 0, 0);
    fillStrip(2, 0, LED_COUNT, 0, 0, 0);
  }
}


// Scrolling rainbow: inserts a new color at index 0 each tick and shifts the rest down.
void rainbow(int loopDelay, byte colorLen) {

  static byte step = 0;
  static int transitionCounter = 0;

  loopCounter(loopDelay);

  if (loopPulse == true) {

    switch (step) {
      case 0: leds1[0] = CRGB(148, 0, 211); break; // violet
      case 1: leds1[0] = CRGB(75, 0, 130);  break; // indigo
      case 2: leds1[0] = CRGB(0, 0, 255);   break; // blue
      case 3: leds1[0] = CRGB(0, 255, 0);   break; // green
      case 4: leds1[0] = CRGB(255, 255, 0); break; // yellow
      case 5: leds1[0] = CRGB(255, 127, 0); break; // orange
      case 6: leds1[0] = CRGB(255, 0, 0);   break; // red
      default: step = 0; break;
    }

    // hold each color for colorLen ticks before advancing
    colorLen = constrain(1, colorLen, LED_COUNT-1);
    if (transitionCounter < colorLen) {
      transitionCounter++;
    } else {
      step++;
      transitionCounter = 0;
    }

    // shift strip towards the end
    for (int idx=LED_COUNT-1; idx > 0; idx--) {
      leds1[idx] = leds1[idx-1];
    }

    memcpy(leds2, leds1, sizeof(leds2));
  }
}


// Red scanning beam that bounces end-to-end, Knight Rider style.
void knightScanner(int beamLen, int loopMax)
{
  static int positionIdx = 0;
  static bool positiveDirection = true;

  CRGB beamShape[beamLen];
  const int BEAM_OFFSET = beamLen - 1;

  // build brightness gradient: full at head, zero at tail
  float beamSpread = 255.0 / (beamLen - 1);
  int beamStep = 0;
  for (int idx=0; idx < beamLen; idx++) {
    beamStep = (idx < beamLen - 1) ? 255 - (idx * beamSpread) : 0;
    beamShape[idx] = CRGB(beamStep, 0, 0);
  }

  loopCounter(loopMax);

  // advance position each tick, reverse at strip boundaries
  if ((positionIdx < LED_COUNT + BEAM_OFFSET - 1) && (positiveDirection == true)) {
    if (loopPulse == true) {positionIdx++;}
  } else {
    positiveDirection = false;
  }

  if ((positionIdx > 0 - BEAM_OFFSET) && (positiveDirection == false)) {
    if (loopPulse == true) {positionIdx--;}
  } else {
    positiveDirection = true;
  }

  // paint beam onto LED arrays (skip out-of-range indices)
  if (positiveDirection == true) {
    for (int idx=0; idx < beamLen; idx++) {
      int copyPos = positionIdx - idx;
      if ((copyPos < LED_COUNT) && (copyPos >= 0)) {
        leds1[copyPos] = beamShape[idx];
        leds2[copyPos] = beamShape[idx];
      }
    }
  } else {
    for (int idx=0; idx < beamLen; idx++) {
      int copyPos = positionIdx + idx;
      if ((copyPos < LED_COUNT) && (copyPos >= 0)) {
        leds1[copyPos] = beamShape[idx];
        leds2[copyPos] = beamShape[idx];
      }
    }
  }
}


// Alternating red/blue police-style flash sequence.
// Sequence: red solo → blue solo (repeated TRANSITION_COUNT times) → blue split → red split → repeat.
void policeLights(int loopMax) {

  static byte step = 0;
  static int flashCounter = 0;
  static int transitionCounter = 0;

  const int FLASH_COUNT = 8;       // flashes per phase before advancing
  const int TRANSITION_COUNT = 10; // red/blue alternation cycles before split phase

  loopCounter(loopMax);

  switch (step) {

  case 0: // flash strip 1 RED
    fillStrip(1, 0, LED_COUNT, flipFlop ? 255 : 0, 0, 0);
    if (loopPulse == true) {flashCounter++;}
    if (flashCounter == FLASH_COUNT) {flashCounter = 0; step = 1;}
    break;

  case 1: // flash strip 2 BLUE
    fillStrip(2, 0, LED_COUNT, 0, 0, flipFlop ? 255 : 0);
    if (loopPulse == true) {flashCounter++;}
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
    if (flipFlop == true) {
      fillStrip(1, 0, 11, 0, 0, 255);
      fillStrip(2, 0, 11, 0, 0, 255);
    } else {
      fillStrip(1, 0, LED_COUNT, 0, 0, 0);
      fillStrip(2, 0, LED_COUNT, 0, 0, 0);
    }
    if (loopPulse == true) {flashCounter++;}
    if (flashCounter == FLASH_COUNT) {flashCounter = 0; step = 3;}
    break;

  case 3: // flash right half of both strips RED
    if (flipFlop == true) {
      fillStrip(1, 12, LED_COUNT, 255, 0, 0);
      fillStrip(2, 12, LED_COUNT, 255, 0, 0);
    } else {
      fillStrip(1, 0, LED_COUNT, 0, 0, 0);
      fillStrip(2, 0, LED_COUNT, 0, 0, 0);
    }
    if (loopPulse == true) {flashCounter++;}
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
void breathEffect(int loopMax) {

  const uint8_t MIN_INTENSITY = 10;
  const uint8_t MAX_INTENSITY = 80;

  static uint8_t theta = 0;

  loopCounter(loopMax);

  if (loopPulse == true) {
    theta++;
    int intensity = map(sin8(theta), 0, 255, MIN_INTENSITY, MAX_INTENSITY);
    fillStrip(1, 0, LED_COUNT, intensity, 0, intensity);
    fillStrip(2, 0, LED_COUNT, intensity, 0, intensity);
  }
}


// Blue-white shooting star: bright head with a fading trail that decays via nscale8.
void meteorEffect(int loopMax) {

  const int     METEOR_SIZE  = 5;
  const uint8_t TRAIL_DECAY  = 192; // 0-255: higher = longer trail

  static int pos = 0;

  loopCounter(loopMax);

  if (loopPulse == true) {

    // decay all pixels to extend the fading trail
    for (int i = 0; i < LED_COUNT; i++) {
      leds1[i].nscale8(TRAIL_DECAY);
      leds2[i].nscale8(TRAIL_DECAY);
    }

    // draw meteor: full brightness at head, dimming toward tail
    for (int j = 0; j < METEOR_SIZE; j++) {
      int idx = pos - j;
      if (idx >= 0 && idx < LED_COUNT) {
        uint8_t bright = 255 - (j * (255 / METEOR_SIZE));
        leds1[idx] = CRGB(bright, bright, 255);
        leds2[idx] = CRGB(bright, bright, 255);
      }
    }

    if (++pos >= LED_COUNT + METEOR_SIZE) {
      pos = 0;
    }
  }
}


// Fill LEDs [from, to) on the given strip (1 or 2) with a solid color.
void fillStrip(byte stripNo, int from, int to, uint8_t r, uint8_t g, uint8_t b)
{
  for (int i=from; i < to; i++) {
    if (stripNo == 1) leds1[i] = CRGB(r, g, b);
    if (stripNo == 2) leds2[i] = CRGB(r, g, b);
  }
}


// Tick divider: sets loopPulse=true and toggles flipFlop every loopMax calls.
void loopCounter(int loopMax) {

  static int loopCnt = 0;

  if (loopCnt < loopMax) {
    loopCnt++;
    loopPulse = false;
  } else {
    loopCnt = 0;
    loopPulse = true;
    flipFlop = !flipFlop;
  }
}
