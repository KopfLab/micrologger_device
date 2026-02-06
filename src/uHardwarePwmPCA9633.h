#pragma once

#include "uTypedef.h"
#include "enums.h"
#include "Particle.h"
#include "uHardwareI2C.h"

// hardware constants
// (to address all TCA9534s, can also use 0x70, for software reset can use 0x03)
#define DIMMER_I2C_ADDRESS 0x62

// PCA9633: 4-channel pwm dimmer with 1 wire (I2C) commms
class ThardwarePwmPCA9633 : public ThardwareI2C
{

public:
    // enumerations
    enum class Driver
    {
        DIRECT,
        EXTN,
        EXTP
    };
    sdds_enum(UNSET, ON, OFF, DIMMED) Tvalue;

    // 8-bit resolution
    const static uint8_t MAX = 255;

private:
    // relevant registers (see manual Table 7)
    static constexpr uint8_t FregistersN = 9;
    static constexpr uint8_t Fmode1Register = 0;
    static constexpr uint8_t Fmode2Register = 1;
    static constexpr uint8_t Fpwm1Register = 2;
    static constexpr uint8_t Fpwm2Register = 3;
    static constexpr uint8_t Fpwm3Register = 4;
    static constexpr uint8_t Fpwm4Register = 5;
    static constexpr uint8_t FgrpPwmRegister = 6;
    static constexpr uint8_t FgrpFreqRegister = 7;
    static constexpr uint8_t FoutputModeRegister = 8;

    // register autoincrement (see manual Fig. 10, Table 6)
    const static uint8_t FincrementFlagAll = 0x80;

    // values (see manual Tables 8 - 13)
    const static uint8_t Fmode1Default = 0b10010001;
    const static uint8_t Fmode2Default = 0b00000101;
    const static uint8_t FgrpPwmDefault = 0xff;
    const static uint8_t FgrpFreqDefault = 0x00;
    const static uint8_t FoutputModeOff = 0b00;
    const static uint8_t FoutputModeOn = 0b01;
    const static uint8_t FoutputModeDimmed = 0b10;

    // default
    Driver Fdriver = Driver::DIRECT;

    // low level interactions with the chip via I2C

    /**
     * @brief read registers
     * @param _first register to start at
     * @param _n number of register values to read
     * @param _values pointer to array of size _n to read into
     * @return error code from Wire.endTransmission() or custom error code 0xff if not the correct number of values read back
     */
    uint8_t readRegisters(uint8_t _first, uint8_t _n, uint8_t *_values)
    {
        // lock for thread safety
        WITH_LOCK(Wire)
        {
            Wire.beginTransmission(Fi2cAddress);
            // start at first register with autoincrement
            Wire.write(FincrementFlagAll | _first);
            // false = keep connection alive instead of stop
            uint8_t transmitCode = Wire.endTransmission(false);
            if (transmitCode != SYSTEM_ERROR_NONE)
            {
                Log.trace("could not read PwmDimmer register values");
                return transmitCode;
            }

            // request to read all relevant registers
            uint8_t request = Wire.requestFrom(Fi2cAddress, _n);
            if (request != _n)
            {
                Log.trace("could not read the expected number of PwmDimmer register values");
                return 0xff;
            }

            // read the values
            for (int i = 0; i < _n; i++)
            {
                if (!Wire.available())
                {
                    Log.trace("could not read the expected number of PwmDimmer register values");
                    return 0xff;
                }
                _values[i] = Wire.read();
            }
        }
        return SYSTEM_ERROR_NONE;
    }

    /**
     * @brief write registers
     * @param _first register to start at
     * @param _n number of register values to write
     * @param _values pointer to array of size _n to write from
     * @return error code from Wire.endTransmission()
     */
    uint8_t writeRegisters(uint8_t _first, uint8_t _n, const uint8_t *_values)
    {
        // lock for thread safety
        WITH_LOCK(Wire)
        {
            // transmit configuration
            // control: start at _first, auto-increment from there (see Fig. 10, Table 6 & 7 in datasheet)
            Wire.beginTransmission(Fi2cAddress);
            Wire.write(FincrementFlagAll | _first);
            for (uint8_t i = 0; i < _n; i++)
            {
                Wire.write(_values[i]);
            }
            uint8_t transmitCode = Wire.endTransmission();
            if (transmitCode != SYSTEM_ERROR_NONE)
            {
                Log.trace("could not write PwmDimmer register values");
                return transmitCode;
            }
        }
        return SYSTEM_ERROR_NONE;
    }

    /**
     * @brief write the relevant registers (first 9)
     * @return error code from Wire.endTransmission() or custom error code 0xff if uint8_t request failed
     */
    bool writeConfiguration()
    {

        // registers to write
        uint8_t regs[FregistersN];
        regs[Fmode1Register] = Fmode1Default;
        regs[Fmode1Register] &= ~(1 << 4); // switch bit 4 off to switch from sleep to normal mode (Table 8)
        regs[Fmode2Register] = Fmode2Default;
        if (Fdriver == Driver::EXTN)
            regs[Fmode2Register] |= (1 << 4); // switch bit 4 on to invert output logic for NMOS drivers (Table 9 & 17), DIRECT and PMOS don't need this
        regs[Fpwm1Register] = setpoint1.value();
        regs[Fpwm2Register] = setpoint2.value();
        regs[Fpwm3Register] = setpoint3.value();
        regs[Fpwm4Register] = setpoint4.value();
        regs[FgrpPwmRegister] = FgrpPwmDefault;
        regs[FgrpFreqRegister] = FgrpFreqDefault;
        regs[FoutputModeRegister] = 0;

        // output mode
        uint8_t outputModes[4] = {
            getOutputMode(&state1, &setpoint1),
            getOutputMode(&state2, &setpoint2),
            getOutputMode(&state3, &setpoint3),
            getOutputMode(&state4, &setpoint4)};
        regs[FoutputModeRegister] |= (outputModes[0] << 0); // state1 goes into bits 1:0
        regs[FoutputModeRegister] |= (outputModes[1] << 2); // state2 goes into bits 3:2
        regs[FoutputModeRegister] |= (outputModes[2] << 4); // state3 goes into bits 5:4
        regs[FoutputModeRegister] |= (outputModes[3] << 6); // state4 goes into bits 7:6

        // write the registers
        if (writeRegisters(Fmode1Register, FregistersN, regs) != SYSTEM_ERROR_NONE)
            return false;

        // read them back
        uint8_t reads[FregistersN];
        if (readRegisters(Fmode1Register, FregistersN, reads) != SYSTEM_ERROR_NONE)
            return false;

        // compare the values
        for (uint8_t i = 0; i < FregistersN; i++)
        {
            if (reads[i] != regs[i])
            {
                Log.trace("PwmDimmer register %d value does not match - expected: %s, received: %s",
                          i, byteBits(regs[i]).c_str(), byteBits(reads[i]).c_str());
                return false;
            }
        }

        // update sdds vars
        setValue(&setpoint1, &state1, &value1);
        setValue(&setpoint2, &state2, &value2);
        setValue(&setpoint3, &state3, &value3);
        setValue(&setpoint4, &state4, &value4);

        // done
        return true;
    }

    /**
     * @brief get the output mode uint8_t code for a pin state and setpoint
     */
    uint8_t getOutputMode(enums::ToffOn *_state, Tuint8 *_setpoint)
    {
        if (*_state == enums::ToffOn::off || _setpoint->value() == 0)
            return FoutputModeOff;
        else if (*_state == enums::ToffOn::on && _setpoint->value() == MAX)
            return FoutputModeOn;
        else
            return FoutputModeDimmed;
    }

    // set sdds value
    void setValue(Tuint8 *_setpoint, enums::ToffOn *_state, Tvalue *_value)
    {
        if ((*_state == enums::ToffOn::off || _setpoint->value() == 0) && *_value != Tvalue::OFF)
            *_value = Tvalue::OFF;
        else if (*_state == enums::ToffOn::on && _setpoint->value() >= MAX && *_value != Tvalue::ON)
            *_value = Tvalue::ON;
        else if (*_state == enums::ToffOn::on && _setpoint->value() < MAX && *_value != Tvalue::DIMMED)
            *_value = Tvalue::DIMMED;
    }

    // reset sdds values after disconnect
    void resetValue(Tvalue *_value)
    {
        if (*_value != Tvalue::UNSET)
            *_value = Tvalue::UNSET;
    }

protected:
    // I2C write function
    virtual bool write() override
    {
        if (status != enums::TconStatus::connected && !connect())
            return false;
        if (!writeConfiguration())
            return false;
        return true;
    }

public:
    // sdds variables
    sdds_var(Tuint8, setpoint1, sdds::opt::nothing, 255);
    sdds_var(enums::ToffOn, state1);
    sdds_var(Tvalue, value1, sdds::opt::readonly);
    sdds_var(Tuint8, setpoint2, sdds::opt::nothing, 255);
    sdds_var(enums::ToffOn, state2);
    sdds_var(Tvalue, value2, sdds::opt::readonly);
    sdds_var(Tuint8, setpoint3, sdds::opt::nothing, 255);
    sdds_var(enums::ToffOn, state3);
    sdds_var(Tvalue, value3, sdds::opt::readonly);
    sdds_var(Tuint8, setpoint4, sdds::opt::nothing, 255);
    sdds_var(enums::ToffOn, state4);
    sdds_var(Tvalue, value4, sdds::opt::readonly);

    // constructor
    ThardwarePwmPCA9633()
    {

        // connection status
        on(status)
        {
            if (status == enums::TconStatus::disconnected)
            {
                // disconnected
                resetValue(&value1);
                resetValue(&value2);
                resetValue(&value3);
                resetValue(&value4);
            }
        };
    }

    // default i2c address in init
    void init(Driver _driver, uint8_t _i2cAddress = DIMMER_I2C_ADDRESS)
    {
        Fdriver = _driver;
        ThardwareI2C::init(_i2cAddress);
        // make sure to write configuration right away
        action = ThardwareI2C::Taction::write;
    }
};
