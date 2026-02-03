#pragma once

#include "uTypedef.h"
#include "Particle.h"
#include "uRunningStats.h"
#include "enums.h"

// OPT101 light sensor
class ThardwareSensorOPT101 : public TmenuHandle
{

public:
    // enumerations
    sdds_enum(none, saturated) Terror;

    // constants
    const static dtypes::uint16 adcResolution = 4095; // 12-bit adc

private:
    // analog signal pit
    dtypes::uint8 FsignalPin;

    // signal stats
    TrunningStats FsignalStats;

    // timers
    Ttimer FreadTimer;

public:
    // sdds vars
    sdds_var(enums::ToffOn, state);
    sdds_var(Tuint16, interval_ms, sdds::opt::saveval, 10);                                                    // how many ms between reads
    sdds_var(Tuint16, reads, sdds::opt::saveval, 50);                                                          // how many reads to average across
    sdds_var(Tuint16, maxValue, sdds::opt::saveval, static_cast<dtypes::uint16>(round(0.95 * adcResolution))); // what is considered the maximum value before it's considered saturated? (0.95 % of the adc resolution)
    sdds_var(Tuint16, value, sdds::opt::readonly);                                                             // read signal
    sdds_var(Tuint16, sdev, sdds::opt::readonly);                                                              // stdev
    sdds_var(Terror, error, sdds::opt::readonly);                                                              // signal error

    // constructor
    ThardwareSensorOPT101()
    {
        // active?
        on(state)
        {
            if (state == enums::ToffOn::on && !FreadTimer.running())
            {
                FreadTimer.start(0);
            }
            else if (state == enums::ToffOn::off && FreadTimer.running())
            {
                FreadTimer.stop();
            }
            FsignalStats.reset();
        };

        // read signal
        on(FreadTimer)
        {
            FsignalStats.add(analogRead(FsignalPin));
            if (FsignalStats.count() >= reads)
            {
                dtypes::uint16 mean = static_cast<dtypes::uint16>(round(FsignalStats.mean()));
                if (mean > maxValue)
                {
                    if (error != Terror::saturated)
                        error = Terror::saturated;
                }
                else
                {
                    if (error != Terror::none)
                        error = Terror::none;
                }
                value = mean;
                sdev = static_cast<dtypes::uint16>(round(FsignalStats.stdDev()));
                FsignalStats.reset();
            }
            FreadTimer.start(interval_ms);
        };
    }

    // init with signal pin
    void init(dtypes::uint8 _signalPin)
    {
        FsignalPin = _signalPin;
        pinMode(FsignalPin, INPUT);
    }

    // reset current running stats
    void reset()
    {
        FsignalStats.reset();
    }
};