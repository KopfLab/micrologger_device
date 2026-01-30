#pragma once

#include "uTypedef.h"
#include "Particle.h"
#include "uRunningStats.h"

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
    sdds_var(Tuint16, interval_ms, sdds::opt::saveval, 5);                                                     // how many ms between reads
    sdds_var(Tuint16, reads, sdds::opt::saveval, 100);                                                         // how many reads to average across
    sdds_var(Tuint16, maxValue, sdds::opt::saveval, static_cast<dtypes::uint16>(round(0.95 * adcResolution))); // what is considered the maximum value before it's considered saturated? (0.95 % of the adc resolution)
    sdds_var(Tuint16, value, sdds::opt::readonly);                                                             // read signal
    sdds_var(Tuint16, sdev, sdds::opt::readonly);                                                              // stdev
    sdds_var(Terror, error, sdds::opt::readonly);                                                              // signal error

    // constructor
    ThardwareSensorOPT101()
    {
        // read signal
        on(FreadTimer)
        {
            FsignalStats.add(analogRead(FsignalPin));
            if (FsignalStats.count() >= reads)
            {
                value = static_cast<dtypes::uint16>(round(FsignalStats.mean()));
                sdev = static_cast<dtypes::uint16>(round(FsignalStats.stdDev()));
                if (value > maxValue)
                {
                    if (error != Terror::saturated)
                        error = Terror::saturated;
                }
                else
                {
                    if (error != Terror::none)
                        error = Terror::none;
                }
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
        FreadTimer.start(interval_ms);
    }

    // reset current running stats
    void reset()
    {
        FsignalStats.reset();
    }
};