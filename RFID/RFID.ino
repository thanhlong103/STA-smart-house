#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN 5    // Slave Select pin
#define RST_PIN 22  // Reset pin

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance

// Predefined RFID card UID (Modify this with your card's UID)
byte authorizedUID[] = {0x29, 0x48, 0x12, 0x05}; // Replace with your actual card UID

void setup() {
    Serial.begin(115200);
    SPI.begin();         // Initialize SPI bus
    mfrc522.PCD_Init();  // Initialize RFID module
    Serial.println("Place your RFID card near the scanner...");
}

void loop() {
    // Look for new cards
    if (!mfrc522.PICC_IsNewCardPresent()) {
        return;
    }

    // Select one of the cards
    if (!mfrc522.PICC_ReadCardSerial()) {
        return;
    }

    Serial.print("Card UID: ");
    for (byte i = 0; i < mfrc522.uid.size; i++) {
        Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
        Serial.print(mfrc522.uid.uidByte[i], HEX);
    }
    Serial.println();

    // Check if the scanned UID matches the predefined UID
    if (isAuthorized(mfrc522.uid.uidByte, mfrc522.uid.size)) {
        Serial.println("Access Granted!");
    } else {
        Serial.println("Access Denied!");
    }

    // Halt the PICC (Power-down the RFID card)
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
}

// Function to check if the UID matches the authorized card
bool isAuthorized(byte *uid, byte length) {
    for (byte i = 0; i < length; i++) {
        if (uid[i] != authorizedUID[i]) {
            return false;
        }
    }
    return true;
}
