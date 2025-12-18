#pragma once

#include "uTypedef.h"
#include "Particle.h"

// stirrer motor
class ThardwareMotor : public TmenuHandle
{

private:
    // initialized
    bool Finitalized = false;

    // speed
    dtypes::TtickCount FspeedStart = 0L;      // when was the speed last set?
    dtypes::uint8 FspeedPin;                  // PWM output pin
    const dtypes::uint16 FspeedFreq = 25000L; // PWM frequency
    Ttimer FspeedCheckTimer;

    // decoder for speed measurement
    dtypes::uint8 FdecoderPin; // digital pin on decoder
    TisrEvent decoderISR;
    dtypes::TtickCount FdecoderStart = 0L; // when was the decoder last set
    dtypes::uint32 FdecoderCounter = 0L;

    // decoder interrupt
    void decoderChangeISR()
    {
        decoderISR.signal();
    };

    // rpm limits (inclusive)
    const dtypes::uint16 FminRpm = 50;
    const dtypes::uint16 FmaxRpm = 5000;

    // resulting step limits (will be updated later)
    dtypes::uint16 FminStep = 0;
    dtypes::uint16 FmaxStep = 4095;

    // step to rpm calibration
    struct Calibration
    {
        dtypes::uint16 stepMin; // inclusive
        dtypes::uint16 stepMax; // exclusive (except last)
        dtypes::float32 rpmMin; // inclusive
        dtypes::float32 rpmMax; // inclusive for last
        dtypes::float32 b;      // intercept_mean  (RPM at step=0 for this calibration)
        dtypes::float32 m;      // slope_mean      (RPM per step)
    };

    // calibration data
    static constexpr Calibration calibrations[4] = {
        // stepMin stepMax   rpmMin   rpmMax    b(intercept)   m(slope)
        {0, 1000, 236.0f, 1352.0f, 111.0f, 0.693f},
        {1000, 2000, 1335.0f, 2846.0f, 118.0f, 0.688f},
        {2000, 3000, 2812.0f, 4307.0f, 75.9f, 0.702f},
        {3000, 4000, 4263.0f, 5415.0f, 123.0f, 0.691f},
    };

    // convert rm to step
    dtypes::uint16 rpmToStep(dtypes::uint16 _rpm, bool _checkLimits = true)
    {
        // deal with limits
        if (_checkLimits && _rpm <= FminRpm)
            return FminStep;
        if (_checkLimits && _rpm >= FmaxRpm)
            return FmaxStep;
        // find calibration
        dtypes::float32 rpm = static_cast<dtypes::float32>(_rpm);
        dtypes::uint8 calib_idx = 0;
        for (; calib_idx < 4; calib_idx++)
        {
            if (rpm <= calibrations[calib_idx].rpmMax)
                break;
        }
        // apply calibration
        return static_cast<dtypes::uint16>(round(calibrations[calib_idx].b + calibrations[calib_idx].m * rpm));
    }

    // convert steps to rpm
    dtypes::uint16 stepToRpm(dtypes::uint16 _step)
    {
        // deal with limits
        if (_step <= FminStep)
            return FminRpm;
        if (_step >= FmaxStep)
            return FmaxRpm;
        // find calibration
        dtypes::uint8 calib_idx = 0;
        for (; calib_idx < 4; calib_idx++)
        {
            if (_step <= calibrations[calib_idx].stepMax)
                break;
        }
        // apply calibration
        return static_cast<dtypes::uint16>(round((static_cast<dtypes::float32>(_step) - calibrations[calib_idx].b) / calibrations[calib_idx].m));
    }

public:
    sdds_var(Tuint16, steps, sdds::opt::nothing, 0);
    sdds_var(Tuint16, targetSpeed, sdds::opt::nothing, 0);
    sdds_var(Tuint16, measuredSpeed, sdds::opt::nothing, 0);
    sdds_var(Tuint16, speedCheckInterval, sdds::opt::nothing, 1000);
    sdds_enum(none, noResponse) Terror;
    sdds_var(Terror, error, sdds::opt::readonly);

    ThardwareMotor()
    {

        FminStep = rpmToStep(FminRpm, false);
        FmaxStep = rpmToStep(FmaxRpm, false);

        on(steps)
        {
            if (steps == 0)
            {
                // turn motor off
                if (Finitalized)
                {
                    analogWrite(FspeedPin, 0, FspeedFreq);
                    FspeedStart = millis();
                }
                if (targetSpeed != 0)
                {
                    // check to aovid trigger loop
                    targetSpeed = 0;
                }
            }
            else if (steps < FminStep)
            {
                // jump to the min step
                steps = FminStep;
            }
            else if (steps > FmaxStep)
            {
                // back down to max step
                steps = FmaxStep;
            }
            else
            {
                // set steps
                if (Finitalized)
                {
                    analogWrite(FspeedPin, steps.Fvalue, FspeedFreq);
                    FspeedStart = millis();
                }
                if (targetSpeed != stepToRpm(steps.Fvalue))
                {
                    // check to aovid trigger loop
                    targetSpeed = stepToRpm(steps.Fvalue);
                }
            }
        };

        // set speed
        on(targetSpeed)
        {
            if (targetSpeed == 0 && steps != 0)
            {
                steps = 0;
            }
            else if (steps != rpmToStep(targetSpeed.Fvalue))
            {
                steps = rpmToStep(targetSpeed.Fvalue);
            }
        };

        // decoder interrupt triggered
        on(decoderISR)
        {
            FdecoderCounter++;
        };

        // check speed timer
        on(FspeedCheckTimer)
        {
            // decoder info: rpm = freq (in Hz) / 100 * 60 = counts / dt.ms * 1000 * 1/100 * 60 = count / dt.ms * 600.
            float rpm = (600. * FdecoderCounter) / (millis() - FdecoderStart);
            measuredSpeed = static_cast<dtypes::uint16>(round(rpm));
            if (steps > 0 && FdecoderCounter < 10 && (millis() - FspeedStart) > 2 * speedCheckInterval)
            {
                // even at 50pm, the FdecoderCounter should be counting at least 80 cts/sec after the second speed check interval
                if (error != Terror::noResponse)
                    // mark as motor not having a response
                    error = Terror::noResponse;
            }
            else if (error != Terror::none)
            {
                // all good
                error = Terror::none;
            }
            resetDecoder();
        };
    }

    void init(dtypes::uint8 _speedPin, dtypes::uint8 _decoderPin)
    {
        // save pins
        FspeedPin = _speedPin;
        FdecoderPin = _decoderPin;

        // set up the motor pin
        pinMode(FspeedPin, OUTPUT);
        analogWriteResolution(FspeedPin, 12);
        analogWrite(FspeedPin, 0, FspeedFreq);

        // attach the encoder interrupt
        pinMode(FdecoderPin, INPUT);
        attachInterrupt(
            digitalPinToInterrupt(FdecoderPin),
            &ThardwareMotor::decoderChangeISR,
            this, RISING);

        // initialization is complete
        Finitalized = true;

        // start speed measurements
        resetDecoder();
    }

    void resetDecoder()
    {
        // reset decoder
        FdecoderStart = millis();
        FdecoderCounter = 0;
        FspeedCheckTimer.start(speedCheckInterval);
    }
};