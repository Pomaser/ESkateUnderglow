// add libraries
#include <FastGPIO.h>
#define APA102_USE_FAST_GPIO
#include <APA102.h>

// set pins
const int PIN1_CLOCK         = 9;
const int PIN1_DATA          = 10;
const int PIN2_CLOCK         = 11;
const int PIN2_DATA          = 12;
const int PIN_CHANGE_PATTERN = 8;

// create strips
APA102<PIN1_DATA, PIN1_CLOCK> ledStrip1;
APA102<PIN2_DATA, PIN2_CLOCK> ledStrip2;

// set LED count for strip
#define LED_COUNT 22

// create buffers for both strips
rgb_color buffer1[LED_COUNT];
rgb_color buffer2[LED_COUNT];

// create constants
const int PATTERN_MAX          = 6;   // highest pattern index (added meteor)
const int DEFAULT_EFFECT       = 4;   // effect started when pin 8 is HIGH at boot (4 = breath)
const int GLOBAL_BRIGHTNESS    = 10;
const int TIMOUT_BUTTON_CYCLES = 500;

// Hall sensor — speed measurement (Meepo Mini 2 ER, LingYi hub motor)
// Wiring: motor JST pin 1 (+5V) → 5V, pin 2 (GND) → GND, pin 3 (Hall A) → D2
const uint8_t  HALL_PIN     = 2;     // D2 = INT0
const uint8_t  POLE_PAIRS   = 14;    // motor pole pairs (Meepo 540W = 14)
const float    WHEEL_MM     = 100.0; // wheel diameter [mm]
const uint32_t CALC_PERIOD  = 500;   // speed recalculation interval [ms]
const uint32_t ZERO_THRESH  = 3000;  // no pulse for this long → 0 km/h [ms]
const float    WHEEL_CIRC_M = (WHEEL_MM * PI) / 1000.0;

// create variables
byte step = 0;
int positionIdx = 0;
int meteorPos = 0;
int loopCnt = 0;
int flashCounter = 0;
int transitionCounter = 0;
int intensity = 0;
int buttonState = 0;
int lastButtonState = 1;
int patternIdx = 0;
int buttonCounter = 0;
int buttonTimer = 0;
bool loopPulse = false;
bool positiveDirection = true;
bool flipFlop = false;
bool intensityUp = false;
bool changePattern;

// Hall sensor ISR state — volatile because modified inside interrupt
volatile uint32_t hallPulseCount  = 0;
volatile uint32_t hallLastPulseUs = 0;


// initialization stuff
void setup()
{
  pinMode(PIN_CHANGE_PATTERN, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  delay(40);

  // clear both strips
  fillStrip(1, 0, LED_COUNT, 0, 0, 0);
  fillStrip(2, 0, LED_COUNT, 0, 0, 0);
  ledStrip1.write(buffer1, LED_COUNT, GLOBAL_BRIGHTNESS);
  ledStrip2.write(buffer2, LED_COUNT, GLOBAL_BRIGHTNESS);

  // Startup indicator: run meteor for 2 s so the user knows the system booted
  uint32_t startupEnd = millis() + 2000;
  while (millis() < startupEnd) {
    meteorEffect(8);
    ledStrip1.write(buffer1, LED_COUNT, GLOBAL_BRIGHTNESS);
    ledStrip2.write(buffer2, LED_COUNT, GLOBAL_BRIGHTNESS);
    delay(5);
  }

  // clear after startup animation
  fillStrip(1, 0, LED_COUNT, 0, 0, 0);
  fillStrip(2, 0, LED_COUNT, 0, 0, 0);
  ledStrip1.write(buffer1, LED_COUNT, GLOBAL_BRIGHTNESS);
  ledStrip2.write(buffer2, LED_COUNT, GLOBAL_BRIGHTNESS);

  // read button state for first start
  lastButtonState = digitalRead(PIN_CHANGE_PATTERN);

  // start DEFAULT_EFFECT if button is held at boot
  if (digitalRead(PIN_CHANGE_PATTERN) == HIGH) {
    patternIdx = DEFAULT_EFFECT;
  }

  setupHallSensor();
}


// main loop
void loop()
{
  // read change pattern input
  buttonState = digitalRead(PIN_CHANGE_PATTERN);

  // copy state to output LED
  if ( buttonState == 1 ) {
    digitalWrite(LED_BUILTIN, HIGH);
  } else {
    digitalWrite(LED_BUILTIN, LOW);
  }

  // compare the buttonState to its previous state
  if (buttonState != lastButtonState) {

    // rising edge of button
    if (buttonState == HIGH) {
      buttonCounter++;
    }
    // avoid bouncing
    delay(50);
  }

  // save the current state as the last state, for next time through the loop
  lastButtonState = buttonState;

  // count cycles
  if ( buttonCounter > 0 ) {
    buttonTimer++;
  }

  // reset cycle counter
  if ( buttonTimer > TIMOUT_BUTTON_CYCLES ) {
    buttonCounter = 0;
    buttonTimer = 0;
  }

  // change pattern signal after some presses
  if (( buttonTimer < TIMOUT_BUTTON_CYCLES ) && ( buttonCounter > 1 )) {
    changePattern = true;
    buttonCounter = 0;
    buttonTimer = 0;
  }

  // change pattern index
  if (changePattern == true ) {

    // erase old effect
    fillStrip(1, 0, LED_COUNT, 0, 0, 0);
    fillStrip(2, 0, LED_COUNT, 0, 0, 0);
    changePattern = false;

    // change pattern
    if (patternIdx < PATTERN_MAX ) {
      patternIdx++;
    } else {
      patternIdx = 0;
    }
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
      strobe(20, rgb_color(200, 200, 200));
      break;
    case 6:
      meteorEffect(8);
      break;
    default:
      patternIdx = 0;
      break;
  }

  // write to outputs
  ledStrip1.write(buffer1, LED_COUNT, GLOBAL_BRIGHTNESS);
  ledStrip2.write(buffer2, LED_COUNT, GLOBAL_BRIGHTNESS);

  float speed = getSpeed(); // TODO: use speed to drive effects
  (void)speed;
}


void strobe(int loopDelay, rgb_color color) {

  loopCounter(loopDelay);

  if (loopPulse == false) {
    if (flipFlop == false) {
      fillStrip(1, 0, LED_COUNT, color.red, color.green, color.blue);
      fillStrip(2, 0, LED_COUNT, color.red, color.green, color.blue);
    } else {
      fillStrip(1, 0, LED_COUNT, 0, 0, 0);
      fillStrip(2, 0, LED_COUNT, 0, 0, 0);
    }
  }
}


// rainbow effect
void rainbow(int loopDelay, byte colorLen) {

  loopCounter(loopDelay);

  if (loopPulse == true) {

    switch (step) {
      case 0: buffer1[0] = rgb_color(148, 0, 211); break; // violet
      case 1: buffer1[0] = rgb_color(75, 0, 130);  break; // indigo
      case 2: buffer1[0] = rgb_color(0, 0, 255);   break; // blue
      case 3: buffer1[0] = rgb_color(0, 255, 0);   break; // green
      case 4: buffer1[0] = rgb_color(255, 255, 0); break; // yellow
      case 5: buffer1[0] = rgb_color(255, 127, 0); break; // orange
      case 6: buffer1[0] = rgb_color(255, 0, 0);   break; // red
      default: step = 0;                            break;
    }

    colorLen = constrain(colorLen, 1, LED_COUNT - 1);
    if ( transitionCounter < colorLen ) {
      transitionCounter++;
    } else {
      step++;
      transitionCounter = 0;
    }

    for (int idx = LED_COUNT - 1; idx > 0; idx--) {
      buffer1[idx] = buffer1[idx - 1];
    }

    memcpy(buffer2, buffer1, sizeof(buffer2));
  }
}


// Knight rider scanner effect
void knightScanner(int beamLen, int loopMax)
{
  rgb_color beamShape[beamLen];
  const int BEAM_OFFSET = beamLen - 1;

  float beamSpread = 255.0 / (beamLen - 1);
  int beamStep = 0;
  for (int idx = 0; idx < beamLen; idx++) {
    if ( idx < (beamLen - 1) ) {
      beamStep = 255 - (idx * beamSpread);
    } else {
      beamStep = 0;
    }
    beamShape[idx].red   = beamStep;
    beamShape[idx].green = 0;
    beamShape[idx].blue  = 0;
  }

  loopCounter(loopMax);

  if ( (positionIdx < LED_COUNT + BEAM_OFFSET - 1) && (positiveDirection == true) ) {
    if (loopPulse == true) { positionIdx++; }
  } else {
    positiveDirection = false;
  }

  if ( (positionIdx > 0 - BEAM_OFFSET) && (positiveDirection == false) ) {
    if (loopPulse == true) { positionIdx--; }
  } else {
    positiveDirection = true;
  }

  if ( positiveDirection == true ) {
    for (int idx = 0; idx < beamLen; idx++) {
      int copyPos = positionIdx - idx;
      if ( (copyPos < LED_COUNT) && (copyPos >= 0) ) {
        buffer1[copyPos] = beamShape[idx];
        buffer2[copyPos] = beamShape[idx];
      }
    }
  } else {
    for (int idx = 0; idx < beamLen; idx++) {
      int copyPos = positionIdx + idx;
      if ( (copyPos < LED_COUNT) && (copyPos >= 0) ) {
        buffer1[copyPos] = beamShape[idx];
        buffer2[copyPos] = beamShape[idx];
      }
    }
  }
}


// police lights
void policeLights(int loopMax) {

  const int FLASH_COUNT      = 8;
  const int TRANSITION_COUNT = 10;

  loopCounter(loopMax);

  switch (step) {

  case 0:
    if ( flipFlop == true ) {
      fillStrip(1, 0, LED_COUNT, 255, 0, 0);
    } else {
      fillStrip(1, 0, LED_COUNT, 0, 0, 0);
    }
    if ( loopPulse == true ) { flashCounter++; }
    if ( flashCounter == FLASH_COUNT ) { flashCounter = 0; step = 1; }
    break;

  case 1:
    if ( flipFlop == true ) {
      fillStrip(2, 0, LED_COUNT, 0, 0, 255);
    } else {
      fillStrip(2, 0, LED_COUNT, 0, 0, 0);
    }
    if ( loopPulse == true ) { flashCounter++; }
    if ( flashCounter == FLASH_COUNT ) {
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

  case 2:
    if ( flipFlop == true ) {
      fillStrip(1, 0, 11, 0, 0, 255);
      fillStrip(2, 0, 11, 0, 0, 255);
    } else {
      fillStrip(1, 0, LED_COUNT, 0, 0, 0);
      fillStrip(2, 0, LED_COUNT, 0, 0, 0);
    }
    if ( loopPulse == true ) { flashCounter++; }
    if ( flashCounter == FLASH_COUNT ) { flashCounter = 0; step = 3; }
    break;

  case 3:
    if ( flipFlop == true ) {
      fillStrip(1, 12, LED_COUNT, 255, 0, 0);
      fillStrip(2, 12, LED_COUNT, 255, 0, 0);
    } else {
      fillStrip(1, 0, LED_COUNT, 0, 0, 0);
      fillStrip(2, 0, LED_COUNT, 0, 0, 0);
    }
    if ( loopPulse == true ) { flashCounter++; }
    if ( flashCounter == FLASH_COUNT ) {
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


void breathEffect(int loopMax) {

  const int MIN_INTENSITY = 0;
  const int MAX_INTENSITY = 80;

  loopCounter(loopMax);

  if ( intensityUp == false ) {
    if ( intensity < MAX_INTENSITY ) {
      if ( loopPulse == true ) {
        fillStrip(1, 0, LED_COUNT, intensity, 0, intensity);
        fillStrip(2, 0, LED_COUNT, intensity, 0, intensity);
        intensity = intensity + 2;
      }
    } else {
      intensityUp = not intensityUp;
    }
  } else {
    if ( intensity > MIN_INTENSITY ) {
      if ( loopPulse == true ) {
        fillStrip(1, 0, LED_COUNT, intensity, 0, intensity);
        fillStrip(2, 0, LED_COUNT, intensity, 0, intensity);
        intensity = intensity - 2;
      }
    } else {
      intensityUp = not intensityUp;
    }
  }
}


// Blue-white shooting star: bright head with a fading trail.
void meteorEffect(int loopMax) {

  const int     METEOR_SIZE  = 5;   // number of LEDs in the meteor body
  const uint8_t TRAIL_DECAY  = 192; // scale factor per tick: higher = longer trail

  loopCounter(loopMax);

  if (loopPulse == true) {

    // Decay all pixels to extend the fading trail
    for (int i = 0; i < LED_COUNT; i++) {
      buffer1[i].red   = (uint8_t)((buffer1[i].red   * TRAIL_DECAY) >> 8);
      buffer1[i].green = (uint8_t)((buffer1[i].green * TRAIL_DECAY) >> 8);
      buffer1[i].blue  = (uint8_t)((buffer1[i].blue  * TRAIL_DECAY) >> 8);
      buffer2[i].red   = (uint8_t)((buffer2[i].red   * TRAIL_DECAY) >> 8);
      buffer2[i].green = (uint8_t)((buffer2[i].green * TRAIL_DECAY) >> 8);
      buffer2[i].blue  = (uint8_t)((buffer2[i].blue  * TRAIL_DECAY) >> 8);
    }

    // Draw meteor: full brightness at head, dimming toward tail
    for (int j = 0; j < METEOR_SIZE; j++) {
      int idx = meteorPos - j;
      if (idx >= 0 && idx < LED_COUNT) {
        uint8_t bright = 255 - (j * (255 / METEOR_SIZE));
        buffer1[idx] = rgb_color(bright, bright, 255);
        buffer2[idx] = rgb_color(bright, bright, 255);
      }
    }

    if (++meteorPos >= LED_COUNT + METEOR_SIZE) {
      meteorPos = 0;
    }
  }
}


// ISR — called on every rising edge of the Hall signal.
void onHallPulse()
{
  hallPulseCount++;
  hallLastPulseUs = micros();
}


// Attach interrupt to HALL_PIN. Call once from setup().
void setupHallSensor()
{
  pinMode(HALL_PIN, INPUT); // ESC provides pull-up via sensor connector
  attachInterrupt(digitalPinToInterrupt(HALL_PIN), onHallPulse, RISING);
}


// Returns current speed in km/h. Recalculates every CALC_PERIOD ms.
// Returns 0.0 if no pulse received within ZERO_THRESH ms.
float getSpeed()
{
  static uint32_t prevCalcTime   = 0;
  static uint32_t prevPulseSnap  = 0;
  static float    cachedSpeedKmh = 0.0;

  uint32_t now = millis();
  if (now - prevCalcTime < CALC_PERIOD) {
    return cachedSpeedKmh;
  }

  noInterrupts();
  uint32_t pulses      = hallPulseCount - prevPulseSnap;
  prevPulseSnap        = hallPulseCount;
  uint32_t lastPulseUs = hallLastPulseUs;
  interrupts();

  float dt_s   = (now - prevCalcTime) / 1000.0;
  prevCalcTime = now;

  bool stopped = ((now - (lastPulseUs / 1000)) > ZERO_THRESH);

  if (stopped || pulses == 0) {
    cachedSpeedKmh = 0.0;
  } else {
    float rps      = (float)pulses / ((float)POLE_PAIRS * dt_s);
    cachedSpeedKmh = rps * WHEEL_CIRC_M * 3.6;
  }

  return cachedSpeedKmh;
}


// fill strip with single color
void fillStrip(byte stripNo, int from, int to, uint8_t r, uint8_t g, uint8_t b)
{
  for (int i = from; i < to; i++) {
    if ( stripNo == 1 ) {
      buffer1[i].red   = r;
      buffer1[i].green = g;
      buffer1[i].blue  = b;
    }
    if ( stripNo == 2 ) {
      buffer2[i].red   = r;
      buffer2[i].green = g;
      buffer2[i].blue  = b;
    }
  }
}


// loop counter
void loopCounter(int loopMax) {
  if ( loopCnt < loopMax ) {
    loopCnt++;
    loopPulse = false;
  } else {
    loopCnt   = 0;
    loopPulse = true;
    flipFlop  = not flipFlop;
  }
}
