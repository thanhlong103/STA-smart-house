#include <Adafruit_NeoPixel.h>

#define LED_PIN 16     // Data pin connected to NeoPixel
#define NUM_LEDS 8    // Number of LEDs in the strip

Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
    strip.begin();
    strip.show();  // Initialize all pixels to 'off'
    Serial.begin(115200);
}

void loop() {
    rainbowCycle(10);   // Smooth rainbow effect
    colorWipe(strip.Color(255, 0, 0), 50);  // Red color wipe
    colorWipe(strip.Color(0, 255, 0), 50);  // Green color wipe
    colorWipe(strip.Color(0, 0, 255), 50);  // Blue color wipe
    theaterChase(strip.Color(255, 255, 0), 100);  // Yellow theater chase
}

// Rainbow cycle across entire strip
void rainbowCycle(int wait) {
    for (int j = 0; j < 256 * 5; j++) { // 5 cycles of all colors
        for (int i = 0; i < strip.numPixels(); i++) {
            strip.setPixelColor(i, Wheel((i * 256 / strip.numPixels() + j) & 255));
        }
        strip.show();
        delay(wait);
    }
}

// Color wipe effect (one LED at a time)
void colorWipe(uint32_t color, int wait) {
    for (int i = 0; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, color);
        strip.show();
        delay(wait);
    }
}

// Theater chase effect
void theaterChase(uint32_t color, int wait) {
    for (int j = 0; j < 10; j++) {  // 10 cycles
        for (int q = 0; q < 3; q++) {
            for (int i = 0; i < strip.numPixels(); i += 3) {
                strip.setPixelColor(i + q, color); // Turn every third pixel on
            }
            strip.show();
            delay(wait);
            for (int i = 0; i < strip.numPixels(); i += 3) {
                strip.setPixelColor(i + q, 0); // Turn every third pixel off
            }
        }
    }
}

// Helper function to generate rainbow colors
uint32_t Wheel(byte WheelPos) {
    if (WheelPos < 85) {
        return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
    } else if (WheelPos < 170) {
        WheelPos -= 85;
        return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
    } else {
        WheelPos -= 170;
        return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
    }
}
