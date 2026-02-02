#pragma once

#include "uTypedef.h"
#include "Particle.h"
#include "uHardwareI2C.h"

// hardware constants
#define TEMPERATUER_SENSOR_I2C_ADDRESS 0x48

// IO expander: TCA9534 8-channel multiplexer with 1 wire (I2C) commms
class ThardwareSensorTMP117 : public ThardwareI2C
{

private:
    // registers
    static constexpr uint8_t FpinTempRegister = 0x00; // remperature register

    // calculation constants
    static constexpr dtypes::float32 FtempCelsiusPerBit = 0.0078125f; // 7.8125 m Celsius per LSB

    // low level interactions with the chip via I2C

    /**
     * @brief read a register value (2 bytes = uint16)
     * @return error code from Wire.endTransmission() or custom error code 0xff if request failed
     */
    uint8_t readRegister(uint8_t _register, uint16_t *_value)
    {
        uint8_t transmitCode = SYSTEM_ERROR_NONE;
        // lock for thread safety
        WITH_LOCK(Wire)
        {
            Wire.beginTransmission(Fi2cAddress);
            Wire.write(_register); // flag register for reading
            // false = keep connection alive instead of stop
            transmitCode = Wire.endTransmission(false);

            // if it worked, request the uint8_t data
            if (transmitCode != SYSTEM_ERROR_NONE)
                return transmitCode;

            const uint8_t bytesRequested = 2;
            uint8_t request = Wire.requestFrom(Fi2cAddress, bytesRequested);
            if (request != bytesRequested && Wire.available())
                // custom error: did not receive the expected number of bytes
                return 0xff;

            uint8_t msb = Wire.read();
            uint8_t lsb = Wire.read();
            *_value = (uint16_t)((msb << 8) | lsb);
        }
        return SYSTEM_ERROR_NONE;
    }

    // convert raw signal to celsius
    static dtypes::float32 rawToC(uint16_t _raw)
    {
        int16_t raw_signed = static_cast<int16_t>(_raw);
        return static_cast<dtypes::float32>(raw_signed) * FtempCelsiusPerBit;
    }

    // I2C read function
    virtual bool read() override
    {
        if (status != enums::TconStatus::connected && !connect())
            return false;

        uint16_t raw = 0;
        if (readRegister(FpinTempRegister, &raw) != SYSTEM_ERROR_NONE)
            return false;

        temperature_C = rawToC(raw);
        return true;
    }

public:
    // sdds vars
    sdds_var(Tfloat32, temperature_C, sdds::opt::readonly);

    // constructor
    ThardwareSensorTMP117()
    {
    }

    // default i2c address in init
    void init(uint8_t _i2cAddress = TEMPERATUER_SENSOR_I2C_ADDRESS)
    {
        ThardwareI2C::init(_i2cAddress);
    }
};