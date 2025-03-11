#define BLYNK_TEMPLATE_ID "TMPL66YAWZ7f8"
#define BLYNK_TEMPLATE_NAME "STA SMART HOME"
#define BLYNK_AUTH_TOKEN "uYNKFZhEEreYAoJERId2O62wEKvfkUaM"

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ESP32Servo.h>
#include <Adafruit_NeoPixel.h>

char ssid[] = "Talon";
char pass[] = "abcdef12";

#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

#define pinFan 2
#define FAN_CHANNEL 3

#define SS_PIN 5
#define RST_PIN 22
MFRC522 mfrc522(SS_PIN, RST_PIN);
byte authorizedUID[] = {0x29, 0x48, 0x12, 0x05};

Servo myservo;
#define SERVO_PIN 27

#define NEOPIXEL_PIN 16
#define NUMPIXELS 8
Adafruit_NeoPixel pixels(NUMPIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
int neoMode = 0;
bool neoOverride = false;

#define LIGHT_SENSOR_PIN 34
int lightIntensity = 0;

BlynkTimer timer;

int fanSpeed = 0;           // Fan speed (0-255)
bool doorState = false;
unsigned long servoTimer = 0;
bool fanManualOverride = false;  // Tracks if Blynk manually sets fan speed
unsigned long lastFlashTime = 0; // For NeoPixel flashing
const long flashInterval = 500;  // Flash every 500ms

void setup() {
    Serial.begin(115200);

    myservo.attach(SERVO_PIN, 500, 2500);
    myservo.write(0);

    SPI.begin();
    mfrc522.PCD_Init();

    dht.begin();

    pinMode(pinFan, OUTPUT);
    ledcSetup(FAN_CHANNEL, 25000, 8); // 25kHz PWM, 8-bit resolution
    ledcAttachPin(pinFan, FAN_CHANNEL);
    ledcWrite(FAN_CHANNEL, 0); // Fan off initially

    pixels.begin();
    pixels.setBrightness(50);
    pixels.clear();
    pixels.show();

    pinMode(LIGHT_SENSOR_PIN, INPUT);

    Serial.print("Connecting to Wi-Fi: ");
    Serial.println(ssid);
    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWi-Fi connected successfully!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    Serial.println("Connecting to Blynk server...");
    Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
    Serial.println("Connected to Blynk server!");

    timer.setInterval(5000L, sendSensorData);
    timer.setInterval(2000L, checkLightSensor);
}

BLYNK_WRITE(V0) {
    Serial.println("BLYNK_WRITE(V0) triggered!");
    neoMode = param.asInt();
    neoOverride = true;
    Serial.print("NeoPixel mode set to: ");
    Serial.println(neoMode);
    updateNeoPixel();
}

BLYNK_WRITE(V1) {
    int value = param.asInt();
    doorState = value;
    if (value == 1) {
        myservo.write(90);
        Serial.println("Door opened via Blynk");
        servoTimer = millis();
    } else {
        myservo.write(0);
        Serial.println("Door closed via Blynk");
    }
}

BLYNK_WRITE(V4) {
    fanSpeed = param.asInt();
    fanManualOverride = true;  // Blynk takes priority
    ledcWrite(FAN_CHANNEL, fanSpeed);
    Serial.print("Fan speed set to: ");
    Serial.println(fanSpeed);
}

void sendSensorData() {
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();
    if (isnan(temperature) || isnan(humidity)) {
        Serial.println("Failed to read from DHT sensor!");
        return;
    }
    Blynk.virtualWrite(V2, temperature);
    Blynk.virtualWrite(V3, humidity);
    Serial.print("Temp: ");
    Serial.print(temperature);
    Serial.print(" °C, Humidity: ");
    Serial.print(humidity);
    Serial.println(" %");

    // Automatic fan control based on temperature (if not overridden by Blynk)
    if (!fanManualOverride) {
        if (temperature > 35) {
            fanSpeed = 255;  // 100% speed
            Serial.println("Temp > 35°C, Fan set to 100%");
        } else if (temperature > 25) {
            fanSpeed = 127;  // 50% speed
            Serial.println("Temp > 25°C, Fan set to 50%");
        } else {
            fanSpeed = 0;    // Off
            Serial.println("Temp <= 25°C, Fan off");
        }
        ledcWrite(FAN_CHANNEL, fanSpeed);
        Blynk.virtualWrite(V4, fanSpeed);  // Sync with Blynk app
    }

    // Automatic NeoPixel control based on temperature (if not overridden by Blynk)
    if (!neoOverride) {
        if (temperature > 35 && neoMode != 3) {
            neoMode = 3;  // Flash mode
            Blynk.virtualWrite(V0, 3);
            Serial.println("Temp > 35°C, NeoPixel set to Flash");
            updateNeoPixel();
        } else if (temperature <= 35 && neoMode == 3) {
            neoMode = 0;  // Turn off if temp drops below 35°C
            Blynk.virtualWrite(V0, 0);
            Serial.println("Temp <= 35°C, NeoPixel turned off");
            updateNeoPixel();
        }
    }
}

void checkLightSensor() {
    lightIntensity = analogRead(LIGHT_SENSOR_PIN);
    Serial.print("Light Intensity: ");
    Serial.println(lightIntensity);
    Blynk.virtualWrite(V5, map(lightIntensity, 0, 4095, 0, 255));
    if (!neoOverride && lightIntensity >= 200 && neoMode != 1) {
        neoMode = 1;
        Blynk.virtualWrite(V0, 1);
        Serial.println("Light intensity >= 200, NeoPixel set to normal by sensor");
        updateNeoPixel();
    } else if (!neoOverride && lightIntensity < 200 && neoMode != 0 && neoMode != 3) {
        neoMode = 0;
        Blynk.virtualWrite(V0, 0);
        Serial.println("Light intensity < 200, NeoPixel turned off by sensor");
        updateNeoPixel();
    }
}

void loop() {
    Blynk.run();
    timer.run();

    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
        if (isAuthorized(mfrc522.uid.uidByte, mfrc522.uid.size)) {
            Serial.println("Access Granted via RFID!");
            if (!doorState) {
                myservo.write(90);
                doorState = true;
                servoTimer = millis();
                Blynk.virtualWrite(V1, 1);
            }
        } else {
            Serial.println("Access Denied!");
            if (doorState) {
                myservo.write(0);
                doorState = false;
                Blynk.virtualWrite(V1, 0);
            }
        }
        mfrc522.PICC_HaltA();
        mfrc522.PCD_StopCrypto1();
    }

    if (doorState && millis() - servoTimer >= 5000) {
        myservo.write(0);
        doorState = false;
        Blynk.virtualWrite(V1, 0);
        Serial.println("Door auto-closed");
    }

    // Handle NeoPixel flashing when in mode 3
    if (neoMode == 3 && millis() - lastFlashTime >= flashInterval) {
        flashEffect();
        lastFlashTime = millis();
    }
}

void updateNeoPixel() {
    switch (neoMode) {
        case 0:
            pixels.clear();
            pixels.show();
            Serial.println("NeoPixel set to OFF");
            break;
        case 1:
            for (int i = 0; i < NUMPIXELS; i++) {
                pixels.setPixelColor(i, pixels.Color(255, 255, 255));
            }
            pixels.show();
            Serial.println("NeoPixel set to Normal (White)");
            break;
        case 2:
            for (int i = 0; i < NUMPIXELS; i++) {
                pixels.setPixelColor(i, pixels.Color(255, 0, 0));
            }
            pixels.show();
            Serial.println("NeoPixel set to Rainbow (Red)");
            break;
        case 3:
            // Initial state for flashing (will be toggled in flashEffect)
            for (int i = 0; i < NUMPIXELS; i++) {
                pixels.setPixelColor(i, pixels.Color(255, 0, 0));
            }
            pixels.show();
            Serial.println("NeoPixel set to Flash (Green)");
            break;
        case 4:
            pixels.clear();
            pixels.setPixelColor(0, pixels.Color(0, 0, 255));
            pixels.show();
            Serial.println("NeoPixel set to Moving (Blue at 0)");
            break;
    }
}

bool isAuthorized(byte *uid, byte length) {
    if (length != sizeof(authorizedUID) / sizeof(authorizedUID[0])) return false;
    for (byte i = 0; i < length; i++) {
        if (uid[i] != authorizedUID[i]) return false;
    }
    return true;
}

void rainbowEffect() {
    static uint16_t j = 0;
    for (int i = 0; i < NUMPIXELS; i++) {
        pixels.setPixelColor(i, Wheel((i + j) & 255));
    }
    pixels.show();
    j = (j + 1) % 256;
}

void flashEffect() {
    static bool state = false;
    state = !state;
    for (int i = 0; i < NUMPIXELS; i++) {
        pixels.setPixelColor(i, state ? pixels.Color(255, 0, 0) : pixels.Color(0, 0, 0));
    }
    pixels.show();
}

void movingEffect() {
    static int pos = 0;
    pixels.clear();
    pixels.setPixelColor(pos, pixels.Color(0, 255, 0));
    pixels.show();
    pos = (pos + 1) % NUMPIXELS;
}

uint32_t Wheel(byte WheelPos) {
    WheelPos = 255 - WheelPos;
    if (WheelPos < 85) {
        return pixels.Color(255 - WheelPos * 3, 0, WheelPos * 3);
    }
    if (WheelPos < 170) {
        WheelPos -= 85;
        return pixels.Color(0, WheelPos * 3, 255 - WheelPos * 3);
    }
    WheelPos -= 170;
    return pixels.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}