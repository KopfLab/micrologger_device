#pragma once

#include "uTypedef.h"
#include "enums.h"
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

    // step limits (will be refined based on the max and min RPM sdds variables later)
    dtypes::uint16 FminStep = 0;
    dtypes::uint16 FmaxStep = 4095;

    // step to rpm calibration
    struct Calibration
    {
        dtypes::uint16 stepMin; // inclusive
        dtypes::uint16 stepMax; // exclusive (except last)
        dtypes::float32 rpmMin; // inclusive
        dtypes::float32 rpmMax; // inclusive for last
        dtypes::float32 b;      // intercept_mean  (steps at RPM=0 for this calibration)
        dtypes::float32 m;      // slope_mean      (steps per RPM)
    };

    // calibration data
    static constexpr Calibration calibrations[4] = {
        // stepMin stepMax   rpmMin   rpmMax    b(intercept)   m(slope)
        {0, 1000, 236.0f, 1352.0f, 111.0f, 0.693f},
        {1000, 2000, 1335.0f, 2846.0f, 118.0f, 0.693f},
        {2000, 3000, 2812.0f, 4307.0f, 75.9f, 0.702f},
        {3000, 4000, 4263.0f, 5415.0f, 123.0f, 0.691f},
    };
    // average slope
    static constexpr dtypes::float32 Fmavg = (0.693f + 0.693f + 0.702f + 0.691f) / 4.0f;

    // convert rm to step
    dtypes::uint16 rpmToStep(dtypes::uint16 _rpm, bool _checkLimits = true)
    {
        // deal with limits
        if (_checkLimits && _rpm <= minSpeed_rpm)
            return FminStep;
        if (_checkLimits && _rpm >= maxSpeed_rpm)
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
            return minSpeed_rpm;
        if (_step >= FmaxStep)
            return maxSpeed_rpm;
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
    sdds_var(Tuint16, minSpeed_rpm, sdds::opt::readonly, 50);
    sdds_var(Tuint16, maxSpeed_rpm, sdds::opt::readonly, 5000);
    sdds_var(Tuint16, targetSpeed_rpm, sdds::opt::nothing, 0);
    sdds_var(Tuint16, measuredSpeed_rpm, sdds::opt::readonly, 0);
    sdds_var(Tuint16, targetSteps, sdds::opt::nothing, 0);
    sdds_var(Tuint16, steps, sdds::opt::readonly, 0);
    sdds_var(enums::ToffOn, autoAdjust, sdds::opt::nothing, enums::ToffOn::on);
    // speed tolerance: based on the calibration slopes of ~0.7 steps/rpm --> 1/0.7 = 1.4 rpm/step --> aim for targetSpeed +/- 2)
    sdds_var(Tuint8, autoAdjustSpeedTolerance_rpm, sdds::opt::nothing, static_cast<dtypes::uint8>(ceil(1.0f / Fmavg)));
    sdds_var(Tuint16, speedCheckInterval_ms, sdds::opt::nothing, 1000);
    sdds_var(Tuint32, speedCheckCounter, sdds::opt::nothing, 0);
    sdds_enum(none, noResponse) Terror;
    sdds_var(Terror, error, sdds::opt::readonly);

    ThardwareMotor()
    {

        FminStep = rpmToStep(minSpeed_rpm, false);
        FmaxStep = rpmToStep(maxSpeed_rpm, false);

        // set actual motor steps
        on(steps)
        {
            if (Finitalized)
            {
                if (steps == 0)
                {
                    // we can't tell if there is an error if we're not running
                    error = Terror::none;
                }
                analogWrite(FspeedPin, steps.Fvalue, FspeedFreq);
                FspeedStart = millis();
                speedCheckCounter = 0;
            }
        };

        // set target steps
        on(targetSteps)
        {
            if (targetSteps == 0)
            {
                // turn motor off
                steps = 0;
                if (targetSpeed_rpm != 0)
                {
                    // check to avoid trigger loop
                    targetSpeed_rpm = 0;
                }
            }
            else if (targetSteps < FminStep)
            {
                // jump to the min step
                targetSteps = FminStep;
            }
            else if (targetSteps > FmaxStep)
            {
                // back down to max step
                targetSteps = FmaxStep;
            }
            else
            {
                // set steps
                steps = targetSteps;
                if (targetSpeed_rpm != stepToRpm(targetSteps.Fvalue))
                {
                    // check to aovid trigger loop
                    targetSpeed_rpm = stepToRpm(targetSteps.Fvalue);
                }
            }
        };

        // set speed
        on(targetSpeed_rpm)
        {
            if (targetSpeed_rpm == 0)
            {
                // loop safety check
                if (targetSteps != 0)
                    targetSteps = 0;
            }
            else if (targetSpeed_rpm < minSpeed_rpm)
            {
                // jump to min speed
                targetSpeed_rpm = minSpeed_rpm;
            }
            else if (targetSpeed_rpm > maxSpeed_rpm)
            {
                // jump to max speed
                targetSpeed_rpm = maxSpeed_rpm;
            }
            else if (targetSteps != rpmToStep(targetSpeed_rpm.Fvalue))
            {
                // set target steps
                targetSteps = rpmToStep(targetSpeed_rpm.Fvalue);
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
            measuredSpeed_rpm = static_cast<dtypes::uint16>(round(rpm));
            speedCheckCounter++;
            // are we up and running? (i.e. already at the 2nd speed check and set to something larger than 0)
            if (speedCheckCounter > 1 && targetSteps > 0)
            {
                // are we having a connectivity issue?
                if (FdecoderCounter < 10)
                {
                    // even at 50pm, the FdecoderCounter should be counting at least 80 cts/sec by the 2nd speed check
                    // (1st check can be slow to adjust)
                    if (error != Terror::noResponse)
                        // mark as motor not having a response
                        error = Terror::noResponse;
                }
                else
                {
                    // all good, we're running for real
                    if (error != Terror::none)
                        // mark motor as not having an error
                        error = Terror::none;
                    // now that we're running, do we want to auto-adjust the steps to get closer to the targetSpeed?
                    if (autoAdjust == enums::ToffOn::on &&
                        abs(measuredSpeed_rpm - targetSpeed_rpm) > autoAdjustSpeedTolerance_rpm)
                    {
                        // finetune step adjustments
                        dtypes::uint8 adjustment = static_cast<dtypes::uint8>(round(static_cast<dtypes::float32>(abs(measuredSpeed_rpm - targetSpeed_rpm)) * Fmavg));
                        if (adjustment > 0)
                            steps = steps + adjustment * ((measuredSpeed_rpm < targetSpeed_rpm) ? 1 : -1);
                    }
                }
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
        FspeedCheckTimer.start(speedCheckInterval_ms);
    }
};