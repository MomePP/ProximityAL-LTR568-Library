/*
*****************************************************************************
@file         Proximity-ALS.h
@mainpage     Proximity & Ambient Light Sensor LTR-568ALS-01 Arduino library
@version      v1.0.0
@date         March 18, 2026
@brief        Header file for LTR-568ALS-01 proximity & ambient light sensor.
              Drop-in replacement for LTR-553ALS-01 library.

              Datasheet: DS86-2023-0003, Rev 1.2 (06/26/2024)
              Lite-On Technology Corp.

NOTE: Where the datasheet and the reference C implementation
      (AliOS-Things drv_als_liteon_ltr568.c) disagree, the
      datasheet is treated as authoritative. Discrepancies are
      noted with "REF_IMPL_DIFF" comments.
*****************************************************************************
*/

#ifndef __PROXIMITY_ALS_H__
#define __PROXIMITY_ALS_H__

#include <stdint.h>
#include "I2CUtils/i2c-register.hpp"

/* ── Logging ───────────────────────────────────────────────────────────── */
#if defined(ARDUINO_ARCH_ESP32)
#include "esp32-hal-log.h"
#else
/* Stub macros for non-ESP32 platforms */
#define log_i(format, ...)
#define log_d(format, ...)
#define log_w(format, ...)
#define log_e(format, ...)
#endif

/* ── I2C Address ───────────────────────────────────────────────────────── */
#define LTR568_ADDR 0x23

/* ── Register Map ──────────────────────────────────────────────────────── */
#define LTR568_ALS_AVE_LIMIT_REG    0x7E
#define LTR568_ALS_AVE_FAC_REG      0x7F
#define LTR568_ALS_CONTR_REG        0x80
#define LTR568_PS_CONTR_REG         0x81
#define LTR568_PS_LED_REG           0x82
#define LTR568_PS_N_PULSES_REG      0x83
#define LTR568_PS_MEAS_RATE_REG     0x84
#define LTR568_ALS_INT_TIME_REG     0x85
#define LTR568_PART_ID_REG          0x86
#define LTR568_MANUFAC_ID_REG       0x87
#define LTR568_ALS_STATUS_REG       0x88
#define LTR568_IR_DATA_LSB_REG      0x89
#define LTR568_IR_DATA_MSB_REG      0x8A
#define LTR568_GREEN_DATA_LSB_REG   0x8B
#define LTR568_GREEN_DATA_MSB_REG   0x8C
#define LTR568_PS_STATUS_REG        0x91
#define LTR568_PS_DATA_LSB_REG      0x92
#define LTR568_PS_DATA_MSB_REG      0x93
#define LTR568_INTERRUPT_REG        0x98
#define LTR568_INT_PERSIST_REG      0x99
#define LTR568_PS_THRES_HI_LSB_REG  0x9A
#define LTR568_PS_THRES_HI_MSB_REG  0x9B
#define LTR568_PS_THRES_LO_LSB_REG  0x9C
#define LTR568_PS_THRES_LO_MSB_REG  0x9D
#define LTR568_PXTALK_LSB_REG       0x9E
#define LTR568_PXTALK_MSB_REG       0x9F
#define LTR568_PS_VREHL_REG         0xB6

/* ── Expected ID Values ────────────────────────────────────────────────── */
/*
 * Datasheet §7.9: PART_ID default = 0x1C
 *   REF_IMPL_DIFF: the AliOS driver uses LTR568_PART_ID_VAL = 0.
 *   Trust the datasheet.
 */
#define LTR568_PART_ID_EXPECTED     0x1C
#define LTR568_MANUFAC_ID_EXPECTED  0x08

/* ═══════════════════════════════════════════════════════════════════════ */
/* ALS_CONTR (0x80) — default 0x20                                        */
/*   bit[0]   ALS_MODE        0=Standby  1=Active                         */
/*   bit[1]   Reserved        must write 0                                */
/*   bit[4:2] ALS_GAIN        000=1×  001=4×  010=16×  011=64×            */
/*                             100=128×  101=512×                          */
/*   bit[5]   IR_EN            0=disable  1=enable                        */
/*   bit[7:6] ALS_DR           00=16-bit  01=15-bit  10=14-bit  11=13-bit */
/* ═══════════════════════════════════════════════════════════════════════ */

/* ALS Mode (bit 0) */
#define ALS_ACTIVE_MODE             0x01
#define ALS_STANDBY_MODE            0x00
#define ALS_MODE_MASK               0x01

/* ALS Gain (bits 4:2) */
#define ALS_GAIN_1X                 (0x00 << 2)
#define ALS_GAIN_4X                 (0x01 << 2)
#define ALS_GAIN_16X                (0x02 << 2)
#define ALS_GAIN_64X                (0x03 << 2)
#define ALS_GAIN_128X               (0x04 << 2)
#define ALS_GAIN_512X               (0x05 << 2)
#define ALS_GAIN_MASK               0x1C

/* IR Enable (bit 5) */
#define ALS_IR_ENABLE               (0x01 << 5)
#define ALS_IR_DISABLE              (0x00 << 5)
#define ALS_IR_EN_MASK              0x20

/* ALS ADC Resolution (bits 7:6) */
#define ALS_DR_16BIT                (0x00 << 6)
#define ALS_DR_15BIT                (0x01 << 6)
#define ALS_DR_14BIT                (0x02 << 6)
#define ALS_DR_13BIT                (0x03 << 6)
#define ALS_DR_MASK                 0xC0

/* ═══════════════════════════════════════════════════════════════════════ */
/* PS_CONTR (0x81) — default 0x10                                         */
/*   bit[0]   SW_RST           0=no action  1=reset all regs              */
/*   bit[1]   PSMODE           0=Standby  1=Active                        */
/*   bit[2]   FTN_NTF_EN       0=disabled  1=enable direction reporting   */
/*   bit[3]   PS_OS            0=disabled  1=enable offset subtraction    */
/*   bit[4]   Reserved         MUST always write 1                        */
/*   bit[5]   PS_16BITS_EN     0=11-bit  1=16-bit                         */
/*   bit[7:6] Reserved         must write 00                              */
/* ═══════════════════════════════════════════════════════════════════════ */

/* Reserved bit 4 — MUST always be 1 in every PS_CONTR write */
#define PS_CONTR_RSVD_BIT4          0x10

/* PS Mode (bit 1) */
#define PS_ACTIVE_MODE              (0x01 << 1)
#define PS_STANDBY_MODE             (0x00 << 1)
#define PS_MODE_MASK                0x02

/* SW Reset (bit 0) — resets ALL registers, returns to standby */
#define PS_SW_RESET                 0x01

/* PS Offset Subtraction (bit 3) */
#define PS_OFFSET_EN                (0x01 << 3)
#define PS_OFFSET_MASK              0x08

/* FTN/NTF Direction Reporting (bit 2) */
#define PS_FTN_NTF_EN               (0x01 << 2)
#define PS_FTN_NTF_MASK             0x04

/* PS 16-bit Output (bit 5) — default is 11-bit */
#define PS_16BIT_EN                 (0x01 << 5)
#define PS_16BIT_MASK               0x20

/* ═══════════════════════════════════════════════════════════════════════ */
/* PS_LED (0x82) — default 0x7A                                           */
/*   bit[2:0] LED_CURRENT      000=0mA .. 111=240mA                      */
/*   bit[4:3] PULSE_WIDTH      00=4µs  01=8µs  10=16µs  11=32µs          */
/*   bit[6:5] PULSE_DUTY       00=12.5%  01=25%  10=50%  11=100%         */
/*   bit[7]   Reserved         must write 0                               */
/* ═══════════════════════════════════════════════════════════════════════ */

/* LED Current (bits 2:0) */
#define PS_LED_CURRENT_0MA          0x00
#define PS_LED_CURRENT_50MA         0x01
#define PS_LED_CURRENT_100MA        0x02
#define PS_LED_CURRENT_120MA        0x03
#define PS_LED_CURRENT_140MA        0x04
#define PS_LED_CURRENT_170MA        0x05
#define PS_LED_CURRENT_190MA        0x06
#define PS_LED_CURRENT_240MA        0x07
#define PS_LED_CURRENT_MASK         0x07

/* LED Pulse Width (bits 4:3) */
#define PS_LED_PULSE_WIDTH_4US      (0x00 << 3)
#define PS_LED_PULSE_WIDTH_8US      (0x01 << 3)
#define PS_LED_PULSE_WIDTH_16US     (0x02 << 3)
#define PS_LED_PULSE_WIDTH_32US     (0x03 << 3)
#define PS_LED_PULSE_WIDTH_MASK     0x18

/* LED Pulse Duty Cycle (bits 6:5) */
#define PS_LED_DUTY_12_5PCT         (0x00 << 5)
#define PS_LED_DUTY_25PCT           (0x01 << 5)
#define PS_LED_DUTY_50PCT           (0x02 << 5)
#define PS_LED_DUTY_100PCT          (0x03 << 5)
#define PS_LED_DUTY_MASK            0x60

/* ═══════════════════════════════════════════════════════════════════════ */
/* PS_N_PULSES (0x83) — default 0x00                                      */
/*   bit[4:0] PULSE_COUNT      0x00=1 pulse .. 0x1F=32 pulses             */
/*   bit[5]   Reserved         must write 0                               */
/*   bit[7:6] PS_AVG_FACTOR    00=none  01=2×  10=4×  11=8×              */
/* ═══════════════════════════════════════════════════════════════════════ */

#define PS_PULSE_COUNT_MASK         0x1F

/* PS Averaging Factor (bits 7:6) */
#define PS_AVG_NONE                 (0x00 << 6)
#define PS_AVG_2X                   (0x01 << 6)
#define PS_AVG_4X                   (0x02 << 6)
#define PS_AVG_8X                   (0x03 << 6)
#define PS_AVG_MASK                 0xC0

/* ═══════════════════════════════════════════════════════════════════════ */
/* PS_MEAS_RATE (0x84) — default 0x04                                     */
/*   bit[2:0] PS meas rate                                                */
/*   bit[7:3] Reserved — must write 00000                                 */
/* ═══════════════════════════════════════════════════════════════════════ */

#define PS_MEAS_RATE_6_125MS        0x00
#define PS_MEAS_RATE_12_5MS         0x01
#define PS_MEAS_RATE_25MS           0x02
#define PS_MEAS_RATE_50MS           0x03
#define PS_MEAS_RATE_100MS          0x04
#define PS_MEAS_RATE_200MS          0x05
#define PS_MEAS_RATE_400MS          0x06
#define PS_MEAS_RATE_800MS          0x07
#define PS_MEAS_RATE_MASK           0x07

/* ═══════════════════════════════════════════════════════════════════════ */
/* ALS_INT_TIME (0x85) — default 0x06                                     */
/*   bit[1:0] ALS_MEAS_RATE    00=100ms  01=200ms  10=400ms  11=800ms    */
/*   bit[3:2] ALS_INT_TIME     00=50ms  01=100ms  10=200ms  11=400ms     */
/*   bit[7:4] Reserved                                                    */
/*                                                                        */
/*   DATASHEET CONFLICT §7.8: field table says bits 7:4 "must write       */
/*   1010" but the pseudo-code examples (Section 8, p.31) use commands    */
/*   0x02, 0x06, 0x0A, 0x0E (upper nibble = 0x0). Pseudo-code is the     */
/*   authoritative implementation guide.  ► Use 0x0 upper nibble.         */
/*                                                                        */
/*   REF_IMPL_DIFF: the AliOS driver agrees with pseudo-code (0x0).      */
/*                                                                        */
/*   NOTE: ALS meas rate must be ≥ integration time.  If rate < int       */
/*   time the IC auto-corrects rate = int time.                           */
/* ═══════════════════════════════════════════════════════════════════════ */

/* ALS Integration Time (bits 3:2) */
#define ALS_INT_TIME_50MS           (0x00 << 2)
#define ALS_INT_TIME_100MS          (0x01 << 2)
#define ALS_INT_TIME_200MS          (0x02 << 2)
#define ALS_INT_TIME_400MS          (0x03 << 2)
#define ALS_INT_TIME_MASK           0x0C

/* ALS Measurement Rate (bits 1:0) */
#define ALS_MEAS_RATE_100MS         0x00
#define ALS_MEAS_RATE_200MS         0x01
#define ALS_MEAS_RATE_400MS         0x02
#define ALS_MEAS_RATE_800MS         0x03
#define ALS_MEAS_RATE_MASK          0x03

/* ═══════════════════════════════════════════════════════════════════════ */
/* ALS_STATUS (0x88) — read only                                          */
/*   bit[0]   ALS_DATA_STATUS  0=old  1=new                              */
/*   bit[2:1] Reserved                                                    */
/*   bit[5:3] ALS_DATA_GAIN    gain range used for current data           */
/*   bit[6]   ALS_DATA_VALID   0=valid  1=invalid                        */
/*   bit[7]   Reserved                                                    */
/*                                                                        */
/*   REF_IMPL_DIFF: the AliOS driver has gain at bits[6:3] (mask 0x78)   */
/*   and validity at bit[7] (mask 0x80).  Datasheet §7.11 places gain    */
/*   at bits[5:3] (mask 0x38) and validity at bit[6] (mask 0x40).        */
/*   ► Trust the datasheet.                                               */
/* ═══════════════════════════════════════════════════════════════════════ */

#define ALS_DATA_VALID_MASK         0x40    /* bit 6: 0=valid, 1=invalid */
#define ALS_DATA_GAIN_MASK          0x38    /* bits 5:3 */
#define ALS_DATA_GAIN_SHIFT         3
#define ALS_DATA_STATUS_MASK        0x01    /* bit 0: 0=old, 1=new */

/* ═══════════════════════════════════════════════════════════════════════ */
/* PS_STATUS (0x91) — read only, default 0x08                             */
/*   bit[0]   PS_DATA_STATUS   0=old  1=new                              */
/*   bit[1]   PS_INT_STATUS    0=inactive  1=active                      */
/*   bit[2]   AMB_SATURATION   0=none  1=saturated                       */
/*   bit[3]   Reserved         reads 1                                    */
/*   bit[4]   NTF              0=none  1=Near-to-Far detected            */
/*   bit[5]   FTN              0=none  1=Far-to-Near detected            */
/*   bit[7:6] Reserved                                                    */
/* ═══════════════════════════════════════════════════════════════════════ */

#define PS_FTN_MASK                 0x20    /* bit 5 */
#define PS_NTF_MASK                 0x10    /* bit 4 */
#define PS_AMB_SAT_MASK             0x04    /* bit 2 */
#define PS_INT_STATUS_MASK          0x02    /* bit 1 */
#define PS_DATA_STATUS_MASK         0x01    /* bit 0 */

/* ═══════════════════════════════════════════════════════════════════════ */
/* PART_ID (0x86) — read only                                             */
/* ═══════════════════════════════════════════════════════════════════════ */

#define PART_NUMBER_ID_MASK         0xF0
#define REVISION_ID_MASK            0x0F

/* ═══════════════════════════════════════════════════════════════════════ */
/* INTERRUPT (0x98) — default 0x08                                        */
/*   bit[0]   INT_MODE         0=inactive  1=PS triggers interrupt        */
/*   bit[1]   Reserved         must write 0                               */
/*   bit[2]   INT_POLARITY     0=active LOW  1=active HIGH               */
/*   bit[7:3] Reserved         must write 00001 (bit3=1)                  */
/*                                                                        */
/*   DATASHEET CONFLICT §7.16: pseudo-code uses 0x88/0x89/0x8C which     */
/*   sets bit7=1.  Field table says bits[7:4] must be 0000.              */
/*   ► Trust the field table.  Use 0x08 base.                             */
/* ═══════════════════════════════════════════════════════════════════════ */

#define INT_BASE                    0x08    /* reserved bit3=1 */
#define INT_MODE_PS_TRIGGER         0x01    /* bit 0 */
#define INT_POLARITY_HIGH           0x04    /* bit 2 */

/* ═══════════════════════════════════════════════════════════════════════ */
/* INTERRUPT_PERSIST (0x99) — default 0x00                                */
/*   bit[7:4] PS_PERSIST       0=every, 1–15 consecutive values          */
/*   bit[3:0] Reserved         must write 0000                           */
/* ═══════════════════════════════════════════════════════════════════════ */

#define INT_PERSIST_MASK            0xF0

/* ═══════════════════════════════════════════════════════════════════════ */
/* Lux formula (datasheet §8)                                             */
/*   Lux = (0.336 × ALS_DATA) / (GAIN × INT) × Window_Factor             */
/* ═══════════════════════════════════════════════════════════════════════ */

#define LTR568_LUX_COEFF            0.336f

/* ═══════════════════════════════════════════════════════════════════════ */
/* PS_VREHL (0xB6) — sunlight fail-safe                                   */
/*                                                                        */
/*   DATASHEET CONFLICT §7.20: field table says write 0x06, pseudo-code   */
/*   says write 0x05.  Default reset value is 0x08.                       */
/*   ► Use 0x06 (field table).                                            */
/* ═══════════════════════════════════════════════════════════════════════ */

#define PS_VREHL_RECOMMENDED        0x06

/* ═══════════════════════════════════════════════════════════════════════ */
/* Class definition                                                       */
/* ═══════════════════════════════════════════════════════════════════════ */

class ProximityAL : public I2CDevice
{
public:
    ProximityAL(void);
    ProximityAL(uint8_t i2c_address);

    /**
     * @brief  Initialise the sensor following datasheet §9.3 sequence.
     *         Verifies device identity, configures all registers, then
     *         activates ALS and PS modes LAST.
     * @retval true   device found and initialised
     * @retval false  device not responding on I2C
     */
    bool begin(void);

    /**
     * @brief  Software reset (PS_CONTR bit 0).
     *         Resets ALL registers to defaults and returns to standby.
     *         Full re-initialisation is required after calling this.
     */
    void softwareReset(void);

    /* === ALS Configuration ============================================ */

    void setALSmode(uint8_t mode);
    void setALSgain(uint8_t gain);
    uint8_t getALSgain(void);
    void setALSresolution(uint8_t resolution);
    void setIRenable(bool enable);
    void setALSintegrationTime(uint8_t intTime);
    uint8_t getALSintegrationTime(void);
    void setALSmeasurementRate(uint8_t measRate);

    /* === PS Configuration ============================================= */

    void setPSmode(uint8_t mode);
    void setPSledCurrent(uint8_t current);
    void setPSledPulseWidth(uint8_t width);
    void setPSledDutyCycle(uint8_t dutyCycle);
    void setPSledPulseCount(uint8_t count);
    void setPSaverageFactor(uint8_t factor);
    void setPSmeasurementRate(uint8_t measRate);
    void setPS16bitMode(bool enable);
    void setPSoffsetSubtraction(bool enable);
    void setPScrosstalk(uint16_t offset);
    void setPSvrehl(uint8_t value);

    /* === Interrupt Configuration ====================================== */

    void setInterrupt(bool enable, bool activeHigh = false);
    void setInterruptPersist(uint8_t count);
    void setPSthresholdHigh(uint16_t threshold);
    void setPSthresholdLow(uint16_t threshold);

    /* === Identification =============================================== */

    uint8_t getPartNumberID(void);
    uint8_t getRevisionID(void);
    uint8_t getManufacturerID(void);

    /* === Data Reading ================================================= */

    uint16_t getPSvalue(void);
    uint16_t getGreenValue(void);
    uint16_t getIRvalue(void);
    float    getLuxValue(void);

    /**
     * @brief  Set window factor for lux calculation.
     * @param  factor  1.0 = clear glass / no window.
     *                 >1.0 for tinted window (calibrate under white LED).
     */
    void setWindowFactor(float factor);

    /* === Status ======================================================= */

    bool    isALSdataNew(void);
    bool    isALSdataValid(void);
    bool    isPSdataNew(void);
    uint8_t getALSstatusGain(void);
    uint8_t getPSstatus(void);

    /* === Backward Compatibility (LTR-553 API) ========================= */

    /** @brief Alias for getGreenValue().  Maps to LTR-553 CH0 (vis+IR). */
    uint16_t getALSCH0value(void) { return getGreenValue(); }

    /** @brief Alias for getIRvalue().  Maps to LTR-553 CH1 (IR). */
    uint16_t getALSCH1value(void) { return getIRvalue(); }

private:
    uint8_t _address;
    float   _windowFactor;
};

#endif /* __PROXIMITY_ALS_H__ */
