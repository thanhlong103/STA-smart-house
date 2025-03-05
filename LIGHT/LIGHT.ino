#define LDR_PIN 25  // Connect LDR to GPIO34 (Any ADC pin)

void setup() {
    Serial.begin(115200);
    Serial.println("Light Sensor Initialized...");
    pinMode(LDR_PIN, INPUT);
}

void loop() {
    int lightValue = analogRead(LDR_PIN);  // Read analog value from LDR
    Serial.print("Light Intensity: ");
    Serial.println(lightValue);  // Print the value - 4000 is dark, 100 is light

    delay(1000);  // Wait 1 second before next reading
}
