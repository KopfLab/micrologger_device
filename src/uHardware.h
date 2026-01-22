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
#include "uHardwarePwmPCA9633.h"
#include "uDisplay.h"

// hardware constants
#define MICROLOGGER_SIGNAL_PIN A1
#define MICROLOGGER_SPEED_PIN A2
#define MICROLOGGER_DECODER_PIN D6

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

    using TlightValue = ThardwarePwmPCA9633::Tvalue;
    using TfanValue = ThardwarePwmPCA9633::Tvalue;

    using TmotorError = ThardwareMotor::Terror;

private:
    // is the hardware initialized?
    bool Finitialized = false;

    // in case it is used in a tree
    Tmeta meta() override { return Tmeta{TYPE_ID, 0, "hardware"}; }

    /**
     * @brief initialize the hardware
     */
    void init()
    {
        if (!Finitialized)
        {
            display.init(particleSystem().version.value());
            expander.init();
            //  dpot.init(ThardwareRheostatMCP4652::Resolution::S256, ThardwareRheostatMCP4652::Resistance::R100k);
            dimmer.init(ThardwarePwmPCA9633::Driver::EXTN);
            motor.init(MICROLOGGER_SPEED_PIN, MICROLOGGER_DECODER_PIN);
            Finitialized = true;
        }
    }

public:
    // hardware components
    sdds_var(Tdisplay, display);
    sdds_var(ThardwareIOExpander, expander);
    sdds_var(ThardwareRheostatMCP4652, dpot);
    sdds_var(ThardwarePwmPCA9633, dimmer);
    sdds_var(ThardwareMotor, motor);

    // aliases
    decltype(expander.value1) &i2cValue = expander.value1;

    decltype(expander.action) &beamAction = expander.action;
    decltype(expander.pin2) &beamState = expander.pin2;
    decltype(expander.value2) &beamValue = expander.value2;

    decltype(dimmer.action) &fanLightAction = dimmer.action;
    decltype(dimmer.error) &fanLightError = dimmer.error;
    decltype(dimmer.status) &fanLightStatus = dimmer.status;

    decltype(dimmer.state1) &lightState = dimmer.state1;
    decltype(dimmer.value1) &lightValue = dimmer.value1;
    decltype(dimmer.setpoint1) &lightSetpoint = dimmer.setpoint1;

    decltype(dimmer.state2) &fanState = dimmer.state2;
    decltype(dimmer.value2) &fanValue = dimmer.value2;
    decltype(dimmer.setpoint2) &fanSetpoint = dimmer.setpoint2;

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

    // read signal
    dtypes::int32 readSignal()
    {
        return analogRead(MICROLOGGER_SIGNAL_PIN);
    }

    // constructor
    Thardware()
    {

#ifdef SDDS_ON_PARTICLE
        // if on a particle system, initialize once startup is complete
        on(particleSystem().startup)
        {
            if (particleSystem().startup == TparticleSystem::TstartupStatus::complete)
            {
                init();
            }
        };
#else
        // initialize during setup
        on(sdds::setup())
        {
            init();
        };
#endif

        // turn the gpio expander connection autocheck on to keep track of device connection
        expander.autoConnect = enums::ToffOn::on;
        expander.pin1 = TioMode::OUTPUT_ON;  // indicator light
        expander.pin2 = TioMode::OUTPUT_OFF; // beam LED

        // deal with i2c connection from auto-connecting expander
        on(expander.status)
        {
            // are we connected?
            if (expander.status == enums::TconStatus::connected)
            {
                // turn on connection indicator LED
                expander.pin1 = Thardware::TioMode::OUTPUT_ON;
                expander.action = Thardware::Ti2cAction::write;

                // make sure to write configuration for dimmer on connect
                dimmer.action = Ti2cAction::write;
            }
            else
            {
                // flag dimmer as disconnected when I2C disconnects
                if (dimmer.status == enums::TconStatus::connected)
                    dimmer.action = Ti2cAction::disconnect;
            }
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