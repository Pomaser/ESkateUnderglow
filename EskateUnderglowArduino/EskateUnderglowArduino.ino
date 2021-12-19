// add libraries
#include <FastGPIO.h>
#define APA102_USE_FAST_GPIO
#include <APA102.h>
#include <EEPROM.h>

// set pins
const int PIN1_CLOCK = 9;
const int PIN1_DATA = 10;
const int PIN2_CLOCK = 11;
const int PIN2_DATA = 12;
const int PIN_CHANGE_PATTERN = 8;

// create strips
APA102<PIN1_DATA, PIN1_CLOCK> ledStrip1;
APA102<PIN2_DATA, PIN2_CLOCK> ledStrip2;

// set LED count for strip
#define LED_COUNT 22

// create buffers for both strips
rgb_color buffer1[LED_COUNT];
rgb_color buffer2[LED_COUNT];

// init variables
const uint8_t GLOBAL_BRIGHTNESS = 10;
int positionIdx = 0;
int loopCnt = 0;
bool loopPulse = false;
bool positiveDirection = true;
int first = false;
bool flipFlop = false;
byte step = 0;
int flashCounter = 0;
int transitionCounter = 0;
int intensity = 0;
bool intensityUp = false;
int buttonState = 0;
int lastButtonState = 1;
bool changePattern;
int patternIdx = 0;
const int PATTERN_MAX = 5;

// initialization stuff
void setup()
{
  pinMode(PIN_CHANGE_PATTERN, INPUT);
  pinMode(PIN_CHANGE_PATTERN, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);
  fillStrip(1, 0, 22, 0, 0, 0);
  fillStrip(2, 0, 22, 0, 0, 0);
  first = false;
  delay(40);  // give pull-ups time raise the input voltage
}

// main loop
void loop()
{

  // read change pattern input
  buttonState = digitalRead(PIN_CHANGE_PATTERN);
  
  // compare the buttonState to its previous state
  if (buttonState != lastButtonState) {
   
    // rising edge of button
    if (buttonState == HIGH) {
      changePattern = true;
      digitalWrite(LED_BUILTIN, HIGH);
    }

    if (buttonState == LOW ) {
      changePattern = true;
      digitalWrite(LED_BUILTIN, LOW);
    }
    // avoid bouncing
    delay(50);
  }
  
  // save the current state as the last state, for next time through the loop
  lastButtonState = buttonState;
    
  // change pattern index
  if (changePattern == true ) {
    changePattern = false;
    if (patternIdx < PATTERN_MAX ) {
      patternIdx++;
    } else {
      patternIdx = 0;
    }
  }

  // switch patterns
  switch (patternIdx) {
    case 0:
      fillStrip(1, 0, 22, 0, 0, 0);
      fillStrip(2, 0, 22, 0, 0, 0);
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
      strobe(20, rgb_color(200,200,200));  
      break;
    default:
      patternIdx = 0;
      break;
  }

  // write to outputs
  ledStrip1.write(buffer1, LED_COUNT, GLOBAL_BRIGHTNESS);
  ledStrip2.write(buffer2, LED_COUNT, GLOBAL_BRIGHTNESS); 
}


void strobe(int loopDelay, rgb_color color) {

  // count loops
  loopCounter(loopDelay);

  // wait for timer
  if (loopPulse==false) {

    // on/off
    if (flipFlop==false) {
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
  
  // count loops
  loopCounter(loopDelay);

  // write effect
  if (loopPulse == true) {

    // set color
    switch (step) {
      case 0:
        buffer1[0] = rgb_color(148, 0, 211); // violet
      break;
      case 1:
        buffer1[0] = rgb_color(75, 0, 130);  // indigo
      break;
      case 2:
        buffer1[0] = rgb_color(0, 0, 255);   // blue
      break;
      case 3:
        buffer1[0] = rgb_color(0, 255, 0); // green
      break;  
      case 4:
        buffer1[0] = rgb_color(255, 255, 0); // yellow
      break;
      case 5:
        buffer1[0] = rgb_color(255, 127, 0); // orange
      break;    
      case 6:
        buffer1[0] = rgb_color(255, 0, 0); // magenta
      break;    
      default:
        step=0;
      break;
    }

    // change color
    colorLen = constrain(1, colorLen, LED_COUNT-1);
    if ( transitionCounter < colorLen ) {
      transitionCounter++;
    }
    else {
      step++;
      transitionCounter = 0;
    }
    
    // shift whole array
    for (int idx=LED_COUNT-1; idx > 0; idx--) {
      buffer1[idx] = buffer1[idx-1];
    }

    // copy to second buffer
    memcpy(buffer2, buffer1, sizeof(buffer2));
  }
}

// Knight rider scanner effect
void knightScanner(int beamLen, int loopMax)
{

  // create variables
  rgb_color beamShape[beamLen];
  const int BEAM_OFFSET = beamLen - 1;

 
  // create beam shape
  float beamSpread = 255.0 / (beamLen - 1);
  int beamStep = 0;
  for (int idx=0; idx < beamLen; idx++) {
    if ( idx < (beamLen - 1) ) {
      beamStep = 255 - (idx * beamSpread);
    }
    else {
      beamStep = 0;
    }
    beamShape[idx].red = beamStep;
    beamShape[idx].blue = 0;
    beamShape[idx].green = 0;
  }

  // count loops
  loopCounter(loopMax);

  // move in positive dir
  if ( (positionIdx < LED_COUNT + BEAM_OFFSET - 1 ) && (positiveDirection == true ) ) {
    if (loopPulse==true) {positionIdx++;}
  }
  else {
    positiveDirection = false;
  }
  
  // move in negative dir
  if ( (positionIdx > 0 - BEAM_OFFSET ) && (positiveDirection == false ) ) {
        if (loopPulse==true) {positionIdx--;}
  }
  else {
    positiveDirection = true;
  }

  // copy beam to buffer
  if ( positiveDirection == true ) {
    for (int idx=0; idx < beamLen; idx++) {
      int copyPos = positionIdx - idx;
      
      // only LEDs in range
      if ( ( copyPos < LED_COUNT ) && ( copyPos >= 0 ) ) {
        buffer1[copyPos] = beamShape[idx];
        buffer2[copyPos] = beamShape[idx];
      }
    }    
  } else {
    for (int idx=0; idx < beamLen; idx++) {
      int copyPos = positionIdx + idx;
      
      // only LEDs in range
      if ( ( copyPos < LED_COUNT ) && ( copyPos >= 0 ) ) {
        buffer1[copyPos] = beamShape[idx];
        buffer2[copyPos] = beamShape[idx];
      }
    }    
  }

}

// police lights 
void policeLights(int loopMax) {

  // init
  const int FLASH_COUNT = 8;
  const int TRANSITION_COUNT = 10;
  
  loopCounter(loopMax);

  // switch effects
  switch (step) {

  // flash RED
  case 0:

    // one flash
    if ( flipFlop == true ) {
      fillStrip(1, 0, LED_COUNT, 255, 0, 0);
    } else {
      fillStrip(1, 0, LED_COUNT, 0, 0, 0);
    }

    // count flashes
    if ( loopPulse == true ) {flashCounter++;}

    // next step
    if ( flashCounter == FLASH_COUNT ) {flashCounter = 0; step = 1;}
    
    break;

  // flash BLUE
  case 1:

    // one flash
    if ( flipFlop == true ) {
      fillStrip(2, 0, LED_COUNT, 0, 0, 255);
    } else {
      fillStrip(2, 0, LED_COUNT, 0, 0, 0);
    }

    // count flashes
    if ( loopPulse == true ) {flashCounter++;}

    // next step
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

  // flash both
  case 2:

    // one flash
    if ( flipFlop == true ) {
      fillStrip(1, 0, 11, 0, 0, 255);
      fillStrip(2, 0, 11, 0, 0, 255);
    } else {
      fillStrip(1, 0, LED_COUNT, 0, 0, 0);
      fillStrip(2, 0, LED_COUNT, 0, 0, 0);
    }

    // count flashes
    if ( loopPulse == true ) {flashCounter++;}

    // next step
    if ( flashCounter == FLASH_COUNT ) {flashCounter = 0; step = 3;}
    break;

  // flash both
  case 3:

    // one flash
    if ( flipFlop == true ) {
      fillStrip(1, 12, LED_COUNT, 255, 0, 0);
      fillStrip(2, 12, LED_COUNT, 255, 0, 0);
    } else {
      fillStrip(1, 0, LED_COUNT, 0, 0, 0);
      fillStrip(2, 0, LED_COUNT, 0, 0, 0);
    }

    // count flashes
    if ( loopPulse == true ) {flashCounter++;}

    // next step
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
    // statements
    break;
  }
  
}


void breathEffect(int loopMax) {

  const int MIN_INTESNSITY = 10;
  const int MAX_INTESNSITY = 80;

  // count loops
  loopCounter(loopMax);


  // increase intensity
  if ( intensityUp == false ) {
    if ( intensity < MAX_INTESNSITY ) {
      if ( loopPulse == true ) {
        fillStrip(1, 0, LED_COUNT, intensity, 0, intensity);
        fillStrip(2, 0, LED_COUNT, intensity, 0, intensity);
        intensity=intensity + 2;
      }
    } else {
      intensityUp = not intensityUp;
    }
  } else {
    if ( intensity > MIN_INTESNSITY ) {
      if ( loopPulse == true ) {
        fillStrip(1, 0, LED_COUNT, intensity, 0, intensity);
        fillStrip(2, 0, LED_COUNT, intensity, 0, intensity);
        intensity=intensity - 2;
      }
    } else {
      intensityUp = not intensityUp;
    } 
  }
  
    
}


// fill strip with color
void fillStrip(byte stripNo, int from, int to, uint8_t r, uint8_t g, uint8_t b)
{
  for (int i=from; i < to;i++) {

    if ( stripNo == 1 ) {
      buffer1[i].red = r;
      buffer1[i].green = g;
      buffer1[i].blue = b;
    }

    if ( stripNo == 2 ) {
      buffer2[i].red = r;
      buffer2[i].green = g;
      buffer2[i].blue = b;
    }

  }  
}

// loop counter
void loopCounter(int loopMax) {
  
  // count loops
  if ( loopCnt < loopMax ) {
    loopCnt++;
    loopPulse = false;
  } else {
    loopCnt=0;
    loopPulse = true;
    flipFlop = not flipFlop;
  }
  
}
