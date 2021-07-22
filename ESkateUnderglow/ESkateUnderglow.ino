#include <FastGPIO.h>
#define APA102_USE_FAST_GPIO

#include <APA102.h>
#include <EEPROM.h>

// set pins
const uint8_t dataPin1 = 9;
const uint8_t clockPin1 = 10;
const uint8_t dataPin2 = 11;
const uint8_t clockPin2 = 12;

// create strips
APA102<dataPin1, clockPin1> ledStrip1;
APA102<dataPin2, clockPin2> ledStrip2;

#define LED_COUNT 22

rgb_color buffer1[LED_COUNT];
rgb_color buffer2[LED_COUNT];

// set the brightness (max 31)
const uint8_t GLOBAL_BRIGHTNESS = 15;
int positionIdx = 0;
int loopCnt = 0;
bool loopPulse = false;
bool positiveDirection = true;
int first = false;
bool flipFlop = false;
byte step = 0;
int flashCounter = 0;
int transitionCounter = 0;


// initialization stuff
void setup()
{
  delay(40);  // give pull-ups time raise the input voltage
  Serial.begin(9600); // open the serial port at 9600 bps:
  first = false;
}

// main loop
void loop()
{
  //knightScanner(14, 15);
  policeLights(25);
  ledStrip1.write(buffer1, LED_COUNT, GLOBAL_BRIGHTNESS);
  ledStrip2.write(buffer2, LED_COUNT, GLOBAL_BRIGHTNESS); 
}

// Knigh rider scanner effect
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
      if (transitionCounter == 10) {
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
      if (transitionCounter == 10) {
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


void roller(int loopMax) {
;
}

// fill strip with color
void fillStrip(byte stripNo, int from, int to, byte r, byte g, byte b)
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
