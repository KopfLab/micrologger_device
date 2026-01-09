#pragma once

#include "uTypedef.h"
#include "Particle.h"

// IO expander: TCA9534 8-channel multiplexer with 1 wire (I2C) commms
#include "uHardwareI2C.h"
class ThardwareIOExpander : public ThardwareI2C
{

public:
    // enumerations

    // pin modes
    sdds_enum(INPUT, OUTPUT_ON, OUTPUT_OFF) Tmode; // INPUT is the default (consistent with config)

    // pin values
    sdds_enum(UNKNOWN, HIGH, LOW, UNSET, ON, OFF) Tvalue; // UNKNOWN is the default (since input is the default)

private:
    // registers
    static constexpr uint8_t FpinModesRegister = 0x03;     // pin config register
    static constexpr uint8_t FoutputValuesRegister = 0x01; // output pin state register
    static constexpr uint8_t FinputValuesRegister = 0x00;  // input pin state register

    // low level interactions with the chip via I2C

    /**
     * @brief read a register value
     * @return error code from Wire.endTransmission() or custom error code 0xff if uint8_t request failed
     */
    uint8_t readRegister(uint8_t _register, uint8_t *_value)
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
            if (transmitCode == SYSTEM_ERROR_NONE)
            {
                const uint8_t bytesRequested = 1;
                uint8_t request = Wire.requestFrom(Fi2cAddress, bytesRequested);
                if (request == bytesRequested && Wire.available())
                {
                    *_value = Wire.read();
                }
                else
                {
                    // custom error: did not receive the expected number of bytes
                    transmitCode = 0xff;
                }
            }
        }
        return transmitCode;
    }

    /**
     * @brief write a register value
     * @return error code from Wire.endTransmission()
     */
    uint8_t writeRegister(uint8_t _register, uint8_t _value)
    {
        uint8_t transmitCode;
        // lock for thread safety
        WITH_LOCK(Wire)
        {
            Wire.beginTransmission(Fi2cAddress);
            Wire.write(_register);
            Wire.write(_value);
            transmitCode = Wire.endTransmission();
        }
        return transmitCode;
    }

    // next level writing/reading modes

    /**
     * @brief writes pin modes (output/input) and checks if written correctly
     */
    bool writePinModes()
    {
        // configure pin inputs/outputs
        uint8_t modes = bitsToByte(
            pin1 == Tmode::INPUT, pin2 == Tmode::INPUT, pin3 == Tmode::INPUT, pin4 == Tmode::INPUT,
            pin5 == Tmode::INPUT, pin6 == Tmode::INPUT, pin7 == Tmode::INPUT, pin8 == Tmode::INPUT);
        uint8_t transmitCode = writeRegister(FpinModesRegister, modes);
        if (transmitCode != SYSTEM_ERROR_NONE)
        {
            Log.trace("could not transmit IOExpander pin modes %s", byteBits(modes, 'O', 'I').c_str());
            return false;
        }

        // read back modes to check if they match
        uint8_t read;
        transmitCode = readRegister(FpinModesRegister, &read);
        Log.trace("read pin modes: %s", byteBits(read, 'O', 'I').c_str());
        if (transmitCode != SYSTEM_ERROR_NONE)
        {
            Log.trace("could not read IOExpander pin modes");
            return false;
        }

        // then compare
        if (read != modes)
        {
            // config doesn't match! something must have not worked during the transmission
            Log.trace("IOExpander pin modes do not match - expected: %s, received: %s", byteBits(modes, 'O', 'I').c_str(), byteBits(read, 'O', 'I').c_str());
            return false;
        }

        // everything in order
        return true;
    }

    /**
     * @brief updates output pin values (does nothing to input pin values) and checks if written correctly
     */
    bool writePinValues()
    {
        // configure pin values (only matters for output pins)
        uint8_t values = bitsToByte(
            pin1 == Tmode::OUTPUT_ON, pin2 == Tmode::OUTPUT_ON, pin3 == Tmode::OUTPUT_ON, pin4 == Tmode::OUTPUT_ON,
            pin5 == Tmode::OUTPUT_ON, pin6 == Tmode::OUTPUT_ON, pin7 == Tmode::OUTPUT_ON, pin8 == Tmode::OUTPUT_ON);

        // configure pin inputs/outputs
        uint8_t transmitCode = writeRegister(FoutputValuesRegister, values);
        if (transmitCode != SYSTEM_ERROR_NONE)
        {
            Log.trace("could not transmit IOExpander pin values %s", byteBits(values, 'H', 'L').c_str());
            error = Terror::failedWrite;
            return false;
        }

        // check back if values are as expected
        uint8_t read;
        transmitCode = readRegister(FoutputValuesRegister, &read);
        Log.trace("read output pin values: %s", byteBits(read, 'H', 'L').c_str());
        if (transmitCode != SYSTEM_ERROR_NONE)
        {
            Log.trace("could not read IOExpander pin values");
            return false;
        }

        // then compare
        if (read != values)
        {
            // config doesn't match!
            Log.trace("IOExpander pin values do not match - expected: %s, received: %s",
                      byteBits(values, 'H', 'L').c_str(), byteBits(read, 'H', 'L').c_str());
            return false;
        }

        // update sdds vars
        setOutputValue(&pin1, &value1);
        setOutputValue(&pin2, &value2);
        setOutputValue(&pin3, &value3);
        setOutputValue(&pin4, &value4);
        setOutputValue(&pin5, &value5);
        setOutputValue(&pin6, &value6);
        setOutputValue(&pin7, &value7);
        setOutputValue(&pin8, &value8);

        // everything in order
        return true;
    }

    /**
     * @brief reads input pin values (does nothing to output pin values)
     */
    bool readPinValues()
    {
        // configure pin values (only matters for output pins)
        uint8_t values = bitsToByte(
            pin1 == Tmode::OUTPUT_ON, pin2 == Tmode::OUTPUT_ON, pin3 == Tmode::OUTPUT_ON, pin4 == Tmode::OUTPUT_ON,
            pin5 == Tmode::OUTPUT_ON, pin6 == Tmode::OUTPUT_ON, pin7 == Tmode::OUTPUT_ON, pin8 == Tmode::OUTPUT_ON);

        // read values
        uint8_t read;
        uint8_t transmitCode = readRegister(FinputValuesRegister, &read);
        Log.trace("read input pin values: %s", byteBits(read, 'H', 'L').c_str());
        if (transmitCode != SYSTEM_ERROR_NONE)
        {
            Log.trace("could not read IOExpander pin values");
            return false;
        }

        // update sdds vars
        setInputValue(&pin1, &value1, isBitSet(read, 0));
        setInputValue(&pin2, &value2, isBitSet(read, 1));
        setInputValue(&pin3, &value3, isBitSet(read, 2));
        setInputValue(&pin4, &value4, isBitSet(read, 3));
        setInputValue(&pin5, &value5, isBitSet(read, 4));
        setInputValue(&pin6, &value6, isBitSet(read, 5));
        setInputValue(&pin7, &value7, isBitSet(read, 6));
        setInputValue(&pin8, &value8, isBitSet(read, 7));

        // everything in order
        return true;
    }

    // I2C read function
    virtual bool read() override
    {
        if (status != Tstatus::connected && !connect())
            return false;
        if (!readPinValues())
            return false;
        return true;
    }

    // I2C write function
    virtual bool write() override
    {
        if (status != Tstatus::connected && !connect())
            return false;
        if (!writePinModes())
            return false;
        if (!writePinValues())
            return false;
        return true;
    }

    // set sdds value for outputs
    void setOutputValue(Tmode *_mode, Tvalue *_value)
    {
        if (*_mode == Tmode::OUTPUT_ON && *_value != Tvalue::ON)
            *_value = Tvalue::ON;
        else if (*_mode == Tmode::OUTPUT_OFF && *_value != Tvalue::OFF)
            *_value = Tvalue::OFF;
    }

    // set sdds value for inputs
    void setInputValue(Tmode *_mode, Tvalue *_value, bool _read)
    {
        if (*_mode == Tmode::INPUT)
        {
            if (_read && *_value != Tvalue::HIGH)
                *_value = Tvalue::HIGH;
            else if (!_read && *_value != Tvalue::LOW)
                *_value = Tvalue::LOW;
        }
    }

    // reset sdds values after pin mode change or disconnect
    void resetValue(Tmode *_mode, Tvalue *_value)
    {
        if (*_mode == Tmode::INPUT && *_value != Tvalue::UNKNOWN)
            *_value = Tvalue::UNKNOWN; // input
        else if (*_mode != Tmode::INPUT && *_value != Tvalue::UNSET)
            *_value = Tvalue::UNSET; // ouput
    }

public:
    // sdds vars
    sdds_var(Tmode, pin1);
    sdds_var(Tvalue, value1, sdds::opt::readonly);
    sdds_var(Tmode, pin2);
    sdds_var(Tvalue, value2, sdds::opt::readonly);
    sdds_var(Tmode, pin3);
    sdds_var(Tvalue, value3, sdds::opt::readonly);
    sdds_var(Tmode, pin4);
    sdds_var(Tvalue, value4, sdds::opt::readonly);
    sdds_var(Tmode, pin5);
    sdds_var(Tvalue, value5, sdds::opt::readonly);
    sdds_var(Tmode, pin6);
    sdds_var(Tvalue, value6, sdds::opt::readonly);
    sdds_var(Tmode, pin7);
    sdds_var(Tvalue, value7, sdds::opt::readonly);
    sdds_var(Tmode, pin8);
    sdds_var(Tvalue, value8, sdds::opt::readonly);

    // constructor
    ThardwareIOExpander()
    {

        // change modes
        on(pin1) { resetValue(&pin1, &value1); };
        on(pin2) { resetValue(&pin2, &value2); };
        on(pin3) { resetValue(&pin3, &value3); };
        on(pin4) { resetValue(&pin4, &value4); };
        on(pin5) { resetValue(&pin5, &value5); };
        on(pin6) { resetValue(&pin6, &value6); };
        on(pin7) { resetValue(&pin7, &value7); };
        on(pin8) { resetValue(&pin8, &value8); };

        // connection status
        on(status)
        {
            if (status == Tstatus::disconnected)
            {
                // disconnected
                resetValue(&pin1, &value1);
                resetValue(&pin2, &value2);
                resetValue(&pin3, &value3);
                resetValue(&pin4, &value4);
                resetValue(&pin5, &value5);
                resetValue(&pin6, &value6);
                resetValue(&pin7, &value7);
                resetValue(&pin8, &value8);
            }
        };
    }

    // default i2c address in init
    void init(uint8_t _i2cAddress = 0x20)
    {
        ThardwareI2C::init(_i2cAddress);
    }
};
