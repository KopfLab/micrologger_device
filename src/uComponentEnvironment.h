#pragma once
#include "uTypedef.h"
#include "uHardware.h"
#include "uParticleSystem.h"

/**
 * @brief environment class, currently only temperature but might get expanded in the future
 */
class TcomponentEnvironment : public TmenuHandle
{

public:
    // enumerations
    using Terror = Thardware::Ti2cError;

private:
    // keep track of publishing to detect when it switches from OFF to ON
    bool FisPublishing = particleSystem().publishing.publish == sdds::enums::OnOff::ON;

    // timers
    Ttimer FreadTimer;

public:
    // sdds vars
    sdds_var(Tuint16, read_sec, sdds::opt::saveval, 1); // how often to read the temperature (in seconds)
    sdds_var(Tfloat32, temperature_C, sdds::opt::readonly);
    sdds_var(Tfloat32, power_V, sdds::opt::readonly);
    sdds_var(Tfloat32, powerReq_V, sdds::opt::saveval, 20.0); // what is the minimum power requirement?
    sdds_var(Thardware::Ti2cError, error, sdds::opt::readonly);

    // constructor
    TcomponentEnvironment()
    {

        // make sure hardware is initalized
        hardware();

        // update from hardware
        on(hardware().temperature.temperature_C)
        {
            if (temperature_C != hardware().temperature.temperature_C)
                temperature_C = hardware().temperature.temperature_C;
        };

        on(hardware().temperature.error)
        {
            if (error != hardware().temperature.error)
                error = hardware().temperature.error;
        };

        on(hardware().voltage.value)
        {
            power_V = hardware().voltage.value;
        };

        // update temperature read timer
        on(FreadTimer)
        {
            hardware().temperature.action = Thardware::Ti2cAction::read;
            FreadTimer.start(read_sec.value() * 1000);
        };
    }

    // pause state if disconnected
    void pauseState()
    {
        if (FreadTimer.running())
            FreadTimer.stop();
    }

    // resume the saved state after power cycling or reconnection
    void resumeState()
    {
        if (!FreadTimer.running())
            FreadTimer.start(0);
    }
};