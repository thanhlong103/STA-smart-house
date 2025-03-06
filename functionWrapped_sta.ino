#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <SPI.h>
#include <ESP32Servo.h>
#include <Adafruit_NeoPixel.h> 
#include <MFRC522.h>

//Temp sensor 
#define DHTPIN 4        
#define DHTTYPE DHT11   
DHT dht(DHTPIN, DHTTYPE);

//Fan
#define pinFan 2 

//neopixel 
#define LED_PIN 16 
#define NUM_LEDS 8
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

//Light 
#define LDR_PIN 25


//RFID card 
#define SS_PIN 5    // Slave Select pin
#define RST_PIN 22  // Reset pin
MFRC522 mfrc522(SS_PIN, RST_PIN); 
byte authorizedUID[] = {0x29, 0x48, 0x12, 0x05};

//Motor 
Servo myservo; 

void setup() {
    Serial.begin(115200);

    //Servo
    myservo.attach(27);
    myservo.write(0);

    //RFID
    SPI.begin();
    mfrc522.PCD_Init();
    dht.begin();

    //Fan
    pinMode(pinFan, OUTPUT);
    analogWrite(pinFan, 0);

    //Light
    pinMode(LDR_PIN, INPUT);

    //Neonlight
    strip.begin();
    strip.show();

}

void loop() {
   tempControl();
   rfidAccess();
   lightControl();
}

//Function for RFID card
bool isAuthorized(byte *uid, byte length) {
    if (length != (sizeof(authorizedUID)/sizeof(authorizedUID[0]))){
      return false;
    }
    for (byte i = 0; i < length; i++) {
        if (uid[i] != authorizedUID[i]) {
            return false;
        }
    }
    return true;
}

void rfidAccess() {
    if (!mfrc522.PICC_IsNewCardPresent()) return;
    if (!mfrc522.PICC_ReadCardSerial()) return;

    if (isAuthorized(mfrc522.uid.uidByte, mfrc522.uid.size)) {
        Serial.println("Access Granted!");
        myservo.write(90);
    } else {
        Serial.println("Access Denied!");
        myservo.write(0);
    }

    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
}



//Function for Light Control 
void lightControl(){
  int lightValue = analogRead(LDR_PIN);
  if (lightValue >= 4000){
    rainbowCycle(10);
    colorWipe(strip.Color(255, 0, 0), 50); 
    colorWipe(strip.Color(0, 255, 0), 50);
    colorWipe(strip.Color(0, 0, 255), 50);  
    theaterChase(strip.Color(255, 255, 0), 100);
  }
}


//Functions for Neonlight:
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


//Function for temp sensor:
void tempControl() {
    float temperature = dht.readTemperature();
    if (temperature > 40) {
        analogWrite(pinFan, 255);
    } else if (temperature > 32) {
        analogWrite(pinFan, 127);
    } else {
        analogWrite(pinFan, 0);
    }
}




