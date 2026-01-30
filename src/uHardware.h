#pragma once

#include "uTypedef.h"
#include "enums.h"
#include "uMultask.h"
#ifdef SDDS_ON_PARTICLE
#include "uParticleSystem.h"
#endif

// hardware parts
#include "uHardwareMotorNidec24H.h"
#include "uHardwareIOExpanderTCA9534.h"
#include "uHardwareRheostatMCP4652.h"
#include "uHardwareRheostatMCP4017.h"
#include "uHardwareRheostatAD5246.h"
#include "uHardwarePwmPCA9633.h"
#include "uHardwareSensorOPT101.h"
#include "uDisplay.h"

// hardware constants
#define MICROLOGGER_SIGNAL_PIN A1
#define MICROLOGGER_SPEED_PIN A2
#define MICROLOGGER_DECODER_PIN D6
#define MICROLOGGER_CONTROLLER_VERSION_PIN1 D2
#define MICROLOGGER_CONTROLLER_VERSION_PIN2 D3
#define MICROLOGGER_CONTROLLER_VERSION_PIN3 D4

/**
 * @brief hardware handler
 * uses a TmenuHandle for the convenience of sdds_vars but is not usually
 * included in the tree except for stand-alone testing
 */
class Thardware : public TmenuHandle
{

public:
    // enumerations
    using Ti2cAction = ThardwareI2C::Taction;
    using Ti2cError = ThardwareI2C::Terror;

    using TioMode = ThardwareIOExpander::Tmode;
    using TioValue = ThardwareIOExpander::Tvalue;

    using TbeamState = ThardwareIOExpander::Tmode;
    using TbeamValue = ThardwareIOExpander::Tvalue;
    using TlightValue = ThardwarePwmPCA9633::Tvalue;
    using TfanValue = ThardwarePwmPCA9633::Tvalue;

    using TmotorError = ThardwareMotorNidec24H::Terror;
    using TsignalError = ThardwareSensorOPT101::Terror;

private:
    // is the hardware initialized?
    bool Finitialized = false;

    // in case it is used in a tree
    Tmeta meta() override { return Tmeta{TYPE_ID, 0, "HARDWARE"}; }

    // get the version of the sensor board
    uint8_t getSensorBoardVersion()
    {
        expander.pin6 = TioMode::INPUT;
        expander.pin7 = TioMode::INPUT;
        expander.pin8 = TioMode::INPUT;
        expander.action = Ti2cAction::read;
        if (expander.error == Ti2cError::none)
        {
            uint8_t b0 = expander.value6 == TioValue::HIGH ? 1 : 0;
            uint8_t b1 = expander.value7 == TioValue::HIGH ? 1 : 0;
            uint8_t b2 = expander.value8 == TioValue::HIGH ? 1 : 0;
            uint8_t version = (b2 << 2) | (b1 << 1) | b0;
            return version + 1;
        }
        // error
        return 0;
    }

public:
    // pcb versions
    class Tpcbs : public TmenuHandle
    {
    public:
        sdds_var(Tuint8, controller, sdds::opt::readonly);
        sdds_var(Tuint8, sensor, sdds::opt::readonly);
    };
    sdds_var(Tpcbs, pcbVersions);
    // display and io expander
    sdds_var(Tdisplay, display);
    sdds_var(ThardwareIOExpander, expander);
    // digital pots
    sdds_var(ThardwareRheostatMCP4017, dpot1);
    sdds_var(ThardwareRheostatAD5246, dpot2);
    // overall gain
    class Tgain : public TmenuHandle
    {
    public:
        sdds_var(Tuint32, base_Ohm, sdds::opt::saveval, 10000); // base resistance in the amplification ciruct
        sdds_var(Tuint16, steps);                               // gain steps
        sdds_var(Tuint32, total_Ohm, sdds::opt::readonly);      // total gain of the amplifier circuit
        sdds_var(Ti2cError, error, sdds::opt::readonly);        // i2c error
    };
    sdds_var(Tgain, gain);
    // signal
    sdds_var(ThardwareSensorOPT101, signal);
    // dimmer and motor
    sdds_var(ThardwarePwmPCA9633, dimmer);
    sdds_var(ThardwareMotorNidec24H, motor);

    // aliases
    decltype(expander.pin2) &i2cState = expander.pin2;
    decltype(expander.value2) &i2cValue = expander.value2;

    decltype(expander.error) &beamError = expander.error;
    decltype(expander.action) &beamAction = expander.action;
    decltype(expander.pin1) &beamState = expander.pin1;
    decltype(expander.value1) &beamValue = expander.value1;

    decltype(dimmer.action) &fanLightAction = dimmer.action;
    decltype(dimmer.error) &fanLightError = dimmer.error;
    decltype(dimmer.status) &fanLightStatus = dimmer.status;

    decltype(dimmer.state1) &lightState = dimmer.state1;
    decltype(dimmer.value1) &lightValue = dimmer.value1;
    decltype(dimmer.setpoint1) &lightSetpoint = dimmer.setpoint1;

    decltype(dimmer.state2) &fanState = dimmer.state2;
    decltype(dimmer.value2) &fanValue = dimmer.value2;
    decltype(dimmer.setpoint2) &fanSetpoint = dimmer.setpoint2;

    // set circuit gain (by resistance)
    void setGain(dtypes::uint32 _ohm)
    {
        dtypes::uint16 _steps;
        dtypes::uint16 maxSteps = dpot1.maxSteps + dpot2.maxSteps - 1; // one step fewer as they are always both set (not independently)
        dtypes::uint32 maxGain = gain.base_Ohm.value() + dpot1.maxResistance_Ohm + dpot2.maxResistance_Ohm;
        // figure out steps for requested gain
        if (gain.base_Ohm.value() > _ohm)
            _steps = 0; // requested gain lower than minimum
        else if (_ohm > maxGain)
            _steps = maxSteps; // requested gain higher than maximum
        else
        {
            _steps = static_cast<dtypes::uint16>(round(static_cast<double>((_ohm - gain.base_Ohm.value())) * maxSteps / (dpot1.maxResistance_Ohm + dpot2.maxResistance_Ohm)));
        }
        setGainSteps(_steps);
    }

    // set circuit gain (by steps)
    void setGainSteps(dtypes::uint16 _steps)
    {
        // figure out individual digipots for requested gain
        dtypes::uint16 maxSteps = dpot1.maxSteps + dpot2.maxSteps - 1; // one step fewer as they are always both set (not independently)
        dtypes::uint16 dpot1_steps, dpot2_steps;
        if (_steps > dpot1.maxSteps)
        {
            dpot1_steps = dpot1.maxSteps;
            dpot2_steps = _steps - dpot1.maxSteps + 1; // above 0
        }
        else
        {
            dpot1_steps = _steps;
            dpot2_steps = 0;
        }
        // set steps via I2C
        dpot1.steps = dpot1_steps;
        dpot1.action = ThardwareI2C::Taction::write;
        dpot2.steps = dpot2_steps;
        dpot2.action = ThardwareI2C::Taction::write;
        // reset signal stats
        signal.reset();
        // update error information
        if (dpot1.error != Ti2cError::none && gain.error != dpot1.error)
            gain.error = dpot1.error;
        else if (dpot2.error != Ti2cError::none && gain.error != dpot2.error)
            gain.error = dpot2.error;
        else if (gain.error != Ti2cError::none)
            gain.error = Ti2cError::none;
        // update steps and total gain
        if (gain.error != Ti2cError::none)
        {
            // assign same value to trigger update
            gain.total_Ohm = gain.total_Ohm.value();
        }
        else
        {
            // no error
            if (gain.steps != _steps)
                gain.steps = _steps;
            gain.total_Ohm = gain.base_Ohm.value() + dpot1.resistance_Ohm + dpot2.resistance_Ohm;
        }
    }

    // automatically determine gain based on target signal
    void autoGain()
    {
    }

    // set beam
    void setBeam(enums::ToffOn::e _state)
    {
        if (_state == enums::ToffOn::off && beamValue != TbeamValue::OFF)
        {
            // beam off
            beamState = TbeamState::OUTPUT_OFF;
            beamAction = Ti2cAction::write;
        }
        else if (_state == enums::ToffOn::on && beamValue != TbeamValue::ON)
        {
            // beam on
            beamState = TbeamState::OUTPUT_ON;
            beamAction = Ti2cAction::write;
        }
    }

    // set light by percentage
    void setLight(enums::ToffOn::e _state, uint8_t _percent = 100)
    {

        if ((_state == enums::ToffOn::off || _percent == 0) && lightValue != TlightValue::OFF)
        {
            // full off
            lightState = enums::ToffOn::off;
            fanLightAction = Ti2cAction::write;
        }
        else if (_state == enums::ToffOn::on && _percent == 100 && lightValue != TlightValue::ON)
        {
            // full on
            lightState = enums::ToffOn::on;
            lightSetpoint = ThardwarePwmPCA9633::MAX;
            fanLightAction = Ti2cAction::write;
        }
        else if (_state == enums::ToffOn::on && _percent < 100)
        {
            // dimmed
            dtypes::uint8 setpoint = static_cast<dtypes::uint8>(round(static_cast<dtypes::float32>(_percent) * ThardwarePwmPCA9633::MAX / 100.));
            if (lightValue != TlightValue::DIMMED || lightSetpoint != setpoint)
            {
                lightState = enums::ToffOn::on;
                lightSetpoint = setpoint;
                fanLightAction = Ti2cAction::write;
            }
        }
    }

    // set fan
    void setFan(enums::ToffOn::e _state)
    {

        if (_state == enums::ToffOn::off && fanValue != TfanValue::OFF)
        {
            // fan off
            fanState = enums::ToffOn::off;
            fanLightAction = Ti2cAction::write;
        }
        else if (_state == enums::ToffOn::on && fanValue != TfanValue::ON)
        {
            // full on
            fanState = enums::ToffOn::on;
            fanSetpoint = ThardwarePwmPCA9633::MAX;
            fanLightAction = Ti2cAction::write;
        }
    }

    // constructor
    Thardware()
    {

        // define pins on setup
        on(sdds::setup())
        {

            pinMode(MICROLOGGER_CONTROLLER_VERSION_PIN1, INPUT);
            pinMode(MICROLOGGER_CONTROLLER_VERSION_PIN2, INPUT);
            pinMode(MICROLOGGER_CONTROLLER_VERSION_PIN3, INPUT);
            pinMode(MICROLOGGER_SIGNAL_PIN, INPUT);
            pinMode(MICROLOGGER_SPEED_PIN, OUTPUT);
            pinMode(MICROLOGGER_SPEED_PIN, INPUT);
        };

        // initialize hardware components on particle startup
        on(particleSystem().startup)
        {
            if (particleSystem().startup == TparticleSystem::TstartupStatus::complete)
            {
                if (Finitialized)
                    return;

                // hardware components
                display.init(particleSystem().version.value());
                expander.init();
                dpot1.init(ThardwareRheostatMCP4017::Resistance::R100k);
                dpot2.init(ThardwareRheostatAD5246::Resistance::R100k);
                dimmer.init(ThardwarePwmPCA9633::Driver::EXTN);
                motor.init(MICROLOGGER_SPEED_PIN, MICROLOGGER_DECODER_PIN);
                signal.init(MICROLOGGER_SIGNAL_PIN);

                // controller board version
                uint8_t b0 = digitalRead(MICROLOGGER_CONTROLLER_VERSION_PIN1) ? 1 : 0;
                uint8_t b1 = digitalRead(MICROLOGGER_CONTROLLER_VERSION_PIN2) ? 1 : 0;
                uint8_t b2 = digitalRead(MICROLOGGER_CONTROLLER_VERSION_PIN1) ? 1 : 0;
                uint8_t version = (b2 << 2) | (b1 << 1) | b0;
                pcbVersions.controller = version + 1;

                // fully initialized
                Finitialized = true;
            }
        };

        // turn the gpio expander connection autocheck on to keep track of device connection
        expander.autoConnect = enums::ToffOn::on;
        beamState = TioMode::OUTPUT_OFF; // beam LED
        i2cState = TioMode::OUTPUT_ON;   // indicator LED

        // deal with i2c connection from auto-connecting expander
        on(expander.status)
        {
            // are we connected?
            if (expander.status == enums::TconStatus::connected)
            {
                // turn on connection indicator LED
                i2cState = Thardware::TioMode::OUTPUT_ON;
                expander.action = Thardware::Ti2cAction::write;

                // make sure to write configuration for dimmer and dpots on connect
                fanLightAction = Ti2cAction::write;
                dpot1.action = Ti2cAction::write;
                dpot2.action = Ti2cAction::write;

                // figure out sensor board version on connect
                uint8_t version = getSensorBoardVersion();
                if (pcbVersions.sensor != version)
                    pcbVersions.sensor = version;
            }
            else
            {
                // flag other i2c devices as disconnected when I2C disconnects
                if (fanLightStatus == enums::TconStatus::connected)
                {
                    fanLightAction = Ti2cAction::disconnect;
                    dpot1.action = Ti2cAction::disconnect;
                    dpot2.action = Ti2cAction::disconnect;
                }
            }
        };

        // adjust gain
        on(gain.steps)
        {
            setGainSteps(gain.steps.value());
        };
    }
};

/**
 * @brief get the static hadware instance
 */
Thardware &hardware()
{
    static Thardware hardware;
    return hardware;
}