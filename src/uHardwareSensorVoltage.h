#pragma once

#include "uTypedef.h"
#include "Particle.h"
#include "uRunningStats.h"
#include "enums.h"

// voltage sensor by simple voltage divider
class ThardwareSensorVoltage : public TmenuHandle
{

public:
    // constants
    const static dtypes::uint16 adcResolution = 4095; // 12-bit adc

private:
    // information
    bool Finitialized = false;
    dtypes::uint8 FsignalPin;
    dtypes::float32 FrefVolage;
    dtypes::float32 Fresistor1;
    dtypes::float32 Fresistor2;
    dtypes::float32 FvoltageDrop;

    // signal stats
    TrunningStats FvoltageStats;

    // timers
    Ttimer FreadTimer;

public:
    // sdds vars
    sdds_var(enums::ToffOn, state);
    sdds_var(Tuint16, interval_ms, sdds::opt::saveval, 200);         // how many ms between reads
    sdds_var(Tuint16, reads, sdds::opt::saveval, 5);                 // how many reads to average across
    sdds_var(Tfloat32, value, sdds::opt::readonly, Tfloat32::nan()); // read signal
    sdds_var(Tfloat32, sdev, sdds::opt::readonly, Tfloat32::nan());  // stdev

    // constructor
    ThardwareSensorVoltage()
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
            FvoltageStats.reset();
        };

        // read signal
        on(FreadTimer)
        {
            if (!Finitialized)
            {
                FreadTimer.start(interval_ms);
                return;
            }

            uint16_t pinValue = analogRead(FsignalPin);
            // voltage: V = pinValue * Vref/4095 * (R1 + R2)/R2 + Vdrop (e.g. from diode)
            dtypes::float32 voltage = static_cast<dtypes::float64>(pinValue) * FrefVolage / adcResolution * (Fresistor1 + Fresistor2) / Fresistor2 + FvoltageDrop;
            FvoltageStats.add(voltage);
            if (FvoltageStats.count() >= reads)
            {
                value = FvoltageStats.mean();
                sdev = FvoltageStats.stdDev();
                FvoltageStats.reset();
            }
            FreadTimer.start(interval_ms);
        };
    }

    // init with signal pin
    // voltage: V = pinValue * Vref/4095 * (R1 + R2)/R2 + Vdrop (e.g. from diode)
    // make sure refVoltage and voltageDrop are the same units and resistor1 and resistor2 are the same units
    void init(const dtypes::uint8 _signalPin, const dtypes::float32 _refVoltage, const dtypes::float32 _resistor1, const dtypes::float32 _resistor2, const dtypes::float32 _voltageDrop = 0)
    {
        FsignalPin = _signalPin;
        FrefVolage = _refVoltage;
        Fresistor1 = _resistor1;
        Fresistor2 = _resistor2;
        FvoltageDrop = _voltageDrop;
        pinMode(FsignalPin, INPUT);
        Finitialized = TRUE;
    }

    // reset current running stats
    void reset()
    {
        FvoltageStats.reset();
    }
};