#pragma once

#include "uTypedef.h"
#include "Particle.h"
#include "uHardwareI2C.h"

// base class for MCP4017 and AD5246 rheostats
class ThardwareRheostat : public ThardwareI2C
{

public:
    // enumerations

    // Enumeration for allowed resistor values
    enum class Resistance
    {
        R5k = 5000,
        R10k = 10000,
        R50k = 50000,
        R100k = 100000
    };

private:
    // low level interactions with the chip via I2C

    /**
     * @brief read the register value
     * @return error code from Wire.endTransmission() or custom error code 0xff if uint8_t request failed
     */
    uint8_t readRegister(uint8_t *_value)
    {
        uint8_t transmitCode = SYSTEM_ERROR_NONE;
        // lock for thread safety
        WITH_LOCK(Wire)
        {
            const uint8_t bytesRequested = 1;
            uint8_t request = Wire.requestFrom(Fi2cAddress, bytesRequested);
            if (request == bytesRequested && Wire.available())
            {
                *_value = Wire.read();
                *_value &= 0x7F; // mask to 7 bits
            }
            else
                // custom error: did not receive the expected number of bytes
                return 0xff;
        }
        return SYSTEM_ERROR_NONE;
    }

    /**
     * @brief write the register value
     * @return error code from Wire.endTransmission()
     */
    uint8_t writeRegister(uint8_t _value)
    {
        uint8_t transmitCode = SYSTEM_ERROR_NONE;
        // lock for thread safety
        WITH_LOCK(Wire)
        {
            Wire.beginTransmission(Fi2cAddress);
            Wire.write(_value);
            transmitCode = Wire.endTransmission();
        }
        return transmitCode;
    }

    /**
     * @brief reads the wiper value
     */
    bool readWiperValue()
    {
        // read values
        uint8_t read;
        uint8_t transmitCode = readRegister(&read);
        if (transmitCode != SYSTEM_ERROR_NONE)
        {
            Log.trace("could not read wiper value");
            return false;
        }

        // update sdds vars
        steps = read;

        // everything in order
        return true;
    }

    /**
     * @brief writes the wiper value
     */
    bool writeWiperValue()
    {
        uint8_t transmitCode = writeRegister(steps.value());
        if (transmitCode != SYSTEM_ERROR_NONE)
        {
            Log.trace("could not write wiper value %d", steps.value());
            return false;
        }

        // read back modes to check if they match
        uint8_t read;
        transmitCode = readRegister(&read);
        if (transmitCode != SYSTEM_ERROR_NONE)
        {
            Log.trace("could not read wiper value");
            return false;
        }

        // then compare
        if (read != steps)
        {
            // config doesn't match! something must have not worked during the transmission
            Log.trace("wiper value does not match - expected: %d, received: %d", steps.value(), read);
            return false;
        }

        // everything in order
        return true;
    }

    // I2C read function
    virtual bool read() override
    {
        if (status != enums::TconStatus::connected && !connect())
            return false;
        if (!readWiperValue())
            return false;
        return true;
    }

    // I2C write function
    virtual bool write() override
    {
        if (status != enums::TconStatus::connected && !connect())
            return false;
        if (!writeWiperValue())
            return false;
        return true;
    }

public:
    // sdds vars
    sdds_var(Tuint8, steps, sdds::opt::nothing, 0);
    sdds_var(Tuint32, resistance_Ohm, sdds::opt::readonly, 0);

    // public vars (no need to be sdds vars)
    dtypes::uint8 maxSteps = 127;
    dtypes::uint32 maxResistance_Ohm = 0; // maximal resistance (in Ohm)

    // constructor
    ThardwareRheostat()
    {
        on(steps)
        {
            if (steps > maxSteps)
                steps = maxSteps;
            else
                resistance_Ohm = static_cast<dtypes::uint32>(round(static_cast<float>(steps.value()) * maxResistance_Ohm / maxSteps));
        };
    }

    // default i2c address in init
    void init(Resistance _resistance, uint8_t _i2cAddress)
    {
        maxResistance_Ohm = static_cast<dtypes::uint32>(_resistance);
        ThardwareI2C::init(_i2cAddress);
    }
};