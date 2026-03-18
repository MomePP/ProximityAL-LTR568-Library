/*
*****************************************************************************
@file         Proximity-ALS.cpp
@mainpage     Proximity & Ambient Light Sensor LTR-568ALS-01 Arduino library
@version      v1.0.0
@date         March 18, 2026
@brief        Implementation for LTR-568ALS-01 proximity & ambient light
              sensor.  Drop-in replacement for LTR-553ALS-01 library.

              Datasheet: DS86-2023-0003, Rev 1.2 (06/26/2024)
              Lite-On Technology Corp.

Library includes:
--> Configuration functions
--> Data reading functions
--> Lux conversion (datasheet §8 formula)
--> I2C communication via I2CUtils
*****************************************************************************
*/

#include "LTR568.h"

/* ── Gain lookup: index from ALS_STATUS[5:3] → multiplier ──────────── */
static const uint16_t _gainLUT[] = {1, 4, 16, 64, 128, 512};

/* ── Integration time lookup: index from ALS_INT_TIME[3:2] → factor ── */
static const float _intTimeLUT[] = {0.5f, 1.0f, 2.0f, 4.0f};

/* ═══════════════════════════════════════════════════════════════════════ */
/* Construction                                                           */
/* ═══════════════════════════════════════════════════════════════════════ */

LTR568::LTR568()
    : LTR568(LTR568_ADDR) {}

LTR568::LTR568(uint8_t i2c_address)
    : I2CDevice(i2c_address), _windowFactor(1.0f)
{
    _address = i2c_address;
}

/* ═══════════════════════════════════════════════════════════════════════ */
/* Initialisation — follows datasheet §9.3 strictly                       */
/*                                                                        */
/*   1.  Power-up → standby                                               */
/*   2.  Delay ≥10 ms                                                     */
/*   3.  Verify PART_ID  (0x86) == 0x1C                                   */
/*   4.  Verify MANUFAC_ID (0x87) == 0x08                                 */
/*   5.  Write PS_VREHL  (0xB6) — sunlight fail-safe                     */
/*   6.  Write PS_LED    (0x82) — LED pulse config                        */
/*   7.  Write PS_N_PULSES (0x83) — pulse count & averaging               */
/*   8.  Write PS_MEAS_RATE (0x84) — PS measurement rate                  */
/*   9.  Write ALS_INT_TIME (0x85) — integration time & ALS meas rate     */
/*  17.  Write ALS_CONTR (0x80) — activate ALS   ◄ MUST BE LAST          */
/*  18.  Write PS_CONTR  (0x81) — activate PS    ◄ MUST BE LAST          */
/* ═══════════════════════════════════════════════════════════════════════ */

bool LTR568::begin(void)
{
    bool deviceExists = false;

    Wire.begin();
    Wire.beginTransmission(_address);
    if (Wire.endTransmission() == 0)
    {
        deviceExists = true;

        /* Step 2 — post-power-up delay */
        delay(10);

        /* Steps 3–4 — read identity registers (triggers internal latch) */
        read8(LTR568_PART_ID_REG);
        read8(LTR568_MANUFAC_ID_REG);

        /* ── Configure ALL registers BEFORE activating modes ────────── */

        /* Step 5 — PS sunlight fail-safe
         *   DATASHEET CONFLICT §7.20: field table → 0x06, pseudo-code → 0x05.
         *   Using 0x06 per field table (≈50 klux threshold).
         */
        write_register(LTR568_PS_VREHL_REG, PS_VREHL_RECOMMENDED);

        /* Step 6 — PS LED: 100% duty, 16 µs pulse width, 100 mA
         *   Reduced from 190 mA / 32 µs to lower power and faster acquisition.
         *   100 mA at 16 µs is sufficient for close-range (~5 cm) detection.
         */
        write_register(LTR568_PS_LED_REG,
                       PS_LED_DUTY_100PCT | PS_LED_PULSE_WIDTH_16US | PS_LED_CURRENT_100MA);

        /* Step 7 — PS pulses: 8 pulses (reg value 0x07), no averaging
         *   Datasheet §7.6: pulse count 0x00=1 .. 0x1F=32.
         *   8 pulses balances SNR vs acquisition speed.
         */
        write_register(LTR568_PS_N_PULSES_REG, PS_AVG_NONE | 0x07);

        /* Step 8 — PS measurement rate: 50 ms
         *   Firmware polls every 100 ms — 50 ms ensures a fresh reading each cycle.
         */
        write_register(LTR568_PS_MEAS_RATE_REG, PS_MEAS_RATE_50MS);

        /* Step 9 — ALS: integration time 100 ms, meas rate 400 ms (default 0x06)
         *   Upper nibble = 0x0 per datasheet pseudo-code (NOT 0xA0).
         */
        write_register(LTR568_ALS_INT_TIME_REG, ALS_INT_TIME_100MS | ALS_MEAS_RATE_400MS);

        /* Step 17 — Activate ALS LAST
         *   IR enabled, 16-bit resolution, gain 1×, active mode.
         *   Default reset = 0x20 (IR_EN=1, standby).  We just add ACTIVE.
         */
        write_register(LTR568_ALS_CONTR_REG,
                       ALS_DR_16BIT | ALS_IR_ENABLE | ALS_GAIN_1X | ALS_ACTIVE_MODE);

        /* Step 18 — Activate PS LAST
         *   Reserved bit4 MUST be 1.
         */
        write_register(LTR568_PS_CONTR_REG,
                       PS_CONTR_RSVD_BIT4 | PS_ACTIVE_MODE);

        /* Wait for first measurement to become available (§4.3: 5–10 ms) */
        delay(10);
    }

    return deviceExists;
}

/* ═══════════════════════════════════════════════════════════════════════ */
/* Software Reset                                                         */
/*   Located in PS_CONTR bit[0] (NOT ALS_CONTR like the LTR-553).         */
/*   Resets ALL registers to power-on defaults.                           */
/*   Full re-initialisation via begin() is required after calling this.   */
/* ═══════════════════════════════════════════════════════════════════════ */

void LTR568::softwareReset(void)
{
    write_register(LTR568_PS_CONTR_REG, PS_CONTR_RSVD_BIT4 | PS_SW_RESET);
    delay(10);
}

/* ═══════════════════════════════════════════════════════════════════════ */
/* ALS Configuration                                                      */
/* ═══════════════════════════════════════════════════════════════════════ */

/**
 * @brief  Set ALS operating mode.
 * @param  mode  ALS_ACTIVE_MODE or ALS_STANDBY_MODE
 */
void LTR568::setALSmode(uint8_t mode)
{
    uint8_t reg = read8(LTR568_ALS_CONTR_REG);
    reg = (reg & ~ALS_MODE_MASK) | (mode & ALS_MODE_MASK);
    write_register(LTR568_ALS_CONTR_REG, reg);
}

/**
 * @brief  Set ALS gain.
 * @param  gain  One of ALS_GAIN_1X .. ALS_GAIN_512X (pre-shifted).
 */
void LTR568::setALSgain(uint8_t gain)
{
    uint8_t reg = read8(LTR568_ALS_CONTR_REG);
    reg = (reg & ~ALS_GAIN_MASK) | (gain & ALS_GAIN_MASK);
    write_register(LTR568_ALS_CONTR_REG, reg);
}

/**
 * @brief  Read configured ALS gain from ALS_CONTR register.
 * @retval Pre-shifted gain field (ALS_GAIN_1X .. ALS_GAIN_512X).
 */
uint8_t LTR568::getALSgain(void)
{
    return read8(LTR568_ALS_CONTR_REG) & ALS_GAIN_MASK;
}

/**
 * @brief  Set ALS ADC resolution.
 * @param  resolution  ALS_DR_16BIT .. ALS_DR_13BIT (pre-shifted).
 */
void LTR568::setALSresolution(uint8_t resolution)
{
    uint8_t reg = read8(LTR568_ALS_CONTR_REG);
    reg = (reg & ~ALS_DR_MASK) | (resolution & ALS_DR_MASK);
    write_register(LTR568_ALS_CONTR_REG, reg);
}

/**
 * @brief  Enable or disable the IR channel.
 * @param  enable  true = IR data available at 0x89–0x8A.
 */
void LTR568::setIRenable(bool enable)
{
    uint8_t reg = read8(LTR568_ALS_CONTR_REG);
    reg = (reg & ~ALS_IR_EN_MASK) | (enable ? ALS_IR_ENABLE : ALS_IR_DISABLE);
    write_register(LTR568_ALS_CONTR_REG, reg);
}

/**
 * @brief  Set ALS integration time.
 * @param  intTime  One of ALS_INT_TIME_50MS .. ALS_INT_TIME_400MS (pre-shifted).
 * @note   Upper nibble forced to 0x0 per datasheet pseudo-code.
 *         ALS meas rate auto-corrects if set below integration time.
 */
void LTR568::setALSintegrationTime(uint8_t intTime)
{
    uint8_t reg = read8(LTR568_ALS_INT_TIME_REG);
    reg = (reg & ~ALS_INT_TIME_MASK) | (intTime & ALS_INT_TIME_MASK);
    reg &= 0x0F; /* upper nibble must be 0x0 per pseudo-code */
    write_register(LTR568_ALS_INT_TIME_REG, reg);
}

/**
 * @brief  Read current ALS integration time setting.
 * @retval Pre-shifted integration time field.
 */
uint8_t LTR568::getALSintegrationTime(void)
{
    return read8(LTR568_ALS_INT_TIME_REG) & ALS_INT_TIME_MASK;
}

/**
 * @brief  Set ALS measurement repeat rate.
 * @param  measRate  One of ALS_MEAS_RATE_100MS .. ALS_MEAS_RATE_800MS.
 * @note   Must be ≥ integration time; IC auto-corrects if smaller.
 */
void LTR568::setALSmeasurementRate(uint8_t measRate)
{
    uint8_t reg = read8(LTR568_ALS_INT_TIME_REG);
    reg = (reg & ~ALS_MEAS_RATE_MASK) | (measRate & ALS_MEAS_RATE_MASK);
    reg &= 0x0F; /* upper nibble must be 0x0 per pseudo-code */
    write_register(LTR568_ALS_INT_TIME_REG, reg);
}

/* ═══════════════════════════════════════════════════════════════════════ */
/* PS Configuration                                                       */
/*   IMPORTANT: every write to PS_CONTR must OR with PS_CONTR_RSVD_BIT4  */
/*   (bit 4 must always be 1).                                            */
/* ═══════════════════════════════════════════════════════════════════════ */

/**
 * @brief  Set PS operating mode.
 * @param  mode  PS_ACTIVE_MODE or PS_STANDBY_MODE (pre-shifted to bit 1).
 */
void LTR568::setPSmode(uint8_t mode)
{
    uint8_t reg = read8(LTR568_PS_CONTR_REG);
    reg = (reg & ~PS_MODE_MASK) | (mode & PS_MODE_MASK);
    reg |= PS_CONTR_RSVD_BIT4;
    write_register(LTR568_PS_CONTR_REG, reg);
}

/**
 * @brief  Set PS LED peak current.
 * @param  current  PS_LED_CURRENT_0MA .. PS_LED_CURRENT_240MA.
 */
void LTR568::setPSledCurrent(uint8_t current)
{
    uint8_t reg = read8(LTR568_PS_LED_REG);
    reg = (reg & ~PS_LED_CURRENT_MASK) | (current & PS_LED_CURRENT_MASK);
    write_register(LTR568_PS_LED_REG, reg);
}

/**
 * @brief  Set PS LED pulse width.
 * @param  width  PS_LED_PULSE_WIDTH_4US .. PS_LED_PULSE_WIDTH_32US (pre-shifted).
 */
void LTR568::setPSledPulseWidth(uint8_t width)
{
    uint8_t reg = read8(LTR568_PS_LED_REG);
    reg = (reg & ~PS_LED_PULSE_WIDTH_MASK) | (width & PS_LED_PULSE_WIDTH_MASK);
    write_register(LTR568_PS_LED_REG, reg);
}

/**
 * @brief  Set PS LED duty cycle.
 * @param  dutyCycle  PS_LED_DUTY_12_5PCT .. PS_LED_DUTY_100PCT (pre-shifted).
 */
void LTR568::setPSledDutyCycle(uint8_t dutyCycle)
{
    uint8_t reg = read8(LTR568_PS_LED_REG);
    reg = (reg & ~PS_LED_DUTY_MASK) | (dutyCycle & PS_LED_DUTY_MASK);
    write_register(LTR568_PS_LED_REG, reg);
}

/**
 * @brief  Set PS LED pulse count.
 * @param  count  0 = 1 pulse, 0x1F = 32 pulses (raw register value).
 */
void LTR568::setPSledPulseCount(uint8_t count)
{
    uint8_t reg = read8(LTR568_PS_N_PULSES_REG);
    reg = (reg & ~PS_PULSE_COUNT_MASK) | (count & PS_PULSE_COUNT_MASK);
    write_register(LTR568_PS_N_PULSES_REG, reg);
}

/**
 * @brief  Set PS digital averaging factor.
 * @param  factor  PS_AVG_NONE, PS_AVG_2X, PS_AVG_4X, or PS_AVG_8X (pre-shifted).
 */
void LTR568::setPSaverageFactor(uint8_t factor)
{
    uint8_t reg = read8(LTR568_PS_N_PULSES_REG);
    reg = (reg & ~PS_AVG_MASK) | (factor & PS_AVG_MASK);
    write_register(LTR568_PS_N_PULSES_REG, reg);
}

/**
 * @brief  Set PS measurement repeat rate.
 * @param  measRate  PS_MEAS_RATE_6_125MS .. PS_MEAS_RATE_800MS.
 */
void LTR568::setPSmeasurementRate(uint8_t measRate)
{
    write_register(LTR568_PS_MEAS_RATE_REG, measRate & PS_MEAS_RATE_MASK);
}

/**
 * @brief  Enable 16-bit PS output (default is 11-bit).
 * @param  enable  true=16-bit, false=11-bit.
 */
void LTR568::setPS16bitMode(bool enable)
{
    uint8_t reg = read8(LTR568_PS_CONTR_REG);
    reg = (reg & ~PS_16BIT_MASK) | (enable ? PS_16BIT_EN : 0x00);
    reg |= PS_CONTR_RSVD_BIT4;
    write_register(LTR568_PS_CONTR_REG, reg);
}

/**
 * @brief  Enable PS offset (crosstalk) subtraction.
 * @param  enable  true=subtract PXTALK from PS_DATA.
 */
void LTR568::setPSoffsetSubtraction(bool enable)
{
    uint8_t reg = read8(LTR568_PS_CONTR_REG);
    reg = (reg & ~PS_OFFSET_MASK) | (enable ? PS_OFFSET_EN : 0x00);
    reg |= PS_CONTR_RSVD_BIT4;
    write_register(LTR568_PS_CONTR_REG, reg);
}

/**
 * @brief  Set PS crosstalk cancellation offset value.
 * @param  offset  16-bit offset subtracted from PS readings when PS_OS=1.
 * @note   Write LSB before MSB (datasheet §7.19).
 */
void LTR568::setPScrosstalk(uint16_t offset)
{
    write_register(LTR568_PXTALK_LSB_REG, (uint8_t)(offset & 0xFF));
    write_register(LTR568_PXTALK_MSB_REG, (uint8_t)((offset >> 8) & 0xFF));
}

/**
 * @brief  Set PS sunlight fail-safe threshold.
 * @param  value  Recommended: 0x06 (datasheet field table) or 0x05 (pseudo-code).
 */
void LTR568::setPSvrehl(uint8_t value)
{
    write_register(LTR568_PS_VREHL_REG, value);
}

/* ═══════════════════════════════════════════════════════════════════════ */
/* Interrupt Configuration                                                */
/* ═══════════════════════════════════════════════════════════════════════ */

/**
 * @brief  Enable/disable PS interrupt and set polarity.
 * @param  enable     true = PS measurement triggers interrupt pin.
 * @param  activeHigh false = active LOW (default), true = active HIGH.
 */
void LTR568::setInterrupt(bool enable, bool activeHigh)
{
    uint8_t reg = INT_BASE; /* reserved bit3=1 */
    if (enable)
        reg |= INT_MODE_PS_TRIGGER;
    if (activeHigh)
        reg |= INT_POLARITY_HIGH;
    write_register(LTR568_INTERRUPT_REG, reg);
}

/**
 * @brief  Set PS interrupt persist count.
 * @param  count  0=every value, 1–15 consecutive out-of-range before INT.
 */
void LTR568::setInterruptPersist(uint8_t count)
{
    write_register(LTR568_INT_PERSIST_REG, (count << 4) & INT_PERSIST_MASK);
}

/**
 * @brief  Set PS upper threshold for interrupt.
 * @note   Write LSB before MSB (both latch when MSB is written).
 */
void LTR568::setPSthresholdHigh(uint16_t threshold)
{
    write_register(LTR568_PS_THRES_HI_LSB_REG, (uint8_t)(threshold & 0xFF));
    write_register(LTR568_PS_THRES_HI_MSB_REG, (uint8_t)((threshold >> 8) & 0xFF));
}

/**
 * @brief  Set PS lower threshold for interrupt.
 */
void LTR568::setPSthresholdLow(uint16_t threshold)
{
    write_register(LTR568_PS_THRES_LO_LSB_REG, (uint8_t)(threshold & 0xFF));
    write_register(LTR568_PS_THRES_LO_MSB_REG, (uint8_t)((threshold >> 8) & 0xFF));
}

/* ═══════════════════════════════════════════════════════════════════════ */
/* Identification                                                         */
/* ═══════════════════════════════════════════════════════════════════════ */

/**
 * @brief  Read Part Number ID (upper nibble of PART_ID register).
 * @retval 4-bit part number.
 */
uint8_t LTR568::getPartNumberID(void)
{
    return (read8(LTR568_PART_ID_REG) & PART_NUMBER_ID_MASK) >> 4;
}

/**
 * @brief  Read Revision ID (lower nibble of PART_ID register).
 * @retval 4-bit revision number.
 */
uint8_t LTR568::getRevisionID(void)
{
    return read8(LTR568_PART_ID_REG) & REVISION_ID_MASK;
}

/**
 * @brief  Read Manufacturer ID.
 * @retval Expected 0x08 for Lite-On.
 */
uint8_t LTR568::getManufacturerID(void)
{
    return read8(LTR568_MANUFAC_ID_REG);
}

/* ═══════════════════════════════════════════════════════════════════════ */
/* Data Reading                                                           */
/* ═══════════════════════════════════════════════════════════════════════ */

/**
 * @brief  Read proximity sensor value.
 * @retval 11-bit value (0–2047) in default mode,
 *         16-bit (0–65535) when PS_16BITS_EN is set.
 *
 * @note   Reading PS_DATA clears the PS interrupt flag.
 *         In default 11-bit mode the upper 5 bits read as 0, so
 *         the returned range is 0–2047 — same as LTR-553.
 */
uint16_t LTR568::getPSvalue(void)
{
    uint8_t buffer[2];
    read_register(LTR568_PS_DATA_LSB_REG, 2, &buffer[0]);
    return ((uint16_t)buffer[1] << 8) | buffer[0];
}

/**
 * @brief  Read ALS visible (green) channel data.
 * @retval 16-bit raw value from GREEN_DATA registers (0x8B–0x8C).
 */
uint16_t LTR568::getGreenValue(void)
{
    uint8_t buffer[2];
    read_register(LTR568_GREEN_DATA_LSB_REG, 2, &buffer[0]);
    return ((uint16_t)buffer[1] << 8) | buffer[0];
}

/**
 * @brief  Read IR channel data.
 * @retval 16-bit raw value from IR_DATA registers (0x89–0x8A).
 * @note   IR channel must be enabled (IR_EN=1 in ALS_CONTR).
 */
uint16_t LTR568::getIRvalue(void)
{
    uint8_t buffer[2];
    read_register(LTR568_IR_DATA_LSB_REG, 2, &buffer[0]);
    return ((uint16_t)buffer[1] << 8) | buffer[0];
}

/**
 * @brief  Calculate lux from ALS green channel using datasheet §8 formula.
 *
 *         Lux = (0.336 × ALS_DATA) / (GAIN × INT) × Window_Factor
 *
 *         - ALS_DATA  : raw 16-bit from GREEN_DATA (0x8B–0x8C)
 *         - GAIN      : actual gain read from ALS_STATUS[5:3] at measurement time
 *         - INT       : integration time factor {0.5, 1.0, 2.0, 4.0}
 *         - Window_Factor : 1.0 for clear glass (set via setWindowFactor())
 *
 * @retval Calculated lux value, or -1.0f if data is invalid.
 *
 * @note   Unlike LTR-553 which uses a ratio-based dual-channel formula
 *         (CH0 + CH1), the LTR-568 uses only the green channel with a
 *         single coefficient.
 */
float LTR568::getLuxValue(void)
{
    /* Read ALS_STATUS to check validity and get actual gain */
    uint8_t status = read8(LTR568_ALS_STATUS_REG);

    /* Bit 6: 0=valid, 1=invalid */
    if (status & ALS_DATA_VALID_MASK)
    {

        return -1.0f;
    }

    /* Gain from ALS_STATUS bits[5:3] — this is the actual gain the IC used */
    uint8_t gainIdx = (status & ALS_DATA_GAIN_MASK) >> ALS_DATA_GAIN_SHIFT;
    float gain = (gainIdx < sizeof(_gainLUT) / sizeof(_gainLUT[0]))
                     ? (float)_gainLUT[gainIdx]
                     : 1.0f;

    /* Integration time from ALS_INT_TIME bits[3:2] */
    uint8_t intTimeReg = read8(LTR568_ALS_INT_TIME_REG);
    uint8_t intIdx = (intTimeReg & ALS_INT_TIME_MASK) >> 2;
    float intFactor = (intIdx < sizeof(_intTimeLUT) / sizeof(_intTimeLUT[0]))
                          ? _intTimeLUT[intIdx]
                          : 1.0f;

    /* Read green (visible) channel */
    uint16_t alsData = getGreenValue();

    /* Lux = (0.336 × ALS_DATA) / (GAIN × INT) × Window_Factor */
    float lux = (LTR568_LUX_COEFF * (float)alsData / (gain * intFactor)) * _windowFactor;

    return lux;
}

/**
 * @brief  Set window compensation factor for lux calculation.
 * @param  factor  1.0 = no window / clear glass (default).
 *                 >1.0 for tinted or recessed window — calibrate with white LED.
 */
void LTR568::setWindowFactor(float factor)
{
    _windowFactor = factor;
}

/* ═══════════════════════════════════════════════════════════════════════ */
/* Status                                                                 */
/* ═══════════════════════════════════════════════════════════════════════ */

/**
 * @brief  Check if new ALS data is available.
 * @retval true = new unread data, false = old/already read.
 */
bool LTR568::isALSdataNew(void)
{
    return (read8(LTR568_ALS_STATUS_REG) & ALS_DATA_STATUS_MASK) != 0;
}

/**
 * @brief  Check if current ALS data is valid.
 * @retval true = valid, false = invalid.
 */
bool LTR568::isALSdataValid(void)
{
    /* Bit 6: 0 = valid, 1 = invalid → invert for bool semantics */
    return (read8(LTR568_ALS_STATUS_REG) & ALS_DATA_VALID_MASK) == 0;
}

/**
 * @brief  Check if new PS data is available.
 * @retval true = new unread data.
 */
bool LTR568::isPSdataNew(void)
{
    return (read8(LTR568_PS_STATUS_REG) & PS_DATA_STATUS_MASK) != 0;
}

/**
 * @brief  Read actual ALS gain from ALS_STATUS register.
 * @retval Gain index 0–5 corresponding to {1, 4, 16, 64, 128, 512}×.
 */
uint8_t LTR568::getALSstatusGain(void)
{
    return (read8(LTR568_ALS_STATUS_REG) & ALS_DATA_GAIN_MASK) >> ALS_DATA_GAIN_SHIFT;
}

/**
 * @brief  Read raw PS_STATUS register.
 * @retval Raw byte — use PS_FTN_MASK, PS_NTF_MASK, PS_AMB_SAT_MASK,
 *         PS_INT_STATUS_MASK, PS_DATA_STATUS_MASK to decode.
 */
uint8_t LTR568::getPSstatus(void)
{
    return read8(LTR568_PS_STATUS_REG);
}
