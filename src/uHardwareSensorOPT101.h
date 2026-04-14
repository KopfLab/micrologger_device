#pragma once

#include "uTypedef.h"
#include "Particle.h"
#include "uRunningStats.h"
#include "enums.h"
#include "stats.h"

// OPT101 light sensor
class ThardwareSensorOPT101 : public TmenuHandle
{

public:
    // enumerations
    sdds_enum(none, saturated) Terror;
    sdds_enum(mean, mode) TsignalStats; // how to calculate signal

    // constants
    const static dtypes::uint32 adcResolution = 4095; // 12-bit adc
    const static dtypes::uint16 maxReads = 100;       // averaging
    const static dtypes::uint16 nModeBoostrap = 25;   // how many times to boostrap the standard deviation and peak when using mode TpeakCalculation

private:
    // analog signal pit
    dtypes::uint8 FsignalPin;

    // signal stats
    TrunningStats FsignalStats;
    TpeakStats<adcResolution, maxReads, nModeBoostrap> FpeakStats;

    // timers
    Ttimer FreadTimer;

public:
    // sdds vars
    sdds_var(enums::ToffOn, state);
    sdds_var(Tuint16, interval_ms, sdds::opt::saveval, 10);                                                    // how many ms between reads
    sdds_var(Tuint16, reads, sdds::opt::saveval, 50);                                                          // how many reads to average across
    sdds_var(TsignalStats, calculation, sdds::opt::saveval);                                                   // how to calculate averages and standard deviations
    sdds_var(Tuint16, maxValue, sdds::opt::saveval, static_cast<dtypes::uint16>(round(0.95 * adcResolution))); // what is considered the maximum value before it's considered saturated? (0.95 % of the adc resolution)
    sdds_var(Tuint16, value, sdds::opt::readonly, Tfloat32::nan());                                            // read signal
    sdds_var(Tuint16, sdev, sdds::opt::readonly, Tfloat32::nan());                                             // stdev
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

        // limit reads range
        on(reads)
        {
            if (reads < 1)
                reads = 1;
            else if (reads > maxReads)
                reads = maxReads;
        };

        // read signal
        on(FreadTimer)
        {
            bool newValues = false;
            dtypes::uint16 mean = 0;
            dtypes::uint16 sd = 0;
            if (calculation == TsignalStats::mean)
            {
                // normal/gaussian peaks --> calculate mean
                FsignalStats.add(analogRead(FsignalPin));

                if (FsignalStats.count() >= reads)
                {
                    newValues = true;
                    mean = static_cast<dtypes::uint16>(round(FsignalStats.mean()));
                    sd = static_cast<dtypes::uint16>(round(FsignalStats.stdDev()));
                    FsignalStats.reset();
                }
            }
            else if (calculation == TsignalStats::mode)
            {
                // skewed peak --> calculate mode
                FpeakStats.add(analogRead(FsignalPin));
                if (FpeakStats.count() >= reads)
                {
                    TkdeMode result;
                    if (FpeakStats.calculate(result))
                    {
                        newValues = true;
                        mean = static_cast<dtypes::uint16>(round(result.peak));
                        sd = static_cast<dtypes::uint16>(round(result.sd));
                    }
                    FpeakStats.reset();
                }
            }

            // assign
            if (newValues)
            {
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
                sdev = sd;
                value = mean;
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