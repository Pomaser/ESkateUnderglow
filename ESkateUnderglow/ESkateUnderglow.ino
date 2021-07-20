#include <FastGPIO.h>
#define APA102_USE_FAST_GPIO

#include <APA102.h>
#include <EEPROM.h>

// set pins
const uint8_t dataPin1 = 6;
const uint8_t clockPin1 = 7;
const uint8_t dataPin2 = 8;
const uint8_t clockPin2 = 9;

// create strips
APA102<dataPin1, clockPin1> ledStrip1;
APA102<dataPin2, clockPin2> ledStrip2;

#define LED_COUNT 22

rgb_color buffer1[LED_COUNT];
rgb_color buffer2[LED_COUNT];

// set the brightness (max 31)
const uint8_t GLOBAL_BRIGHTNESS = 5;
int loopIdx = 0;
int loopCnt = 0;
bool loopPulse = false;
bool positiveDirection = true;
const int BEAM_LEN = 18;
rgb_color beamShape[BEAM_LEN];

// initialization stuff
void setup()
{
  delay(40);  // give pull-ups time raise the input voltage
  Serial.begin(9600); // open the serial port at 9600 bps:
  createKittBeam();
}

// main loop
void loop()
{
  knightScanner(loopIdx, 15);
  ledStrip1.write(buffer1, LED_COUNT, GLOBAL_BRIGHTNESS);
  ledStrip2.write(buffer2, LED_COUNT, GLOBAL_BRIGHTNESS); 
}

// Knigh rider scanner effect
void knightScanner(int pos, int loopMax)
{
  const int MAX_BRIGHTNESS = 15;

  // count loops
  if ( loopCnt < loopMax ) {
    loopCnt++;
    loopPulse = false;
  } else {
    loopCnt=0;
    loopPulse = true;
  }

  // move in positive dir
  if ( (loopIdx < LED_COUNT + BEAM_LEN ) && (positiveDirection == true ) ) {
    if (loopPulse==true) {loopIdx++;}
  }
  else {
    positiveDirection = false;
  }
  
  // move in negative dir
  if ( (loopIdx > 0 - BEAM_LEN) && (positiveDirection == false ) ) {
        if (loopPulse==true) {loopIdx--;}
  }
  else {
    positiveDirection = true;
  }

  // copy beam to buffer
  if ( positiveDirection == true ) {
    for (int idx=0; idx < BEAM_LEN; idx++) {
      int copyPos = loopIdx - idx;
      
      // only LEDs in range
      if ( ( copyPos < LED_COUNT ) && ( copyPos >= 0 ) ) {
        buffer1[copyPos] = beamShape[idx];
        buffer2[copyPos] = beamShape[idx];
      }
    }    
  } else {
    for (int idx=0; idx < BEAM_LEN; idx++) {
      int copyPos = loopIdx + idx;
      
      // only LEDs in range
      if ( ( copyPos < LED_COUNT ) && ( copyPos >= 0 ) ) {
        buffer1[copyPos] = beamShape[idx];
        buffer2[copyPos] = beamShape[idx];
      }
    }    
  }

}

// create kitt beam shape
void createKittBeam() {
  float beamSpread = 255.0 / (BEAM_LEN - 1);
  int beamStep = 0;
  for (int idx=0; idx < BEAM_LEN; idx++) {
    if ( idx < (BEAM_LEN - 1) ) {
      beamStep = 255 - (idx * beamSpread);
    }
    else {
      beamStep = 0;
    }
    beamShape[idx].red = beamStep;
    beamShape[idx].blue = 0;
    beamShape[idx].green = 0;
  }
}

boolean inRange(int pos) {
  if ( (pos <= LED_COUNT ) and (pos >= 0 ) ) {
    return true;
  }
}

void fillStrip()
{
  for (int i=0; i < LED_COUNT;i++)
  {
    buffer1[i].red = 255;
    buffer1[i].green = 55;
    buffer1[i].blue = 55;

    buffer2[i].red = 255;
    buffer2[i].green = 55;
    buffer2[i].blue = 55;

  }  
}
