#pragma once
#include "uTypedef.h"
#include "uCoreEnums.h"
#include "enums.h"
#include "uRunningStats.h"
#include "uHardware.h"
#include "uComponentStirrer.h"
#include "uComponentLights.h"
#include "uParticleSystem.h"

/**
 * @brief stirrer class
 */
class TcomponentOpticalDensity : public TmenuHandle
{

public:
    // enumerations
    sdds_enum(___, zero, beamOn, beamOff, optimizeGain, reset) Taction;
    sdds_enum(idle, reading, optimizing, zeroing) Tstatus;
    sdds_enum(none, saturated, failedGain, failedZero, failedRead) Terror;

private:
    // keep track of publishing to detect when it switches from OFF to ON
    bool FisPublishing = particleSystem().publishing.publish == sdds::enums::OnOff::ON;

    // other components that need to be paused by this one
    TcomponentStirrer *Fstirrer = nullptr;
    TcomponentLights *Flights = nullptr;

    // timers
    Ttimer FreadTimer;
    Ttimer FinfoTimer;
    Ttimer FwaitTimer;
    Ttimer FwarmupTimer;
    system_tick_t FnextRead = 0;

    // temporary values for gain adjustment
    bool FbeamWasOn = false;
    dtypes::uint16 FlastSignal = 0;
    dtypes::uint16 FtargetSignal = 0;
    dtypes::uint16 FsignalDifference = 0;
    dtypes::uint16 FlastSteps = 0;
    enum TgainAdjustmentStages
    {
        GAIN_IDLE,
        GAIN_START,
        GAIN_VORTEX,
        GAIN_STIR_STOP,
        GAIN_WAIT,
        GAIN_WARMUP,
        READ_NO_GAIN,
        READ_INITIAL_GAIN,
        FINE_ADJUSTMENT
    } FgainAdjustment;
    const dtypes::uint8 FinitialGainSteps = 5;

    // temporary values for reading values
    enum TreadingStages
    {
        READ_IDLE,
        READ_START,
        READ_VORTEX,
        READ_STIR_STOP,
        READ_WAIT,
        READ_WARMUP,
        READ_SIGNAL,
        READ_COOLDOWN,
        READ_DARK,
        ZERO_WAIT_FOR_GAIN
    } Freading;

    // conversion functions from signal to parts per thousand and back
    dtypes::uint16 signalToPpt(dtypes::uint16 _signal)
    {
        return static_cast<dtypes::uint16>(round(_signal * 1000. / hardware().signal.adcResolution));
    }
    dtypes::uint16 pptToSignal(dtypes::uint16 _ppt)
    {
        return static_cast<dtypes::uint16>(round(_ppt * hardware().signal.adcResolution / 1000.));
    }

    // process signal based on the status
    void process(bool _stageDone = false)
    {

        if (status == Tstatus::reading || status == Tstatus::zeroing)
        {
            // read signal sequence (light and dark)
            bool Ferror = beam.status == Tbeam::Tstatus::error || (status == Tstatus::reading && zero.valid == enums::TnoYes::no);
            switch (Freading)
            {
            case TreadingStages::READ_START:
                if (Ferror)
                    break;
                // zeroing invalidates the last zero
                if (status == Tstatus::zeroing && zero.valid != enums::TnoYes::no)
                    zero.valid = enums::TnoYes::no;

                // reading.vortex if it's supposed to reading.vortex
                if (Fstirrer && reading.vortex == enums::TnoYes::yes)
                {
                    Freading = TreadingStages::READ_VORTEX;
                    (*Fstirrer).action = TcomponentStirrer::Taction::vortex;
                    break;
                }
            case TreadingStages::READ_VORTEX:
                // still waiting on this stage to get completed?
                if (Ferror || (Freading != TreadingStages::READ_START && !_stageDone))
                    break;

                // turn stirrer off if it's supposed to go off and isn't currently
                if (Fstirrer && reading.stopStirrer == enums::TnoYes::yes && (*Fstirrer).status != TcomponentStirrer::Tstatus::off)
                {
                    Freading = TreadingStages::READ_STIR_STOP;
                    (*Fstirrer).action = TcomponentStirrer::Taction::pause;
                    break;
                }
            case TreadingStages::READ_STIR_STOP:
                // still waiting on this stage to get completed?
                if (Ferror || (Freading != TreadingStages::READ_START && !_stageDone))
                    break;

                // wait to let the liquid settle after stirring/reading.vortexing
                if (Fstirrer && (reading.vortex == enums::TnoYes::yes || reading.stopStirrer == enums::TnoYes::yes))
                {
                    Freading = TreadingStages::READ_WAIT;
                    FwaitTimer.start(reading.wait_ms);
                    break;
                }
            case TreadingStages::READ_WAIT:
                // still waiting on this stage to get completed?
                if (Ferror || (Freading != TreadingStages::READ_START && !_stageDone))
                    break;

                // pause lights
                (*Flights).action = TcomponentLights::Taction::pause;

                // turn beam on if not already on and wait for warmup period
                hardware().setBeam(enums::ToffOn::on);
                Freading = TreadingStages::READ_WARMUP;
                FwarmupTimer.start(reading.warmup_ms);
                break;
            case TreadingStages::READ_WARMUP:
                // still waiting on this stage to get completed?
                if (Ferror || (Freading != TreadingStages::READ_START && !_stageDone))
                    break;

                // record signal
                Freading = TreadingStages::READ_SIGNAL;
                hardware().recordSignal(enums::ToffOn::on);
                break;
            case TreadingStages::READ_SIGNAL: // first read signal stage
                if (Ferror)
                    break;
                // save as zeroing value?
                if (status == Tstatus::zeroing)
                    zero.signal = hardware().signal.value.value();
                else
                    reading.signal = hardware().signal.value.value();

                // saturated?
                if (hardware().signal.error == Thardware::TsignalError::saturated && error == Terror::none)
                    error = Terror::saturated;
                else if (hardware().signal.error == Thardware::TsignalError::none && error == Terror::saturated)
                    error = Terror::none;

                hardware().setBeam(enums::ToffOn::off); // beam back off
                Freading = TreadingStages::READ_COOLDOWN;
                FwarmupTimer.start(reading.warmup_ms);
                break;
            case TreadingStages::READ_COOLDOWN:
                // still waiting on this stage to get completed?
                if (Ferror || (Freading != TreadingStages::READ_START && !_stageDone))
                    break;
                // record dark
                Freading = TreadingStages::READ_DARK;
                break;
            case TreadingStages::READ_DARK: // second read signal stage
                if (Ferror)
                    break;
                if (status == Tstatus::zeroing)
                {
                    // finish zero
                    zero.bgrd = hardware().signal.value.value();
                    if (zero.bgrd.value() > zero.signal.value())
                    {
                        // bgrd is larger than signal! something is wrong
                        Ferror = true;
                        break;
                    }
                    reading.bgrd = zero.bgrd;
                    reading.signal = zero.signal;
                    zero.last = Time.format(Time.now(), TIME_FORMAT_ISO8601_FULL);
                    zero.valid = enums::TnoYes::yes;
                }
                else
                {
                    // finish read
                    reading.bgrd = hardware().signal.value.value();
                    if (reading.bgrd > reading.signal)
                        reading.signal = reading.bgrd; // = 0 transmittance
                    hardware().recordSignal(enums::ToffOn::off);
                }
                // finish calculations and return to idle
                if (reading.signal == reading.bgrd)
                {
                    reading.transmittance = 0.0;  // no signal left
                    reading.OD = Tfloat32::nan(); // not a number
                }
                else
                {
                    reading.transmittance = static_cast<dtypes::float32>((reading.signal.value() - reading.bgrd.value())) / (zero.signal.value() - zero.bgrd.value());
                    reading.OD = -log10(reading.transmittance.value());
                }
                if (error != Terror::none && error != Terror::saturated)
                    error = Terror::none;
                status = Tstatus::idle;
                break;
            }

            // act if there were errors
            if (Ferror)
            {
                if (status == Tstatus::zeroing && error != Terror::failedZero)
                    error = Terror::failedZero;
                else if (status == Tstatus::reading && error != Terror::failedRead)
                    error = Terror::failedRead;
                status = Tstatus::idle;
            }
        }
        else if (status == Tstatus::optimizing)
        {
            // in gain adjustment
            bool Ferror = gain.status == Tgain::Tstatus::error || beam.status == Tbeam::Tstatus::error;
            dtypes::float64 slope;  // init before switch
            dtypes::uint16 newGain; // init before switch

            // gain state machine switch
            switch (FgainAdjustment)
            {
            case TgainAdjustmentStages::GAIN_START:
                if (Ferror)
                    break;
                // gain adjustment invalidates the last zero
                if (zero.valid != enums::TnoYes::no)
                    zero.valid = enums::TnoYes::no;

                // reading.vortex if it's supposed to reading.vortex
                if (Fstirrer && reading.vortex == enums::TnoYes::yes)
                {
                    FgainAdjustment = TgainAdjustmentStages::GAIN_VORTEX;
                    (*Fstirrer).action = TcomponentStirrer::Taction::vortex;
                    break;
                }
            case TgainAdjustmentStages::GAIN_VORTEX:
                // still waiting on this stage to get completed?
                if (Ferror || (FgainAdjustment != TgainAdjustmentStages::GAIN_START && !_stageDone))
                    break;

                // turn stirrer off if it's supposed to go off and isn't currently
                if (Fstirrer && reading.stopStirrer == enums::TnoYes::yes && (*Fstirrer).status != TcomponentStirrer::Tstatus::off)
                {
                    FgainAdjustment = TgainAdjustmentStages::GAIN_STIR_STOP;
                    (*Fstirrer).action = TcomponentStirrer::Taction::pause;
                    break;
                }
            case TgainAdjustmentStages::GAIN_STIR_STOP:
                // still waiting on this stage to get completed?
                if (Ferror || (FgainAdjustment != TgainAdjustmentStages::GAIN_START && !_stageDone))
                    break;

                // wait to let the liquid settle after stirring/reading.vortexing
                if (Fstirrer && (reading.vortex == enums::TnoYes::yes || reading.stopStirrer == enums::TnoYes::yes))
                {
                    FgainAdjustment = TgainAdjustmentStages::GAIN_WAIT;
                    FwaitTimer.start(reading.wait_ms);
                    break;
                }
            case TgainAdjustmentStages::GAIN_WAIT:
                // still waiting on this stage to get completed?
                if (Ferror || (FgainAdjustment != TgainAdjustmentStages::GAIN_START && !_stageDone))
                    break;

                // set target for gain adjumsnet
                FtargetSignal = pptToSignal(gain.target_ppt.value());

                // put gain to 0
                hardware().setGainSteps(0);

                // pause lights
                (*Flights).action = TcomponentLights::Taction::pause;

                // turn beam on if not already on and wait for warmup period
                FbeamWasOn = beam.status == Tbeam::Tstatus::on;
                if (!FbeamWasOn)
                {
                    hardware().setBeam(enums::ToffOn::on);
                    FgainAdjustment = TgainAdjustmentStages::GAIN_WARMUP;
                    FwarmupTimer.start(reading.warmup_ms);
                    break;
                }
            case TgainAdjustmentStages::GAIN_WARMUP:
                // still waiting on this stage to get completed?
                if (Ferror || (FgainAdjustment != TgainAdjustmentStages::GAIN_START && !_stageDone))
                    break;

                // reduce number of reads for quick gain adjustment
                hardware().recordSignal(enums::ToffOn::on);
                FgainAdjustment = TgainAdjustmentStages::READ_NO_GAIN;
                break;
            case TgainAdjustmentStages::READ_NO_GAIN: // first read signal stage
                if (Ferror)
                    break;
                FlastSignal = hardware().signal.value.value();
                // is the zero signal higher than the target? --> no way we can adjust to reach the garget
                if (FlastSignal > FtargetSignal)
                    Ferror = true;
                FlastSteps = FinitialGainSteps;
                hardware().setGainSteps(FlastSteps);
                FgainAdjustment = TgainAdjustmentStages::READ_INITIAL_GAIN;
                break;
            case TgainAdjustmentStages::READ_INITIAL_GAIN: // second read signal stage
                if (Ferror)
                    break;
                // was there signal increase with the higher gain? if not --> error
                if (FlastSignal > hardware().signal.value.value())
                {
                    Ferror = true;
                    break;
                }
                // calculate slope (gain / step)
                slope = static_cast<dtypes::float64>(hardware().signal.value.value() - FlastSignal) / FinitialGainSteps;
                // cadjust gain to reach target approximately
                newGain = static_cast<dtypes::uint16>(round((FtargetSignal - FlastSignal) / slope));
                // update last signal for fine-tuning
                FgainAdjustment = TgainAdjustmentStages::FINE_ADJUSTMENT;
                FlastSignal = hardware().signal.value.value();
                FsignalDifference = abs(FtargetSignal - FlastSignal);
                hardware().setGainSteps(newGain);
                break;
            case TgainAdjustmentStages::FINE_ADJUSTMENT: // last read signal stage
                if (Ferror)
                    break;
                dtypes::uint16 newDiff = abs(FtargetSignal - hardware().signal.value.value());
                if (newDiff > FsignalDifference)
                {
                    // previous setting was better, keep that one and FINISH
                    error = Terror::none;
                    if (!FbeamWasOn)
                        hardware().setBeam(enums::ToffOn::off);
                    FgainAdjustment = TgainAdjustmentStages::GAIN_IDLE;
                    // should zeroing start now?
                    if (Freading == TreadingStages::ZERO_WAIT_FOR_GAIN)
                    {
                        // yes zero
                        status = Tstatus::zeroing;
                        Freading = TreadingStages::READ_START;
                    }
                    else
                    {
                        // no just go back to idle
                        status = Tstatus::idle;
                    }
                    // set gain as last step (so the new resistance is registered)
                    hardware().setGainSteps(FlastSteps);
                }
                else
                {
                    // new setting is better --> repeat fine adjustment
                    FlastSignal = hardware().signal.value.value();
                    FlastSteps = hardware().gain.steps.value();
                    FsignalDifference = newDiff;
                    hardware().setGainSteps(FlastSteps + ((FtargetSignal > FlastSignal) ? +1 : -1));
                }
                break;
            }

            // act if there were errors
            if (Ferror)
            {
                if (error != Terror::failedGain)
                    error = Terror::failedGain;
                status = Tstatus::idle;
            }
            hardware().resetSignal();
        }
    }

public:
    // sdds variables
    sdds_var(Taction, action);                      // what to do
    sdds_var(Tstatus, status, sdds::opt::readonly); // what's happening

    // sdds variable for latest values
    class Treading : public TmenuHandle
    {
    public:
        sdds_var(Tuint16, saturation_ppt, sdds::opt::readonly);                       // current signal saturation in parts per thousand
        sdds_var(Tuint16, readInterval_ms, sdds::opt::saveval, 60000);                // how often to read
        sdds_var(enums::TnoYes, vortex, sdds::opt::saveval, enums::TnoYes::no);       // reading.vortex before reading?
        sdds_var(enums::TnoYes, stopStirrer, sdds::opt::saveval, enums::TnoYes::yes); // stop stirrer before read?
        sdds_var(Tuint16, wait_ms, sdds::opt::saveval, 1000);                         // how long to wait after reading.vortex/stirrer stop
        sdds_var(Tuint16, warmup_ms, sdds::opt::saveval, 200);                        // beam warmup (how long to wait whenever the beam is turned on)
        sdds_var(Tstring, nextRead, sdds::opt::readonly);
        sdds_var(Tuint16, signal, sdds::opt::readonly, 0);
        sdds_var(Tuint16, bgrd, sdds::opt::readonly, 0);
        sdds_var(Tfloat32, transmittance, sdds::opt::readonly, Tfloat32::nan());
        sdds_var(Tfloat32, OD, sdds::opt::readonly, Tfloat32::nan());
    };
    sdds_var(Treading, reading);

    // sdds variable for zeroing
    class Tzero : public TmenuHandle
    {
    public:
        sdds_var(Tstring, last, sdds_joinOpt(sdds::opt::saveval, sdds::opt::readonly), "never");
        sdds_var(enums::TnoYes, valid, sdds_joinOpt(sdds::opt::saveval, sdds::opt::readonly), enums::TnoYes::no);
        sdds_var(Tuint16, signal, sdds_joinOpt(sdds::opt::saveval, sdds::opt::readonly), 0);
        sdds_var(Tuint16, bgrd, sdds_joinOpt(sdds::opt::saveval, sdds::opt::readonly), 0);
    };
    sdds_var(Tzero, zero);

    // sdds variables for beam
    class Tbeam : public TmenuHandle
    {
    public:
        sdds_enum(on, off, error) Tstatus;
        sdds_var(Tstatus, status, sdds::opt::readonly);
        sdds_var(Thardware::Ti2cError, error, sdds::opt::readonly);
    };
    sdds_var(Tbeam, beam);

    // sdds variables for amplifier gain
    class Tgain : public TmenuHandle
    {
    public:
        sdds_var(enums::TnoYes, automatic, sdds::opt::nothing, enums::TnoYes::yes); // not saved, always need to unlock to do manual changes
        sdds_var(Tuint16, max_ppt, sdds::opt::readonly);
        sdds_var(Tuint16, target_ppt, sdds::opt::saveval, 920);
        sdds_var(Tuint32, gain_Ohm, sdds::opt::saveval);
        sdds_enum(set, error) Tstatus;
        sdds_var(Tstatus, status, sdds::opt::readonly);
        sdds_var(Thardware::Ti2cError, error, sdds::opt::readonly);
    };
    sdds_var(Tgain, gain);
    // errors
    sdds_var(Terror, error, sdds::opt::readonly);

    // constructor
    TcomponentOpticalDensity()
    {

        // make sure hardware is initalized
        hardware();

        // gain goals
        on(hardware().signal.maxValue)
        {
            gain.max_ppt = signalToPpt(hardware().signal.maxValue);
        };
        on(gain.target_ppt)
        {
            if (gain.target_ppt > gain.max_ppt)
                gain.target_ppt = gain.max_ppt;
        };
        hardware().signal.maxValue.signalEvents();

        // update gain status from hardware
        on(hardware().gain.total_Ohm)
        {
            // don't update if we're in the middle of optimizing gain or not yet full initialized
            if (status == Tstatus::optimizing || particleSystem().startup != TparticleSystem::TstartupStatus::complete)
                return;

            // update gain with actual hardware gain
            if (gain.gain_Ohm != hardware().gain.total_Ohm)
            {
                gain.gain_Ohm = hardware().gain.total_Ohm;
            }
            // update gain status
            if (hardware().gain.error != Thardware::Ti2cError::none && gain.status != Tgain::Tstatus::error)
                gain.status = Tgain::Tstatus::error;
            else if (hardware().gain.error == Thardware::Ti2cError::none && gain.status != Tgain::Tstatus::set)
                gain.status = Tgain::Tstatus::set;
        };

        // update gain error from hardware
        on(hardware().gain.error)
        {
            if (gain.error != hardware().gain.error)
                gain.error = hardware().gain.error;
            // update  gain status too
            hardware().gain.total_Ohm.signalEvents();
        };

        // update signal from hardware
        on(hardware().signal.value)
        {
            // update ppt value
            reading.saturation_ppt = signalToPpt(hardware().signal.value.value());
            // process the signal
            process();
        };

        // update beam status from hardware
        on(hardware().beamValue)
        {
            if (hardware().beamError != Thardware::Ti2cError::none && beam.status != Tbeam::Tstatus::error)
                beam.status = Tbeam::Tstatus::error;
            else if (hardware().beamValue == Thardware::TbeamValue::ON && beam.status != Tbeam::Tstatus::on)
                beam.status = Tbeam::Tstatus::on;
            else if (hardware().beamValue != Thardware::TbeamValue::ON && beam.status != Tbeam::Tstatus::off)
                beam.status = Tbeam::Tstatus::off;
        };

        // update beam error from hardware
        on(hardware().beamError)
        {
            if (beam.error != hardware().beamError)
                beam.error = hardware().beamError;
            // also update status
            hardware().beamValue.signalEvents();
        };

        // update error if we don't have OD (if we have OD, i.e. are zeroed, this gets incorporated during reads)
        on(hardware().signal.error)
        {
            if (zero.valid != enums::TnoYes::yes)
            {
                if (hardware().signal.error == Thardware::TsignalError::saturated && error == Terror::none)
                    error = Terror::saturated;
                else if (hardware().signal.error == Thardware::TsignalError::none && error == Terror::saturated)
                    error = Terror::none;
            }
        };

        // actions
        on(action)
        {

            // stop if no action
            if (action == Taction::___)
                return;

            // reset error
            error = Terror::none;

            // process actions
            if (action == Taction::reset)
            {
                // reset the zeroing information
                status = Tstatus::idle;
                zero.valid = enums::TnoYes::no;
            }
            else if (action == Taction::beamOn)
            {
                hardware().setBeam(enums::ToffOn::on);
            }
            else if (action == Taction::beamOff)
            {
                hardware().setBeam(enums::ToffOn::off);
            }
            else if (action == Taction::optimizeGain)
            {
                // start gain optimizating
                status = Tstatus::optimizing;
                FgainAdjustment = TgainAdjustmentStages::GAIN_START;
                Freading = TreadingStages::READ_IDLE;
                process();
            }
            else if (action == Taction::zero && gain.automatic == enums::TnoYes::yes)
            {
                // auto-gain first then zero
                status = Tstatus::optimizing;
                FgainAdjustment = TgainAdjustmentStages::GAIN_START;
                Freading = TreadingStages::ZERO_WAIT_FOR_GAIN;
                process();
            }
            else if (action == Taction::zero && gain.automatic == enums::TnoYes::no)
            {
                // zero directly
                status = Tstatus::zeroing;
                Freading = TreadingStages::READ_START;
                process();
            }

            // reset action
            action = Taction::___;
        };

        on(zero.valid)
        {
            // valid?
            if (zero.valid == enums::TnoYes::yes)
            {
                // stop continuous recording
                hardware().recordSignal(enums::ToffOn::off);
                FnextRead = millis() + reading.readInterval_ms.value();
                FreadTimer.start(reading.readInterval_ms.value());
                FinfoTimer.start(0);
            }
            else if (zero.valid == enums::TnoYes::no)
            {
                // continueous signal reads if not zeroed yet
                hardware().recordSignal(enums::ToffOn::on);
                if (FreadTimer.running())
                    FreadTimer.stop();
                reading.nextRead = ""; // no next read
            }
        };

        on(status)
        {
            // are we switching back to idle?
            if (status == Tstatus::idle)
            {
                // resume stirrer and lights (they manage what that means - nothing if they're off)
                if (Fstirrer)
                    (*Fstirrer).action = TcomponentStirrer::Taction::resume;
                if (Flights)
                    (*Flights).action = TcomponentLights::Taction::resume;
            }
        };

        on(FwaitTimer)
        {
            // wait period is done
            process(true);
        };

        on(FwarmupTimer)
        {
            // warump period is done
            process(true);
        };

        on(FreadTimer)
        {
            if (zero.valid == enums::TnoYes::yes)
            {
                status = Tstatus::reading;
                Freading = TreadingStages::READ_START;
                FnextRead = millis() + reading.readInterval_ms.value();
                FreadTimer.start(reading.readInterval_ms.value());
                process();
            }
        };

        on(reading.readInterval_ms)
        {
            if (FreadTimer.running())
            {
                FreadTimer.stop();
                FnextRead = millis() + reading.readInterval_ms.value();
                FreadTimer.start(reading.readInterval_ms.value());
            }
        };

        on(FinfoTimer)
        {
            if (FnextRead > millis())
            {
                char buf[16]; // snprintf buffer
                dtypes::uint32 diff = static_cast<dtypes::uint32>(round((FnextRead - millis()) / 1000.));
                if (diff > 59)
                    snprintf(buf, sizeof(buf), "%dm%ds", diff / 60, diff % 60);
                else
                    snprintf(buf, sizeof(buf), "%ds", diff);
                reading.nextRead = buf;
            }
            else
            {
                reading.nextRead = "now";
            }

            if (zero.valid == enums::TnoYes::yes)
                FinfoTimer.start(1000); // restart
        };
    }

    // set pointer to stirrer component to control stirring
    void setStirrer(TcomponentStirrer *_stirrer)
    {
        Fstirrer = _stirrer;

        // register events
        on((*Fstirrer).event)
        {
            // check if reading.vortexing is done
            if ((FgainAdjustment == TgainAdjustmentStages::GAIN_VORTEX || Freading == TreadingStages::READ_VORTEX) && (*Fstirrer).event == TcomponentStirrer::Tevent::none)
                process(true);
        };

        on((*Fstirrer).status)
        {
            // check if stopping is done
            if ((FgainAdjustment == TgainAdjustmentStages::GAIN_STIR_STOP || Freading == TreadingStages::READ_STIR_STOP) && (*Fstirrer).status == TcomponentStirrer::Tstatus::off)
                process(true);
        };
    }

    // set pointer to lights component to control lights
    void setLights(TcomponentLights *_lights)
    {
        Flights = _lights;
    }

    // pause state if disconnected
    void pauseState()
    {
    }

    // resume the saved state
    void resumeState()
    {
        // beam off
        hardware().setBeam(enums::ToffOn::off);

        // gain reset
        hardware().setGain(gain.gain_Ohm.value());

        // signal reset
        zero.valid.signalEvents();
    }
};
