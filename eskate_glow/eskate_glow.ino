#include <FastLED.h>

// set pins
const int PIN1_CLOCK = 9;
const int PIN1_DATA = 10;
const int PIN2_CLOCK = 11;
const int PIN2_DATA = 12;
const int PIN_CHANGE_PATTERN = 8;

// LED count and brightness
#define LED_COUNT 22
const uint8_t GLOBAL_BRIGHTNESS = 10;

// LED arrays
CRGB leds1[LED_COUNT];
CRGB leds2[LED_COUNT];

// constants
const int PATTERN_MAX = 6;
const int TIMOUT_BUTTON_CYCLES = 500;

// shared timing state (written by loopCounter, read by effects)
bool loopPulse = false;
bool flipFlop = false;

int patternIdx = 0;


// initialization stuff
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
  delay(40);  // give pull-ups time raise the input voltage

  fillStrip(1, 0, LED_COUNT, 0, 0, 0);
  fillStrip(2, 0, LED_COUNT, 0, 0, 0);
  FastLED.show();
}

// main loop
void loop()
{
  // read button and advance pattern on double-press
  if (readButton()) {
    fillStrip(1, 0, LED_COUNT, 0, 0, 0);
    fillStrip(2, 0, LED_COUNT, 0, 0, 0);
    patternIdx = (patternIdx < PATTERN_MAX) ? patternIdx + 1 : 0;
  }

  // switch patterns
  switch (patternIdx) {
    case 0:
      fillStrip(1, 0, LED_COUNT, 0, 0, 0);
      fillStrip(2, 0, LED_COUNT, 0, 0, 0);
      break;
    case 1:
      knightScanner(14, 15);
      break;
    case 2:
      policeLights(25);
      break;
    case 3:
      rainbow(15, 10);
      break;
    case 4:
      breathEffect(25);
      break;
    case 5:
      strobe(20, CRGB(200, 200, 200));
      break;
    case 6:
      meteorEffect(8);
      break;
    default:
      patternIdx = 0;
      break;
  }

  FastLED.show();
}


// read button, return true when pattern should change
bool readButton()
{
  static int lastButtonState = 1;
  static int buttonCounter = 0;
  static int buttonTimer = 0;

  int buttonState = digitalRead(PIN_CHANGE_PATTERN);

  // copy state to output LED
  digitalWrite(LED_BUILTIN, buttonState == 1 ? HIGH : LOW);

  // compare the buttonState to its previous state
  if (buttonState != lastButtonState) {

    // rising edge of button
    if (buttonState == HIGH) {
      buttonCounter++;
    }
    // falling edge of button
    if (buttonState == LOW) {
      ;
    }
    // avoid bouncing
    delay(50);
  }

  // save the current state as the last state, for next time through the loop
  lastButtonState = buttonState;

  // count cycles
  if (buttonCounter > 0) {
    buttonTimer++;
  }

  // reset cycle counter
  if (buttonTimer > TIMOUT_BUTTON_CYCLES) {
    buttonCounter = 0;
    buttonTimer = 0;
  }

  // signal pattern change after double-press within timeout
  if ((buttonTimer < TIMOUT_BUTTON_CYCLES) && (buttonCounter > 1)) {
    buttonCounter = 0;
    buttonTimer = 0;
    return true;
  }

  return false;
}


void strobe(int loopDelay, CRGB color) {

  // count loops
  loopCounter(loopDelay);

  // wait for timer
  if (loopPulse == false) {

    // on/off
    if (flipFlop == false) {
      fillStrip(1, 0, LED_COUNT, color.r, color.g, color.b);
      fillStrip(2, 0, LED_COUNT, color.r, color.g, color.b);
    } else {
      fillStrip(1, 0, LED_COUNT, 0, 0, 0);
      fillStrip(2, 0, LED_COUNT, 0, 0, 0);
    }
  }
}


// rainbow effect
void rainbow(int loopDelay, byte colorLen) {

  static byte step = 0;
  static int transitionCounter = 0;

  // count loops
  loopCounter(loopDelay);

  // write effect
  if (loopPulse == true) {

    // set color
    switch (step) {
      case 0: leds1[0] = CRGB(148, 0, 211); break; // violet
      case 1: leds1[0] = CRGB(75, 0, 130);  break; // indigo
      case 2: leds1[0] = CRGB(0, 0, 255);   break; // blue
      case 3: leds1[0] = CRGB(0, 255, 0);   break; // green
      case 4: leds1[0] = CRGB(255, 255, 0); break; // yellow
      case 5: leds1[0] = CRGB(255, 127, 0); break; // orange
      case 6: leds1[0] = CRGB(255, 0, 0);   break; // magenta
      default: step = 0; break;
    }

    // change color
    colorLen = constrain(1, colorLen, LED_COUNT-1);
    if (transitionCounter < colorLen) {
      transitionCounter++;
    }
    else {
      step++;
      transitionCounter = 0;
    }

    // shift whole array
    for (int idx=LED_COUNT-1; idx > 0; idx--) {
      leds1[idx] = leds1[idx-1];
    }

    // copy to second strip
    memcpy(leds2, leds1, sizeof(leds2));
  }
}


// Knight rider scanner effect
void knightScanner(int beamLen, int loopMax)
{
  static int positionIdx = 0;
  static bool positiveDirection = true;

  CRGB beamShape[beamLen];
  const int BEAM_OFFSET = beamLen - 1;

  // create beam shape
  float beamSpread = 255.0 / (beamLen - 1);
  int beamStep = 0;
  for (int idx=0; idx < beamLen; idx++) {
    if (idx < (beamLen - 1)) {
      beamStep = 255 - (idx * beamSpread);
    }
    else {
      beamStep = 0;
    }
    beamShape[idx] = CRGB(beamStep, 0, 0);
  }

  // count loops
  loopCounter(loopMax);

  // move in positive dir
  if ((positionIdx < LED_COUNT + BEAM_OFFSET - 1) && (positiveDirection == true)) {
    if (loopPulse == true) {positionIdx++;}
  }
  else {
    positiveDirection = false;
  }

  // move in negative dir
  if ((positionIdx > 0 - BEAM_OFFSET) && (positiveDirection == false)) {
    if (loopPulse == true) {positionIdx--;}
  }
  else {
    positiveDirection = true;
  }

  // copy beam to buffer
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


// police lights
void policeLights(int loopMax) {

  static byte step = 0;
  static int flashCounter = 0;
  static int transitionCounter = 0;

  const int FLASH_COUNT = 8;
  const int TRANSITION_COUNT = 10;

  loopCounter(loopMax);

  switch (step) {

  // flash RED
  case 0:
    if (flipFlop == true) {
      fillStrip(1, 0, LED_COUNT, 255, 0, 0);
    } else {
      fillStrip(1, 0, LED_COUNT, 0, 0, 0);
    }
    if (loopPulse == true) {flashCounter++;}
    if (flashCounter == FLASH_COUNT) {flashCounter = 0; step = 1;}
    break;

  // flash BLUE
  case 1:
    if (flipFlop == true) {
      fillStrip(2, 0, LED_COUNT, 0, 0, 255);
    } else {
      fillStrip(2, 0, LED_COUNT, 0, 0, 0);
    }
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

  // flash both left
  case 2:
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

  // flash both right
  case 3:
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


// Breath effect with sine
void breathEffect(int loopMax) {

  const uint8_t MIN_INTENSITY = 10;
  const uint8_t MAX_INTENSITY = 80;

  static uint8_t theta = 0;

  // count loops
  loopCounter(loopMax);

  // update only on pulse
  if (loopPulse == true) {

    // advance phase and map sine to intensity range
    theta++;
    int intensity = map(sin8(theta), 0, 255, MIN_INTENSITY, MAX_INTENSITY);

    fillStrip(1, 0, LED_COUNT, intensity, 0, intensity);
    fillStrip(2, 0, LED_COUNT, intensity, 0, intensity);
  }
}


// meteor effect — shooting star with fading trail
void meteorEffect(int loopMax) {

  const int     METEOR_SIZE  = 5;
  const uint8_t TRAIL_DECAY  = 192; // 0-255: higher = longer trail

  static int pos = 0;

  loopCounter(loopMax);

  if (loopPulse == true) {

    // fade existing LEDs to extend trail
    for (int i = 0; i < LED_COUNT; i++) {
      leds1[i].nscale8(TRAIL_DECAY);
      leds2[i].nscale8(TRAIL_DECAY);
    }

    // draw meteor head and body
    for (int j = 0; j < METEOR_SIZE; j++) {
      int idx = pos - j;
      if (idx >= 0 && idx < LED_COUNT) {
        uint8_t bright = 255 - (j * (255 / METEOR_SIZE));
        leds1[idx] = CRGB(bright, bright, 255); // blue-white
        leds2[idx] = CRGB(bright, bright, 255);
      }
    }

    // advance and reset after meteor leaves strip
    if (++pos >= LED_COUNT + METEOR_SIZE) {
      pos = 0;
    }
  }
}


// fill strip with single color
void fillStrip(byte stripNo, int from, int to, uint8_t r, uint8_t g, uint8_t b)
{
  for (int i=from; i < to; i++) {
    if (stripNo == 1) leds1[i] = CRGB(r, g, b);
    if (stripNo == 2) leds2[i] = CRGB(r, g, b);
  }
}


// loop counter
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
