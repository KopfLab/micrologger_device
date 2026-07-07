#pragma once

#include "uTypedef.h"
#include "Particle.h"
#include "uHardwareRheostat.h"

// hardware constants
#define RHEOSTAT_AD5241_I2C_ADDRESS 0x2C       // AD0 and AD1 are both tied to GND on the sensor board
#define RHEOSTAT_AD5241_MAX_STEPS 255          // 256-position (8-bit) wiper
#define RHEOSTAT_AD5241_WRITE_INSTRUCTION 0x00 // instruction byte

// AD5241 rheostat
class ThardwareRheostatAD5241 : public ThardwareRheostat
{

protected:
    // Unlike the AD5246/MCP4017 (single-byte frame, 7-bit wiper) the AD5241 uses a two-byte
    // I2C frame (instruction byte + 8-bit data byte) and an 8-bit (256-position) wiper register
    virtual uint8_t writeRegister(uint8_t _value) override
    {
        uint8_t transmitCode = SYSTEM_ERROR_NONE;
        // lock for thread safety
        WITH_LOCK(Wire)
        {
            Wire.beginTransmission(Fi2cAddress);
            Wire.write(RHEOSTAT_AD5241_WRITE_INSTRUCTION); // instruction byte
            Wire.write(_value);                            // data byte (wiper position 0-255)
            transmitCode = Wire.endTransmission();
        }
        return transmitCode;
    }

    // read the full 8-bit wiper value (no 7-bit masking like the AD5246/MCP4017)
    virtual uint8_t readRegister(uint8_t *_value) override
    {
        // lock for thread safety
        WITH_LOCK(Wire)
        {
            const uint8_t bytesRequested = 1;
            uint8_t request = Wire.requestFrom(Fi2cAddress, bytesRequested);
            if (request == bytesRequested && Wire.available())
                *_value = Wire.read(); // full 8 bits (256 positions)
            else
                // custom error: did not receive the expected number of bytes
                return 0xff;
        }
        return SYSTEM_ERROR_NONE;
    }

public:
    // default i2c address in init
    void init(Resistance _resistance, uint8_t _i2cAddress = RHEOSTAT_AD5241_I2C_ADDRESS)
    {
        maxSteps = RHEOSTAT_AD5241_MAX_STEPS;
        ThardwareRheostat::init(_i2cAddress, RHEOSTAT_AD5241_MAX_STEPS, _resistance);
        action = ThardwareI2C::Taction::write;
    }
};
