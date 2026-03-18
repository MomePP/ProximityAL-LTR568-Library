/*
 *   LTR-568ALS-01 Light Intensity Reading Example
 *
 *   Reads the ambient light sensor and calculates lux using the
 *   datasheet formula:  Lux = (0.336 × ALS_DATA) / (GAIN × INT)
 *
 *   Default configuration: gain 1×, integration time 100 ms.
 *
 *   I2C address: 0x23 (default)
 */
#include <Proximity-ALS.h>

ProximityAL sensor;

float luxValue;

void setup() {
    Serial.begin(115200);

    if (!sensor.begin()) {
        Serial.println("LTR-568 not found!");
        while (1);
    }
    Serial.println("LTR-568 initialised");
}

void loop() {
    luxValue = sensor.getLuxValue();
    Serial.print("Lux: ");
    Serial.println(luxValue, 2);

    delay(500);
}
