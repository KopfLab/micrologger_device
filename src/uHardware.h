#pragma once

#include "uTypedef.h"
#include "uMultask.h"
#ifdef SDDS_ON_PARTICLE
#include "uParticleSystem.h"
#endif

// hardware parts
#include "uHardwareMotorNidec24H.h"
#include "uHardwareIOExpanderTCA9534.h"
#include "uHardwareRheostatMCP4652.h"
#include "uHardwarePwmPCA9633.h"

/**
 * @brief hardware handler
 * uses a TmenuHandle for the convenience of sdds_vars but is not usually
 * included in the tree except for stand-alone testing
 */
class Thardware : public TmenuHandle
{

public:
    // enumerations
    using TonOff = sdds::enums::OnOff;
    using Ti2cStatus = ThardwareIOExpander::Tstatus;
    using Ti2cAction = ThardwareI2C::Taction;
    using TioMode = ThardwareIOExpander::Tmode;
    using TioValue = ThardwareIOExpander::Tvalue;
    using TdimmerValue = ThardwarePwmPCA9633::Tvalue;

private:
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
            motor.init(A2, D6); // on particle, might be different on other devices!
            expander.init();
            dpot.init(ThardwareRheostatMCP4652::Resolution::S256, ThardwareRheostatMCP4652::Resistance::R100k);
            dimmer.init(ThardwarePwmPCA9633::Driver::EXTN);
        }
    }

public:
    // hardware components
    sdds_var(Ti2cStatus, device, sdds::opt::readonly, Ti2cStatus::disconnected);
    sdds_var(TonOff, beam, sdds::opt::nothing, TonOff::e::OFF);
    sdds_var(TonOff, lights, sdds::opt::nothing, TonOff::e::OFF);
    sdds_var(Tuint8, intensity, sdds::opt::nothing, 0);
    sdds_var(TonOff, fan, sdds::opt::nothing, TonOff::e::OFF);
    sdds_var(ThardwareMotor, motor);
    sdds_var(ThardwareIOExpander, expander);
    sdds_var(ThardwareRheostatMCP4652, dpot);
    sdds_var(ThardwarePwmPCA9633, dimmer);

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
        expander.autoConnect = TonOff::ON;
        expander.pin1 = TioMode::OUTPUT_ON;  // indicator light
        expander.pin2 = TioMode::OUTPUT_OFF; // beam LED

        // check expander I2C status
        on(expander.status)
        {
            // if connected and not on yet, turn on
            if (expander.status == Ti2cStatus::connected && expander.value1 != TioValue::ON)
            {
                expander.pin1 = TioMode::OUTPUT_ON;
                expander.action = Ti2cAction::write;
            }
        };

        // keep I2C status updated
        on(expander.value1)
        {
            if (expander.value1 == TioValue::ON && device == Ti2cStatus::disconnected)
                device = Ti2cStatus::connected;
            else if (expander.value1 != TioValue::ON && device == Ti2cStatus::connected)
                device = Ti2cStatus::disconnected;
        };

        // turn beam on/off
        on(beam)
        {
            if (beam == TonOff::OFF && expander.value2 == TioValue::ON)
            {
                expander.pin2 = TioMode::OUTPUT_OFF;
                expander.action = Ti2cAction::write;
            }
            else if (beam == TonOff::ON && expander.value2 != TioValue::ON)
            {
                expander.pin2 = TioMode::OUTPUT_ON;
                expander.action = Ti2cAction::write;
            }
        };

        // keep beam status updated
        on(expander.value2)
        {
            if (expander.value2 == TioValue::ON && beam == TonOff::OFF)
                beam = TonOff::ON;
            else if (expander.value2 != TioValue::ON && beam == TonOff::ON)
                beam = TonOff::OFF;
        };

        // dimmer I2C connection
        on(device)
        {
            if (device == Ti2cStatus::connected)
            {
                // make sure to write configuration whenever I2C connects
                dimmer.action = Ti2cAction::write;
            }
            else if (device == Ti2cStatus::disconnected && dimmer.status == Ti2cStatus::connected)
            {
                // flag dimmer as disconnected when I2C disconnects
                dimmer.action = Ti2cAction::disconnect;
            }
        };

        // turn lights on/off / dim lights
        on(lights)
        {
            dtypes::uint8 setpoint = static_cast<dtypes::uint8>(round(static_cast<dtypes::float32>(intensity.value()) * ThardwarePwmPCA9633::MAX / 100.));
            if ((lights == TonOff::OFF || intensity == 0) && dimmer.value1 != TdimmerValue::OFF)
            {
                dimmer.state1 = TonOff::OFF;
                dimmer.action = Ti2cAction::write;
            }
            else if (lights == TonOff::ON && intensity == 100 && dimmer.value1 != TdimmerValue::ON)
            {
                dimmer.setpoint1 = ThardwarePwmPCA9633::MAX;
                dimmer.state1 = TonOff::ON;
                dimmer.action = Ti2cAction::write;
            }
            else if (lights == TonOff::ON && intensity < 100 && (dimmer.value1 != TdimmerValue::DIMMED || dimmer.setpoint1 != setpoint))
            {
                dimmer.setpoint1 = setpoint;
                dimmer.state1 = TonOff::ON;
                dimmer.action = Ti2cAction::write;
            }
        };

        // keep lights status updated
        on(dimmer.value1)
        {
            if ((dimmer.value1 == TdimmerValue::ON || dimmer.value1 == TdimmerValue::DIMMED) && lights == TonOff::OFF)
                lights = TonOff::ON;
            else if ((dimmer.value1 != TdimmerValue::ON && dimmer.value1 != TdimmerValue::DIMMED) && lights == TonOff::ON)
                lights = TonOff::OFF;
        };

        // change light intensity
        on(intensity)
        {
            if (intensity == 0)
                intensity = 1;
            else if (intensity > 100)
                intensity = 100;
            else
                lights.signalEvents();
        };
        intensity = 100;

        // turn fan on/off
        // FIXME: double check this works with disconnects/reconnects
        on(fan)
        {
            if (fan == TonOff::OFF && dimmer.value2 != TdimmerValue::OFF)
            {
                dimmer.state2 = TonOff::OFF;
                dimmer.action = Ti2cAction::write;
            }
            else if (fan == TonOff::ON && dimmer.value2 != TdimmerValue::ON)
            {
                dimmer.setpoint2 = ThardwarePwmPCA9633::MAX;
                dimmer.state2 = TonOff::ON;
                dimmer.action = Ti2cAction::write;
            }
        };

        // keep fan status updated
        // FIXME: double check this works with disconnects/reconnects
        on(dimmer.value2)
        {
            if (dimmer.value2 == TdimmerValue::ON && fan == TonOff::OFF)
                fan = TonOff::ON;
            else if (dimmer.value2 != TdimmerValue::ON && fan == TonOff::ON)
                fan = TonOff::OFF;
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