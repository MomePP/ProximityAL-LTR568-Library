/*
 *   LTR-568ALS-01 Proximity Reading Example
 *
 *   Reads the proximity sensor value and prints it to the serial monitor.
 *   Default configuration: 11-bit output (0–2047), 7 LED pulses, 100 mA, 100 ms rate.
 *
 *   I2C address: 0x23 (default)
 */
#include <Proximity-ALS.h>

ProximityAL sensor;

uint16_t proximityValue;

void setup() {
    Serial.begin(115200);

    if (!sensor.begin()) {
        Serial.println("LTR-568 not found!");
        while (1);
    }
    Serial.println("LTR-568 initialised");
}

void loop() {
    proximityValue = sensor.getPSvalue();
    Serial.print("Proximity: ");
    Serial.println(proximityValue);

    delay(100);
}
