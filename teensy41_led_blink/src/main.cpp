#include <Arduino.h>

constexpr uint8_t LED_PIN = LED_BUILTIN;
constexpr uint32_t BLINK_INTERVAL_MS = 500;

void setup() {
    pinMode(LED_PIN, OUTPUT);
    Serial.begin(115200);
    while (!Serial) {
        // Wait for serial connection (Teensy native USB)
    }
    Serial.println("Teensy LED blink + Serial print test started.");
}

void loop() {
    static uint32_t lastToggle = 0;
    const uint32_t now = millis();

    if (now - lastToggle >= BLINK_INTERVAL_MS) {
        digitalWrite(LED_PIN, !digitalRead(LED_PIN));
        Serial.print("LED state: ");
        Serial.println(digitalRead(LED_PIN) ? "ON" : "OFF");
        lastToggle = now;
    }
}