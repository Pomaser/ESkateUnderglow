#include <FastLED.h>

// Number of LEDs and APA102 pins
#define NUM_LEDS 2
#define DATA_PIN 2
#define CLOCK_PIN 4

// LED array
CRGB leds[NUM_LEDS];

// Brightness range
const uint8_t BRIGHT_MIN = 50;
const uint8_t BRIGHT_MAX = 100;

// Sine wave phase (0–255 equals 0–360 degrees)
uint8_t theta = 0;

// Delay before pulsing starts (5 minutes in milliseconds)
const unsigned long PULSE_DELAY = 5UL * 60UL * 1000UL;

void setup() {
  Serial.begin(9600);
  delay(200);

  // Initialize APA102 LEDs
  FastLED.addLeds<APA102, DATA_PIN, CLOCK_PIN, BGR>(leds, NUM_LEDS);

  // Set initial LED color
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Yellow;
  }

  // Set full brightness at startup
  FastLED.setBrightness(BRIGHT_MAX);

  // Send data to LEDs
  FastLED.show();
}

void loop() {
  if (millis() < PULSE_DELAY) {
    // First 5 minutes: steady full brightness
    FastLED.setBrightness(BRIGHT_MAX);
    FastLED.show();
    delay(10);
  } else {
    // After 5 minutes: start pulsing
    theta++;

    // Generate smooth brightness using sine wave
    uint8_t brightness = map(
      sin8(theta),
      0, 255,
      BRIGHT_MIN, BRIGHT_MAX
    );

    // Apply brightness and update LEDs
    FastLED.setBrightness(brightness);
    FastLED.show();

    // Control animation speed
    delay(10);
  }
}