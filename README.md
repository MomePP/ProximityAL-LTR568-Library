# Proximity & Ambient Light Sensor LTR-568ALS-01 Library

Arduino library for the Lite-On **LTR-568ALS-01** integrated proximity and ambient light sensor. Drop-in replacement for the [LTR-553ALS-01 library](https://github.com/MomePP/ProximityAL-LTR553-Library).

## Specifications

- **Sensor:** LTR-568ALS-01 (Lite-On Technology Corp.)
- **Interface:** I²C (standard 100 kHz, fast 400 kHz)
- **I²C Address:** 0x23
- **ALS:** 16-bit resolution, green (visible) + IR channels, lux calculation
- **PS:** 11-bit default (16-bit optional), built-in IR LED driver
- **Supply:** VDD 3.0–3.6 V, VLED 2.5–4.35 V
- **Datasheet:** DS86-2023-0003, Rev 1.2

## Migration from LTR-553

This library uses the **same class name** (`ProximityAL`) and **same header** (`Proximity-ALS.h`). To migrate:

1. Replace the LTR-553 library with this one
2. Recompile — `begin()`, `getPSvalue()`, and `getLuxValue()` work out of the box

Key API differences:
- `getALSCH0value()` → now reads the **green** channel (alias for `getGreenValue()`)
- `getALSCH1value()` → now reads the **IR** channel (alias for `getIRvalue()`)
- `getLuxValue()` → uses the LTR-568 single-coefficient formula (0.336 × data / gain × int)
- New methods: `getGreenValue()`, `getIRvalue()`, `setIRenable()`, `setALSresolution()`, `setPS16bitMode()`, etc.

## Quick Start

```cpp
#include <Proximity-ALS.h>

ProximityAL sensor;

void setup() {
    Serial.begin(115200);
    if (!sensor.begin()) {
        Serial.println("LTR-568 not found!");
        while (1);
    }
}

void loop() {
    Serial.print("Proximity: ");
    Serial.print(sensor.getPSvalue());
    Serial.print("\tLux: ");
    Serial.println(sensor.getLuxValue(), 2);
    delay(100);
}
```

## Repository Contents

- `/src` Source files (.cpp .h)
- `/I2CUtils` I2C register utility base class
- `/examples` Example sketches
- `keywords.txt` Arduino IDE syntax highlighting
- `library.properties` Arduino/PlatformIO library metadata

## Hardware Connections

| Sensor Pin | Function | Board Pin |
|:-----------|:---------|:----------|
| VDD | Power (3.0–3.6V) | 3.3V |
| GND | Ground | GND |
| SDA | I²C Data | SDA |
| SCL | I²C Clock | SCL |
| INT | Interrupt (optional) | Any GPIO |
| LEDA | LED Anode | VLED (2.5–4.35V) |
| LEDK ↔ LDR | LED Cathode ↔ Driver | Connect together |

> **Note:** VDD and VLED must use separate supplies.

## Datasheet Notes

This library follows the LTR-568ALS-01 datasheet (DS86-2023-0003 Rev 1.2) as the authoritative source. Where the datasheet and the [AliOS-Things reference C driver](https://github.com/1021256354/AliOS-Things/blob/master/device/sensor/drv/drv_als_liteon_ltr568.c) disagree, discrepancies are documented with `REF_IMPL_DIFF` comments in the source code.

Known conflicts:
- **ALS_STATUS bit layout:** datasheet has gain at bits[5:3], ref impl at bits[6:3]
- **PART_ID:** datasheet says 0x1C, ref impl expects 0x00
- **ALS_INT_TIME upper nibble:** field table says 0xA0, pseudo-code says 0x00 — library follows pseudo-code
- **PS_VREHL:** field table says 0x06, pseudo-code says 0x05 — library uses 0x06

## License

See [LICENSE](LICENSE) file.
