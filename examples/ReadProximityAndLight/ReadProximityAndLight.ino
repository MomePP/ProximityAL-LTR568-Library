/*
 *   LTR-568ALS-01 Combined Proximity & Light Reading Example
 *
 *   Reads both proximity and ambient light values, plus the raw
 *   green and IR channel data.
 *
 *   I2C address: 0x23 (default)
 */
#include <Proximity-ALS.h>

ProximityAL sensor;

uint16_t proximityValue;
float luxValue;

void setup() {
    Serial.begin(115200);

    if (!sensor.begin()) {
        Serial.println("LTR-568 not found!");
        while (1);
    }

    Serial.print("Part ID: 0x");
    Serial.println(sensor.getPartNumberID(), HEX);
    Serial.print("Manufacturer ID: 0x");
    Serial.println(sensor.getManufacturerID(), HEX);
    Serial.println("LTR-568 initialised");
}

void loop() {
    proximityValue = sensor.getPSvalue();
    luxValue = sensor.getLuxValue();

    Serial.print("Proximity: ");
    Serial.print(proximityValue);

    Serial.print("\tLux: ");
    Serial.print(luxValue, 2);

    Serial.print("\tGreen: ");
    Serial.print(sensor.getGreenValue());

    Serial.print("\tIR: ");
    Serial.print(sensor.getIRvalue());

    Serial.println();
    delay(100);
}
