#include <FastGPIO.h>
#define APA102_USE_FAST_GPIO

#include <APA102.h>
#include <EEPROM.h>

// set pins
const uint8_t dataPin1 = 11;
const uint8_t clockPin1 = 12;
const uint8_t dataPin2 = 6;
const uint8_t clockPin2 = 7;

// create strips
APA102<dataPin1, clockPin1> ledStrip1;
APA102<dataPin2, clockPin2> ledStrip2;

#define LED_COUNT 22

rgb_color buffer1[LED_COUNT];
rgb_color buffer2[LED_COUNT];

// set the brightness (max 31)
const uint8_t globalBrightness = 5;


// initialization stuff
void setup()
{
  delay(10);  // give pull-ups time raise the input voltage
}

// main loop
void loop()
{
  for (int i=0; i < LED_COUNT;i++)
  {
    buffer1[i].red = 255;
    buffer1[i].green = 255;
    buffer1[i].blue = 255;

    buffer2[i].red = 255;
    buffer2[i].green = 255;
    buffer2[i].blue = 255;

  }
  ledStrip1.write(buffer1, LED_COUNT, globalBrightness);
  ledStrip2.write(buffer1, LED_COUNT, globalBrightness); 
}

void knightScanner()
{
  
}
