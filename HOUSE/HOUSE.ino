#define BLYNK_TEMPLATE_ID "TMPL6zBZ_hCXq"
#define BLYNK_TEMPLATE_NAME "ESP32 STA SMARTHOME"
#define BLYNK_AUTH_TOKEN "x3Wnzb9BriKehJMzhzgapnXidjfV1OeQ"

// Blynk and Wi-Fi
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

char ssid[] = "Talon";      // Replace with your Wi-Fi SSID
char pass[] = "abcdef12";  // Replace with your Wi-Fi password

// Other libraries
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ESP32Servo.h>

// DHT11
#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Fan
#define pinFan 2
#define FAN_CHANNEL 3  // PWM channel for fan

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
    ledcSetup(FAN_CHANNEL, 25000, 8); // PWM channel 0, 25kHz, 8-bit resolution
    ledcAttachPin(pinFan, FAN_CHANNEL);
    ledcWrite(FAN_CHANNEL, 0); // Fan off initially

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
    timer.setInterval(2000L, sendSensorData);
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
void sendSensorData() {
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();

    if (isnan(temperature) || isnan(humidity)) {
        Serial.println("Failed to read from DHT sensor!");
        return;
    }

    Blynk.virtualWrite(V2, temperature);  // Send temperature to V2
    Blynk.virtualWrite(V3, humidity);     // Send humidity to V3
    Serial.print("Temp: ");
    Serial.print(temperature);
    Serial.print(" °C, Humidity: ");
    Serial.print(humidity);
    Serial.println(" %");
}

void loop() {
    Blynk.run();  // Handle Blynk communication
    timer.run();  // Run the timer for sensor updates

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
}

bool isAuthorized(byte *uid, byte length) {
    if (length != sizeof(authorizedUID) / sizeof(authorizedUID[0])) return false;
    for (byte i = 0; i < length; i++) {
        if (uid[i] != authorizedUID[i]) return false;
    }
    return true;
}