#include <Adafruit_Sensor.h>
#include <DHT.h>

// Define the DHT sensor type and the ESP32 GPIO pin
#define DHTPIN 4        // GPIO pin connected to the DHT sensor
#define DHTTYPE DHT11   // Change to DHT11 if using a DHT11 sensor

DHT dht(DHTPIN, DHTTYPE);

void setup() {
    Serial.begin(115200);
    dht.begin();
    Serial.println("DHT sensor initialized.");
}

void loop() {
    // Wait a bit between readings
    delay(2000);

    // Read humidity
    float humidity = dht.readHumidity();
    // Read temperature in Celsius
    float temperature = dht.readTemperature();

    // Check if any reading failed
    if (isnan(humidity) || isnan(temperature)) {
        Serial.println("Failed to read from DHT sensor!");
        return;
    }

    // Print values to the Serial Monitor
    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.print("%  Temperature: ");
    Serial.print(temperature);
    Serial.println("°C");
}
