#define BLYNK_TEMPLATE_ID "xxxxx"
#define BLYNK_TEMPLATE_NAME "xxxxx"
#define BLYNK_AUTH_TOKEN "xxxxx"

// Blynk and Wi-Fi
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

char ssid[] = "xxxxx";      // Replace with your Wi-Fi SSID
char pass[] = "xxxxx";  // Replace with your Wi-Fi password

// Other libraries
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ESP32Servo.h>
#include <Adafruit_NeoPixel.h> 

// DHT11
#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Fan
#define pinFan 2
#define FAN_CHANNEL 3  // PWM channel for fan
#define FAN_FREQ 25000  // Fan PWM frequency in Hz
#define FAN_RESOLUTION 8

// RFID
#define SS_PIN 5
#define RST_PIN 22
MFRC522 mfrc522(SS_PIN, RST_PIN);
byte authorizedUID[] = {0x29, 0x48, 0x12, 0x05};

// Servo
Servo myservo;
#define SERVO_PIN 27

// Timer for periodic updates
BlynkTimer timer;

// Variables
int fanSpeed = 0;           // Fan speed from Blynk slider (V4)
bool doorState = false;     // Door state (false = closed, true = open)
unsigned long servoTimer = 0; // For non-blocking servo delay
bool blynkControl = false;  // Tracks if Blynk is controlling the door


int i = 0; 

//neopixel 
#define LED_PIN 16 
#define NUM_LEDS 8
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

//Light 
#define LDR_PIN 33

void setup() {
    Serial.begin(115200);

    // Servo setup
    myservo.attach(SERVO_PIN, 500, 2500); // Pin, min pulse (500µs), max pulse (2500µs)
    myservo.write(0);

    // RFID setup
    SPI.begin();
    mfrc522.PCD_Init();

    // DHT setup
    dht.begin();

    // Fan setup
    pinMode(pinFan, OUTPUT);
    // ledcSetup(FAN_CHANNEL, FAN_FREQ, FAN_RESOLUTION);  // Set up the PWM properties (freq and resolution)
    ledcAttach(pinFan, FAN_FREQ, FAN_RESOLUTION); 
    ledcWrite(FAN_CHANNEL, 0); // Fan off initially

    //Light
    pinMode(LDR_PIN, INPUT_PULLUP);

    //Neonlight
    strip.begin();
    strip.show();
    strip.clear();

    // Wi-Fi and Blynk setup with status prints
    Serial.print("Connecting to Wi-Fi: ");
    Serial.println(ssid);
    WiFi.begin(ssid, pass);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("Wi-Fi connected successfully!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    Serial.println("Connecting to Blynk server...");
    Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
    Serial.println("Connected to Blynk server!");

    // Timer to send sensor data every 2 seconds
    timer.setInterval(2000L, sendTempData);
    timer.setInterval(2000L, sendLightData);
}

// Blynk virtual pin handlers
BLYNK_WRITE(V1) {  // Door control (0 = close, 1 = open)
    int value = param.asInt();
    doorState = value;
    blynkControl = true;  // Blynk is now in control
    if (value == 1) {
        myservo.write(90);
        Serial.println("Door opened via Blynk");
    } else {
        myservo.write(0);
        Serial.println("Door closed via Blynk");
        blynkControl = false;  // Release Blynk control when closed
    }
}

BLYNK_WRITE(V4) {  // Fan speed control (slider 0–255)
    fanSpeed = param.asInt();
    ledcWrite(FAN_CHANNEL, fanSpeed);
    Serial.print("Fan speed set to: ");
    Serial.println(fanSpeed);
}

// Send sensor data to Blynk
float sendTempData() {
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();

    if (isnan(temperature) || isnan(humidity)) {
        Serial.println("Failed to read from DHT sensor!");
        return -1;
    }

    // Blynk.virtualWrite(V2, temperature);  // Send temperature to V2
    // Blynk.virtualWrite(V3, humidity);     // Send humidity to V3
    Serial.print("Temp: ");
    Serial.print(temperature);
    Serial.print(" °C, Humidity: ");
    Serial.print(humidity);
    Serial.println(" %");

    return temperature; 
}


BLYNK_WRITE(V0) { // Neon light mode selection
    int mode = param.asInt(); 
    switch (mode) {
        case 0:
            turnOffNeonLights();
            Serial.println("Lights OFF");
            break;
        case 1:
            colorWipe(strip.Color(0, 0, 255), 20); 
            Serial.println("Lights in NORMAL mode");
            break;
        case 2:
            rainbowCycle(10);
            Serial.println("Lights in RAINBOW mode");
            break;
        case 3:
            strobeEffect(strip.Color(255, 255, 255), 20);
            Serial.println("Lights in FLASH mode");
            break;
        case 4:
            theaterChase(strip.Color(255, 255, 0), 20);
            Serial.println("Lights in MOVING mode");
            break;
        default:
            Serial.println("Invalid mode selected!");
    }
}


//Send light sensor data to Blynk
int sendLightData() {
    int lightValue = analogRead(LDR_PIN);

    if (isnan(lightValue)) {
        Serial.println("Failed to read from Light sensor!");
        return -1;
    }

    Blynk.virtualWrite(V5, lightValue);  // Send light values to V4
    Serial.print("Light intensity: ");
    Serial.print(lightValue);

    return lightValue; 
}

void loop() {
    Blynk.run();  // Handle Blynk communication
    timer.run();  // Run the timer for sensor updates
    int lightValue = sendLightData();
    Serial.print("read Light intensity: ");
    Serial.print(lightValue);
    // RFID logic (only if Blynk isn’t controlling)
    if (!blynkControl && mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
        if (isAuthorized(mfrc522.uid.uidByte, mfrc522.uid.size)) {
            Serial.println("Access Granted via RFID!");
            myservo.write(90);
            doorState = true;
            servoTimer = millis();  // Start timer for auto-close
            Blynk.virtualWrite(V1, 1);  // Update Blynk app
        } else {
            Serial.println("Access Denied!");
            if (!doorState) {  // Only close if Blynk hasn’t set it open
                myservo.write(0);
            }
        }
        mfrc522.PICC_HaltA();
        mfrc522.PCD_StopCrypto1();
    }

    // Auto-close door after 5000ms (only if Blynk isn’t keeping it open)
    if (doorState && !blynkControl && millis() - servoTimer >= 5000) {
        myservo.write(0);
        doorState = false;
        Blynk.virtualWrite(V1, 0);  // Update Blynk app
        Serial.println("Door auto-closed via RFID");
    }

     float temperature = sendTempData();

     if (temperature != -1) {
        if (!blynkControl) {
            if (temperature > 40) {
                ledcWrite(FAN_CHANNEL, 255);  
            } else if (temperature > 32) {
                ledcWrite(FAN_CHANNEL, 127);  
            } else {
                ledcWrite(FAN_CHANNEL, 0);   
            }
        }
    }

    
    
        if (!blynkControl) {
            if (lightValue > 4000) {
                rainbowCycle(10);
                colorWipe(strip.Color(255, 0, 0), 50); 
                colorWipe(strip.Color(0, 255, 0), 50);
                colorWipe(strip.Color(0, 0, 255), 50);  
                theaterChase(strip.Color(255, 255, 0), 100);  
            } 
            else{
              strip.clear();  
              strip.show();  
            }
        }
         
}

bool isAuthorized(byte *uid, byte length) {
    if (length != sizeof(authorizedUID) / sizeof(authorizedUID[0])) return false;
    for (byte i = 0; i < length; i++) {
        if (uid[i] != authorizedUID[i]) return false;
    }
    return true;
}


//neon light patterns:

void rainbowCycle(int wait) {
    for (int j = 0; j < 256 * 5; j++) { // 5 cycles of all colors
        for (int i = 0; i < strip.numPixels(); i++) {
            strip.setPixelColor(i, Wheel((i * 256 / strip.numPixels() + j) & 255));
        }
        strip.show();
        delay(wait);
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

void strobeEffect(uint32_t color, int delayTime) { //flashing lights
    for (int i = 0; i < 10; i++) { // Flash 10 times
        strip.fill(color);
        strip.show();
        delay(delayTime);
        strip.clear();
        strip.show();
        delay(delayTime);
    }
}



void turnOffNeonLights() {
    strip.clear();  
    strip.show();   
}



// #define LDR_PIN 25  // Connect LDR to GPIO34 (Any ADC pin)

// void setup() {
//     Serial.begin(115200);
//     Serial.println("Light Sensor Initialized...");
//     pinMode(LDR_PIN, INPUT);
// }

// void loop() {
//     int lightValue = analogRead(LDR_PIN);  // Read analog value from LDR
//     Serial.print("Light Intensity: ");
//     Serial.println(lightValue);  // Print the value - 4000 is dark, 100 is light

//     delay(1000);  // Wait 1 second before next reading
// }

