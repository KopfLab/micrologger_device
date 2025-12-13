#pragma once

#include "uTypedef.h"
#include "Particle.h"

// IO expander: TCA9534 8-channel multiplexer with 1 wire (I2C) commms
#include "uHardwareI2C.h"
class ThardwareIOExpander : public ThardwareI2C
{

private:
    // registers
    const static byte FpinModesRegister = 0x03;  // pin config register
    const static byte FpinModesInitial = 0xFF;   // see spec sheet
    const static byte FpinValuesRegister = 0x01; // pin state register
    const static byte FpinValuesInitial = 0xFF;  // see spec sheet

    // low level interactions with the chip via I2C

    /**
     * @brief read a register value
     * @return error code from Wire.endTransmission() or custom error code 0xff if byte request failed
     */
    byte readRegister(byte _register, byte *_value)
    {
        byte transmitCode;
        // lock for thread safety
        WITH_LOCK(Wire)
        {
            Wire.beginTransmission(Fi2cAddress);
            Wire.write(_register); // flag register for reading
            // false = keep connection alive instead of stop
            transmitCode = Wire.endTransmission(false);

            // if it worked, request the byte data
            if (transmitCode == SYSTEM_ERROR_NONE)
            {
                uint8_t request = Wire.requestFrom(Fi2cAddress, 1);
                if (request == 1)
                {
                    *_value = Wire.read();
                }
                else
                {
                    // custom error: did not receive the expected 1 byte
                    transmitCode = 0xff;
                }
            }
        }
        return SYSTEM_ERROR_NONE;
    }

    /**
     * @brief write a register value
     * @return error code from Wire.endTransmission()
     */
    byte writeRegister(byte _register, byte _value)
    {
        byte transmitCode;
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

    // WRITE PIN MODES AND VALUES

    /**
     * @brief write pin modes
     */
    bool writePinModes(bool _alwaysWrite = false, bool _check = true)
    {
        if (!_alwaysWrite && pinModes == pinModesTarget)
        {
            // nothing to do
            return true;
        }

        // check connection
        if (!isConnected())
        {
            if (!connect())
                return false;
        }

        // configure pin inputs/outputs
        byte transmitCode = writeRegister(FpinModesRegister, pinModesTarget);
        if (transmitCode != SYSTEM_ERROR_NONE)
        {
            modeErrors++;
            Log.trace("could not transmit IOExpander pin modes %s", byteBits(pinModesTarget).c_str());
            error = Terror::failedWrite;
            return false;
        }
        modeWrites++;

        // if we don't check we can finish
        if (!_check)
            return true;

        // check if modes are as expected -> read first
        if (!readPinModes())
        {
            return false;
        }

        // then compare
        if (pinModes != pinModesTarget)
        {
            // config doesn't match!
            modeErrors++;
            Log.trace("IOExpander pin modes do not match - expected: %s, received: %s", byteBits(pinModesTarget).c_str(), byteBits(pinModes).c_str());
            error = Terror::failedCheck;
            return false;
        }

        // everything in order
        return true;
    }

    /**
     * @brief update pin values
     */
    bool writePinValues(bool _alwaysWrite = false, bool _check = true)
    {
        // only compare values for pins that are ouput (doesn't make sense to include input)
        if (!_alwaysWrite && (pinValues & ~pinModes) == (pinValuesTarget & ~pinModes))
        {
            // nothing to do
            return true;
        }

        // check connection
        if (!isConnected())
        {
            if (!connect())
                return false;
        }

        // configure pin inputs/outputs
        byte transmitCode = writeRegister(FpinValuesRegister, pinValuesTarget);
        if (transmitCode != SYSTEM_ERROR_NONE)
        {
            valueErrors++;
            Log.trace("could not transmit IOExpander pin values %s", byteBits(pinValuesTarget & ~pinModes).c_str());
            error = Terror::failedWrite;
            return false;
        }
        valueWrites++;

        // if we don't check we can finish
        if (!_check)
            return true;

        // check if values are as expected -> read first
        if (!readPinValues())
        {
            return false;
        }

        // check back if values are as expected
        byte read;
        transmitCode = readRegister(FpinValuesRegister, &read);
        pinValues = read;
        if (transmitCode != SYSTEM_ERROR_NONE)
        {
            valueErrors++;
            Log.trace("could not read IOExpander pin values");
            error = Terror::failedRead;
            return false;
        }

        // then compare
        if ((pinValues & ~pinModes) != (pinValuesTarget & ~pinModes))
        {
            // config doesn't match!
            Log.trace("IOExpander pin values do not match - expected: %s, received: %s",
                      byteBits(pinValuesTarget & ~pinModes).c_str(), byteBits(pinValues & ~pinModes).c_str());
            error = Terror::failedCheck;
            return false;
        }

        // everything in order
        return true;
    }

protected:
    // reset registers
    virtual void reset() override
    {
        pinModes = FpinModesInitial;
        pinValues = FpinValuesInitial;
    }

public:
    // sdds enums
    sdds_enum(PIN1, PIN2, PIN3, PIN4, PIN5, PIN6, PIN7, PIN8) Tpin;
    sdds_enum(UNKNOWN, INPUT, OUTPUT) Tmode;
    sdds_enum(UNKNOWN, ON, OFF) Tvalue;

    // sdds vars
    sdds_var(Tuint8, pinModes, sdds_joinOpt(sdds::opt::readonly, sdds::opt::showHex), FpinModesInitial);
    sdds_var(Tuint8, pinModesTarget, sdds_joinOpt(sdds::opt::readonly, sdds::opt::showHex), FpinModesInitial);
    sdds_var(Tuint32, modeReads, sdds::opt::readonly, 0);
    sdds_var(Tuint32, modeWrites, sdds::opt::readonly, 0);
    sdds_var(Tuint32, modeErrors, sdds::opt::readonly, 0);
    sdds_var(Tuint8, pinValues, sdds_joinOpt(sdds::opt::readonly, sdds::opt::showHex), FpinValuesInitial);
    sdds_var(Tuint8, pinValuesTarget, sdds_joinOpt(sdds::opt::readonly, sdds::opt::showHex), FpinValuesInitial);
    sdds_var(Tuint32, valueReads, sdds::opt::readonly, 0);
    sdds_var(Tuint32, valueWrites, sdds::opt::readonly, 0);
    sdds_var(Tuint32, valueErrors, sdds::opt::readonly, 0);
    sdds_var(Tpin, pin);
    sdds_var(Tmode, mode);
    sdds_var(Tvalue, value);

    // constructor
    ThardwareIOExpander()
    {

        // i2c connection
        on(status)
        {
            if (mode != getPinMode(pin))
                mode = getPinMode(pin);
            if (value != getPinValue(pin))
                value = getPinValue(pin);
        };

        // actions (read or write when connected)
        on(action)
        {
            if (action == Taction::read && isConnected())
            {
                readPinModes();
                readPinValues();
            }
            else if (action == Taction::write && isConnected())
            {
                writePinModes();
                writePinValues();
            }

            if (action != Taction::___)
                action = Taction::___;
        };

        // update the overall pin modes
        on(pinModes)
        {
            if (mode != getPinMode(pin))
                mode = getPinMode(pin);
        };

        // update the overall pin values
        on(pinValues)
        {
            if (value != getPinValue(pin))
                value = getPinValue(pin);
        };

        // switch pins
        on(pin)
        {
            if (mode != getPinMode(pin))
                mode = getPinMode(pin);
            if (value != getPinValue(pin))
                value = getPinValue(pin);
        };

        // switch mode (only when connected)
        on(mode)
        {
            // unknown
            if (!isConnected() && mode == Tmode::UNKNOWN)
                return;

            // connected?
            if (isConnected() && mode != Tmode::UNKNOWN && mode != getPinMode(pin))
            {
                setPinMode(pin, mode);
            }

            // not the same? (avoid callback trigger)
            if (mode != getPinMode(pin))
            {
                mode = getPinMode(pin);
            }

            // read values for input
            if (mode == Tmode::INPUT)
            {
                // technically we should read values here but we want to avoid circular read
                // so skipping this, in practice for user they will call isInputOn() which
                // will read the values
            }
        };

        // switch value (only when connected)
        on(value)
        {
            // unknown
            if (!isConnected() && value == Tvalue::UNKNOWN)
                return;

            // connected?
            if (isConnected() && value != Tvalue::UNKNOWN && value != getPinValue(pin))
            {
                setPinValue(pin, value);
            }

            // not the same? (avoid callback trigger)
            if (mode != getPinMode(pin))
            {
                mode = getPinMode(pin);
            }

            // not the same? (avoid callback trigger)
            if (value != getPinValue(pin))
            {
                value = getPinValue(pin);
            }
        };
    }

    // default i2c address in init
    void init(byte _i2cAddress = 0x20)
    {
        ThardwareI2C::init(_i2cAddress);
    }

    // PIN MODES (OUTPUT vs INPUT)

    /**
     * @brief retrieve pin mode
     */
    Tmode::e getPinMode(Tpin::e _pin)
    {
        if (!isConnected())
            return Tmode::UNKNOWN;
        uint8_t pin = static_cast<uint8_t>(_pin);
        if (pin < 8 && (pinModes >> pin) & 0x01)
        {
            return Tmode::INPUT;
        }
        else
        {
            return Tmode::OUTPUT;
        }
    }

    /**
     * @brief set the mode of a pin
     */
    bool setPinMode(Tpin::e _pin, Tmode::e _mode)
    {
        uint8_t pin = static_cast<uint8_t>(_pin);
        if (pin < 8)
        {
            if (_mode == Tmode::OUTPUT)
            {
                pinModesTarget &= ~(0x01 << pin);
            }
            else if (_mode == Tmode::INPUT)
            {
                pinModesTarget |= (0x01 << pin);
            }
        }

        // write the pin modes
        return writePinModes();
    }

    /**
     * @brief read pin modes via I2C - this is rarely called by the user
     */
    bool readPinModes()
    {
        // check connection
        if (!isConnected())
        {
            if (!connect())
                return false;
        }

        // read modes
        byte read = pinModes;
        byte transmitCode = readRegister(FpinModesRegister, &read);
        if (transmitCode != SYSTEM_ERROR_NONE)
        {
            modeErrors++;
            Log.trace("could not read IOExpander pin modes");
            error = Terror::failedRead;
            return false;
        }
        modeReads++;
        pinModes = read;
        return true;
    }

    // PIN VALUES (ON vs OFF)

    /**
     * @brief get pin value
     */
    Tvalue::e getPinValue(Tpin::e _pin)
    {
        if (!isConnected())
            return Tvalue::UNKNOWN;
        uint8_t pin = static_cast<uint8_t>(_pin);
        if (pin < 8 && (pinValues >> pin) & 0x01)
        {
            return Tvalue::ON;
        }
        else
        {
            return Tvalue::OFF;
        }
    }

    /**
     * @brief set the value of a pin (makes sure the pin is an output pin)
     */
    bool setPinValue(Tpin::e _pin, Tvalue::e _value)
    {
        if (!setPinMode(_pin, Tmode::OUTPUT))
        {
            // failed to confirm/configure the pin as OUTPUT
            return false;
        }

        uint8_t pin = static_cast<uint8_t>(_pin);
        if (pin < 8)
        {
            if (_value == Tvalue::OFF)
            {
                pinValuesTarget &= ~(0x01 << pin);
            }
            else if (_value == Tvalue::ON)
            {
                pinValuesTarget |= (0x01 << pin);
            }
        }

        // update values
        return writePinValues();
    }

    /**
     * @brief read pin values - this is useful if some pins are inputs
     */
    bool readPinValues()
    {
        // check connection
        if (!isConnected())
        {
            if (!connect())
                return false;
        }

        // read modes
        byte read = pinValues;
        byte transmitCode = readRegister(FpinValuesRegister, &read);
        if (transmitCode != SYSTEM_ERROR_NONE)
        {
            valueErrors++;
            Log.trace("could not read IOExpander pin values");
            error = Terror::failedRead;
            return false;
        }
        valueReads++;
        pinValues = read;
        return true;
    }

    // CONVENIENCE FUNCTIONS for external classes

    /**
     * @brief convenience function to turn a pin on (setPinValue makes sure the pin is an output pin)
     */
    bool turnOn(Tpin::e _pin)
    {
        return setPinValue(_pin, Tvalue::ON);
    }

    /**
     * @brief is the pin on? (TRUE only if it's connected, it's an output, and it's ON, otherwise FALSE)
     */
    bool isOutputOn(Tpin::e _pin)
    {
        return isConnected() && getPinMode(_pin) == Tmode::OUTPUT && getPinValue(_pin) == Tvalue::ON;
    }

    /**
     * @brief is input on? (TRUE only if it's connected, it's an input, and it's ON, otherwise FALSE)
     */
    bool isInputOn(Tpin::e _pin)
    {
        readPinValues();
        return isConnected() && getPinMode(_pin) == Tmode::INPUT && getPinValue(_pin) == Tvalue::ON;
    }

    /**
     * @brief convenience function to turn a pin off (setPinValue makes sure the pin is an output pin)
     */
    bool turnOff(Tpin::e _pin)
    {
        return setPinValue(_pin, Tvalue::OFF);
    }
};
