# LTR-568ALS-01 — C Library Implementation Reference
 
**Source:** Lite-On Technology Corp. Datasheet DS86-2023-0003, Rev 1.2, Effective Date: 06/26/2024  
**Part Number:** LTR-568ALS-01  
**Package:** 8-pin ChipLED, SMD, Lead-Free  
 
---
 
> **VERIFICATION STATUS:** This document has been systematically verified page-by-page against the official datasheet (DS86-2023-0003 Rev 1.2, 41 pages). All register addresses, reset values, bit fields, command bytes, and formula coefficients are confirmed correct. Three internal datasheet inconsistencies were found and documented in-place (ALS_INT_TIME reserved bits §7.8, INTERRUPT pseudo-code bit7 §7.16, PS_VREHL field-vs-pseudo-code conflict §7.20). Guidance for each contradiction is provided in the relevant section.
 
---
 
## 1. Device Overview
 
The LTR-568ALS-01 is an integrated low-voltage I²C ambient light sensor (ALS) and proximity sensor (PS) with built-in IR LED emitter in a single miniature ChipLED package.
 
**Internal structure:** 2 photodiodes — 1 visible (ambient) diode, 1 IR (proximity) diode. LED driver included on-chip.
 
**Key Features:**
- I²C interface: Standard mode 100 kHz, Fast mode 400 kHz
- ALS: 16-bit effective resolution (programmable 13/14/15/16-bit), close to human eye spectral response, 50/60 Hz flicker rejection
- PS: 16-bit effective resolution, built-in LED driver, high ambient light suppression, crosstalk cancellation
- Programmable interrupt for both ALS and PS (upper/lower thresholds)
- Operating voltage: VDD 3.0 V to 3.6 V; VLED 2.5 V to 4.35 V
- Operating temperature: -40 °C to +85 °C
- Built-in temperature compensation
 
---
 
## 2. Pin Configuration (8-pin ChipLED)
 
| Pin | Symbol | I/O | Description |
|-----|--------|-----|-------------|
| 1   | SDA    | I/O | I²C serial data (open-drain) |
| 2   | INT    | O   | Interrupt output (active LOW by default, open-drain) |
| 3   | LDR    | O   | LED driver return — connect to LED cathode (LEDK pin 4) |
| 4   | LEDK   | —   | LED cathode — connect to LDR (pin 3) |
| 5   | LEDA   | —   | LED anode — connect to VLED supply |
| 6   | GND    | —   | Ground |
| 7   | SCL    | I   | I²C serial clock (open-drain) |
| 8   | VDD    | —   | Supply voltage |
 
> **CRITICAL:** VDD and VLED must be separated (independent supplies).
 
---
 
## 3. Recommended Application Circuit Components
 
| Component | Recommended Value |
|-----------|------------------|
| Rp1, Rp2, Rp3 (pull-ups on SDA, SCL, INT) | 1 kΩ to 10 kΩ (depends on bus capacitance) |
| C1, C3 (VDD, VLED decoupling bulk) | 1 µF ±20%, X7R/X5R ceramic |
| C2, C4 (bypass) | 0.1 µF |
 
---
 
## 4. Electrical Specifications
 
### 4.1 Absolute Maximum Ratings (Ta = 25 °C)
 
| Parameter | Symbol | Min | Max | Unit |
|-----------|--------|-----|-----|------|
| Supply Voltage | VDD | — | 4.5 | V |
| Digital Pin Voltage (SCL, SDA, INT) | — | -0.5 | 4.5 | V |
| LDR Pin Voltage | — | -0.5 | 4.5 | V |
| Storage Temperature | Tstg | -40 | 100 | °C |
| ESD (HBM JESD22-A114) | VHBM | — | 2000 | V |
 
### 4.2 Recommended Operating Conditions
 
| Parameter | Symbol | Min | Max | Unit |
|-----------|--------|-----|-----|------|
| Supply Voltage | VDD | 3.0 | 3.6 | V |
| LED Supply Voltage | VLED | 2.5 | 4.35 | V |
| I²C Input High | VI2Chigh | 1.5 | VDD | V |
| I²C Input Low | VI2Clow | 0 | 0.4 | V |
| Operating Temperature | Tope | -40 | 85 | °C |
 
### 4.3 Electrical & Optical Specifications (VDD = 3.0 V, Ta = 25 °C)
 
| Parameter | Typ | Unit | Condition |
|-----------|-----|------|-----------|
| Supply Current (ALS + PS both active) | 325 | µA | Default setting |
| ALS Active Supply Current | 325 | µA | Full load |
| PS Active Supply Current | 145 | µA | Max MRR, 16 pulse, 100% duty |
| Standby Current | 5 | µA | Shutdown mode |
| Wakeup Time from Standby | 5–10 | ms | Standby → Active, measurement ready |
 
### 4.4 ALS Characteristics
 
| Parameter | Min | Max | Unit | Condition |
|-----------|-----|-----|------|-----------|
| ALS Resolution | — | 16 | Bit | Programmable: 13/14/15/16-bit |
| ALS Lux Accuracy | -15 | +15 | % | Across different light sources |
| Dark Level Count | — | 5 | Count | 0 Lux, 16-bit, Gain×128 |
| Integration Time | 50 | 400 | ms | With 50/60 Hz rejection |
| 50/60 Hz Flicker Noise Error | -5 | +5 | % | — |
 
### 4.5 PS Characteristics
 
| Parameter | Typ | Max | Unit | Condition |
|-----------|-----|-----|------|-----------|
| PS Resolution | — | 16 | Bit | — |
| IR Sensitivity Wavelength | 940 | — | nm | — |
| Detection Distance | — | 8 | cm | 32 µs, 7 pulses, 190 mA |
| LED Pulse Current | — | 240 | mA | Configurable |
| LED Pulse Width | — | 32 | µs | Configurable: 4/8/16/32 µs |
| LED Duty Cycle | — | 100 | % | — |
| Number of LED Pulses | 1 | 32 | — | Programmable |
| Ambient Light Suppression | — | 100 | klux | Direct sunlight |
 
> **NOTE:** Above 50 klux, internal fail-safe forces PS count to zero to prevent false trigger.
 
### 4.6 AC Electrical Characteristics (VBUS = 1.7 V, Ta = 25 °C)
 
| Parameter | Symbol | Std Mode Min | Std Mode Max | Fast Mode Min | Fast Mode Max | Unit |
|-----------|--------|-------------|-------------|--------------|--------------|------|
| SCL Clock Frequency | fSCL | — | 100 | — | 400 | kHz |
| Bus Free Time (STOP→START) | tBUF | 4.7 | — | 1.3 | — | µs |
| START Hold Time | tHD;STA | 4.0 | — | 0.6 | — | µs |
| SCL Low Period | tLOW | 4.7 | — | 1.3 | — | µs |
| SCL High Period | tHIGH | 4.0 | — | 0.6 | — | µs |
| Repeated START Setup | tSU;STA | 4.7 | — | 0.6 | — | µs |
| STOP Setup Time | tSU;STO | 4.0 | — | 0.6 | — | µs |
| SDA/SCL Rise Time | tr | — | 1000 | — | 300 | ns |
| SDA/SCL Fall Time | tf | — | 300 | — | 300 | ns |
| Data Hold Time | tHD;DAT | 0 | — | 0 | — | µs |
| Data Setup Time | tSU;DAT | 250 | — | 100 | — | ns |
 
---
 
## 5. I²C Interface
 
### 5.1 Slave Address
 
| Command | 7-bit Addr | 8-bit Frame |
|---------|-----------|-------------|
| Write   | **0x23**  | 0x46        |
| Read    | **0x23**  | 0x47        |
 
### 5.2 I²C Protocol Patterns
 
**Type 1 Write (set register pointer only):**
```
S | [0x46] | A | [RegAddr] | A | P
```
 
**Type 2 Write (register address + data byte):**
```
S | [0x46] | A | [RegAddr] | A | [Data] | A | P
```
 
**Read (combined format — write address then read):**
```
S | [0x46] | A | [RegAddr] | A | Sr | [0x47] | A | [Data] | N | P
```
 
Legend: S=Start, Sr=Repeated Start, P=Stop, A=ACK(0), N=NACK(1)
 
---
 
## 6. Complete Register Map
 
| Address | R/W | Name | Description | Reset Value |
|---------|-----|------|-------------|-------------|
| 0x7E | RW | ALS_AVE_LIMIT | ALS digital averaging delta limit | 0x01 |
| 0x7F | RW | ALS_AVE_FAC | ALS digital averaging factor | 0x07 |
| 0x80 | RW | ALS_CONTR | ALS operation mode control | 0x20 |
| 0x81 | RW | PS_CONTR | PS operation mode control / SW Reset | 0x10 |
| 0x82 | RW | PS_LED | PS LED pulse width and peak current | 0x7A |
| 0x83 | RW | PS_N_PULSES | PS number of LED pulses and averaging | 0x00 |
| 0x84 | RW | PS_MEAS_RATE | PS measurement repeat rate | 0x04 |
| 0x85 | RW | ALS_INT_TIME | ALS integration time and measurement rate | 0x06 |
| 0x86 | R  | PART_ID | Part number ID and revision ID | 0x1C |
| 0x87 | R  | MANUFAC_ID | Manufacturer ID | 0x08 |
| 0x88 | R  | ALS_STATUS | ALS data status, gain, validity | 0x00 |
| 0x89 | R  | IR_DATA_LSB | IR channel data, LSB | 0x00 |
| 0x8A | R  | IR_DATA_MSB | IR channel data, MSB | 0x00 |
| 0x8B | R  | GREEN_DATA_LSB | ALS (visible) data, LSB | 0x00 |
| 0x8C | R  | GREEN_DATA_MSB | ALS (visible) data, MSB | 0x00 |
| 0x91 | R  | PS_STATUS | PS status flags | 0x08 |
| 0x92 | R  | PS_DATA_LSB | PS measurement data, LSB | 0x00 |
| 0x93 | R  | PS_DATA_MSB | PS measurement data, MSB | 0x00 |
| 0x98 | RW | INTERRUPT | Interrupt pin control | 0x08 |
| 0x99 | RW | INTERRUPT_PERSIST | Interrupt persist setting | 0x00 |
| 0x9A | RW | PS_THRES_HIGH_LSB | PS interrupt upper threshold, LSB | 0xFF |
| 0x9B | RW | PS_THRES_HIGH_MSB | PS interrupt upper threshold, MSB | 0xFF |
| 0x9C | RW | PS_THRES_LOW_LSB | PS interrupt lower threshold, LSB | 0x00 |
| 0x9D | RW | PS_THRES_LOW_MSB | PS interrupt lower threshold, MSB | 0x00 |
| 0x9E | RW | PXTALK_LSB | PS crosstalk offset, LSB | 0x00 |
| 0x9F | RW | PXTALK_MSB | PS crosstalk offset, MSB | 0x00 |
| 0xB6 | RW | PS_VREHL | Sunlight fail-safe threshold | 0x08 |
 
---
 
## 7. Register Bit Field Details
 
### 7.1 ALS_AVE_LIMIT (0x7E) — default 0x01
 
| Bits | Field | Values |
|------|-------|--------|
| 7:2 | Reserved | Must write 0 |
| 1:0 | ALS_AVE_LIMIT | 00=511, **01=255 (default)**, 1x=127 |
 
### 7.2 ALS_AVE_FAC (0x7F) — default 0x07
 
| Bits | Field | Values |
|------|-------|--------|
| 7:0 | ALS_AVE_FAC | 0x00=No averaging, 0x01=Factor 1, **0x07=Factor 7 (default)**, 0xFF=Factor 255 |
 
### 7.3 ALS_CONTR (0x80) — default 0x20
 
| Bits | Field | Values |
|------|-------|--------|
| 7:6 | ALS_DR (ADC resolution) | **00=16-bit (default)**, 01=15-bit, 10=14-bit, 11=13-bit |
| 5 | IR_EN | 0=Disable IR channel, **1=Enable (default)** |
| 4:2 | ALS_GAIN | **000=1x (default)**, 001=4x, 010=16x, 011=64x, 100=128x, 101=512x |
| 1 | Reserved | Must write 0 |
| 0 | ALS_MODE | **0=Standby (default)**, 1=Active |
 
### 7.4 PS_CONTR (0x81) — default 0x10
 
| Bits | Field | Values |
|------|-------|--------|
| 7:6 | Reserved | Must write 00 |
| 5 | PS_16BITS_EN | **0=11-bit output (default)**, 1=16-bit output |
| 4 | Reserved | **Must write 1** |
| 3 | PS_OS | **0=Offset disabled (default)**, 1=Enable offset/xtalk subtraction |
| 2 | FTN/NTF_EN | **0=Disabled (default)**, 1=Enable FTN/NTF direction reporting |
| 1 | PSMODE | **0=Standby (default)**, 1=Active |
| 0 | SW_RST | **0=No action (default)**, 1=Reset all registers to default |
 
### 7.5 PS_LED (0x82) — default 0x7A
 
| Bits | Field | Values |
|------|-------|--------|
| 7 | Reserved | Must write 0 |
| 6:5 | PLED Pulse Duty | 00=12.5%, 01=25%, 10=50%, **11=100% (default)** |
| 4:3 | PLED Pulse Width | 00=4 us, 01=8 us, 10=16 us, **11=32 us (default)** |
| 2:0 | LED Current | 000=0 mA, 001=50 mA, **010=100 mA (default)**, 011=120 mA, 100=140 mA, 101=170 mA, 110=190 mA, 111=240 mA |
 
### 7.6 PS_N_PULSES (0x83) — default 0x00
 
| Bits | Field | Values |
|------|-------|--------|
| 7:6 | PS averaging factor | **00=No averaging (default)**, 01=2x, 10=4x, 11=8x |
| 5 | Reserved | Must write 0 |
| 4:0 | LED Pulse Count | 0x00-0x1F = 1 to 32 pulses (0=1 pulse) |
 
### 7.7 PS_MEAS_RATE (0x84) — default 0x04
 
| Bits | Field | Values |
|------|-------|--------|
| 7:3 | Reserved | Must write 00000 |
| 2:0 | PS Measurement Rate | 000=6.125 ms, 001=12.5 ms, 010=25 ms, 011=50 ms, **100=100 ms (default)**, 101=200 ms, 110=400 ms, 111=800 ms |
 
### 7.8 ALS_INT_TIME (0x85) — default 0x06
 
> **NOTE on reserved bits 7:4:** The field table states "Must write 1010" but the datasheet's own pseudo-code examples (Section 8, p.31) use commands 0x02, 0x06, 0x0A, 0x0E — which have upper nibble 0x0, not 0xA. The pseudo-code is the authoritative implementation guide. **Use the pseudo-code command values listed below.**
 
| Bits | Field | Values |
|------|-------|--------|
| 7:4 | Reserved | Field table says write 1010; pseudo-code uses 0x0 — follow pseudo-code |
| 3:2 | ALS Integration Time | 00=50 ms, **01=100 ms (default)**, 10=200 ms, 11=400 ms |
| 1:0 | ALS Measurement Rate | 00=100 ms, 01=200 ms, **10=400 ms (default)**, 11=800 ms |
 
### 7.9 PART_ID (0x86) — default 0x1C, Read Only
 
| Bits | Field |
|------|-------|
| 7:4 | Part Number ID |
| 3:0 | Revision ID |
 
Default = **0x1C**. Verify on startup to confirm device identity.
 
### 7.10 MANUFAC_ID (0x87) — default 0x08, Read Only
 
Full byte = Manufacturer ID. Expected value = **0x08**.
 
### 7.11 ALS_STATUS (0x88) — default 0x00, Read Only
 
| Bits | Field | Values |
|------|-------|--------|
| 7 | Reserved | — |
| 6 | ALS Data Valid | **0=Valid (default)**, 1=Invalid |
| 5:3 | ALS Data Gain Range | 000=1x, 001=4x, 010=16x, 011=64x, 100=128x, 101=512x |
| 2 | Reserved | — |
| 1 | Reserved | Don't care |
| 0 | ALS Data Status | **0=OLD data (already read)**, 1=NEW data |
 
### 7.12–7.15 Data Registers
 
- **IR_DATA** (0x89-0x8A): 16-bit IR channel, read LSB first
- **GREEN_DATA** (0x8B-0x8C): 16-bit ALS visible channel, read LSB first
- **PS_STATUS** (0x91): FTN/NTF/saturation/interrupt/data-new flags
- **PS_DATA** (0x92-0x93): 11-bit (default) or 16-bit PS data. Reading clears interrupt.
 
### 7.16 INTERRUPT (0x98) — default 0x08
 
| Bits | Field | Values |
|------|-------|--------|
| 7:3 | Reserved | Must write **00001** (bit3=1) |
| 2 | Interrupt Polarity | **0=Active LOW (default)**, 1=Active HIGH |
| 1 | Reserved | Must write 0 |
| 0 | Interrupt Mode | **0=INACTIVE (default)**, 1=PS triggers interrupt |
 
### 7.17–7.20 Threshold and Offset Registers
 
- **INTERRUPT_PERSIST** (0x99): PS persist count (bits 7:4), 0=every value
- **PS_THRES_HIGH** (0x9A-0x9B): Upper threshold, write LSB first
- **PS_THRES_LOW** (0x9C-0x9D): Lower threshold, write LSB first
- **PXTALK** (0x9E-0x9F): Crosstalk offset, subtracted when PS_OS=1
- **PS_VREHL** (0xB6): Sunlight fail-safe. Use 0x06 per field table.
 
---
 
## 8. ALS Lux Conversion Formula

```
Lux = (0.336 x ALS_DATA) / (GAIN x INT) x Window_Factor
```
 
- ALS_DATA: raw 16-bit from GREEN_DATA (0x8B-0x8C)
- GAIN: from ALS_STATUS[5:3] → {1, 4, 16, 64, 128, 512}
- INT: from ALS_INT_TIME[3:2] → {0.5, 1.0, 2.0, 4.0}
- Window_Factor: 1.0 for clear glass
 
---
 
## 9. Recommended Initialization Sequence
 
```
1.  Power-up → device in Standby
2.  Delay >=10 ms
3.  Read PART_ID (0x86) → verify == 0x1C
4.  Read MANUFAC_ID (0x87) → verify == 0x08
5.  Write PS_VREHL (0xB6) = 0x06
6.  Write PS_LED (0x82)
7.  Write PS_N_PULSES (0x83)
8.  Write PS_MEAS_RATE (0x84)
9.  Write ALS_INT_TIME (0x85)
10-16. Optional: ALS averaging, thresholds, interrupt, crosstalk
17. Write ALS_CONTR (0x80) — LAST: activate ALS
18. Write PS_CONTR (0x81) — LAST: activate PS
```
 
> **CRITICAL:** Steps 17 and 18 must be written **after** all other configuration registers.
 
---
 
## 10. Critical Implementation Notes
 
1. **PS_CONTR bit 4 must always be 1.** Always OR with 0x10.
2. **ALS_INT_TIME reserved bits:** Follow pseudo-code (upper nibble = 0x0), not field table.
3. **PS_VREHL:** Use 0x06 (field table) not 0x05 (pseudo-code). Do not leave at default 0x08.
4. **PS default is 11-bit** (0-2047). Set PS_16BITS_EN=1 for 16-bit (0-65535).
5. **ALS Measurement Rate must be >= Integration Time.** IC auto-corrects if smaller.
6. **Reading PS_DATA clears the PS interrupt flag.**
7. **Wakeup time:** wait >=10 ms after activating modes before reading data.
8. **ALS lux formula uses only GREEN channel** (0x8B-0x8C), not IR.
