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
    sdds_enum(___, zero, beamOn, beamOff, optimizeGain) Taction;
    sdds_enum(idle, reading, optimizing, zeroing) Tstatus;
    sdds_enum(none, saturated, failedGain, failedZero) Terror;

private:
    // keep track of publishing to detect when it switches from OFF to ON
    bool FisPublishing = particleSystem().publishing.publish == sdds::enums::OnOff::ON;

    // other components that need to be paused by this one
    TcomponentStirrer *Fstirrer = nullptr;
    TcomponentLights *Flights = nullptr;

    // timers
    Ttimer FreadTimer;
    Ttimer FwaitTimer;
    Ttimer FwarmupTimer;

    // temporary values for gain adjustment
    bool FbeamWasOn = false;
    dtypes::uint16 FnormalSignalReads;
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
            // FIXME: this needs to be implemented, has some similarities to the optimizing flow
            // FIXME: reading should be activated on read_ms schedule if the status is idle and zero.valid is yes
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
                // vortex if it's supposed to vortex
                if (Fstirrer && vortex == enums::TnoYes::yes)
                {
                    (*Fstirrer).action = TcomponentStirrer::Taction::vortex;
                    FgainAdjustment = TgainAdjustmentStages::GAIN_VORTEX;
                    break;
                }
            case TgainAdjustmentStages::GAIN_VORTEX:
                // still waiting on this stage to get completed?
                if (FgainAdjustment != TgainAdjustmentStages::GAIN_START && !_stageDone)
                    break;

                // turn stirrer off if it's supposed to go off
                if (Fstirrer && stopStirrer == enums::TnoYes::yes)
                {
                    (*Fstirrer).action = TcomponentStirrer::Taction::pause;
                    FgainAdjustment = TgainAdjustmentStages::GAIN_STIR_STOP;
                    break;
                }
            case TgainAdjustmentStages::GAIN_STIR_STOP:
                // still waiting on this stage to get completed?
                if (FgainAdjustment != TgainAdjustmentStages::GAIN_START && !_stageDone)
                    break;

                // wait to let the liquid settle after stirring/vortexing
                if (Fstirrer && (vortex == enums::TnoYes::yes || stopStirrer == enums::TnoYes::yes))
                {
                    FgainAdjustment = TgainAdjustmentStages::GAIN_WAIT;
                    FwaitTimer.start(wait_ms);
                    break;
                }
            case TgainAdjustmentStages::GAIN_WAIT:
                // still waiting on this stage to get completed?
                if (FgainAdjustment != TgainAdjustmentStages::GAIN_START && !_stageDone)
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
                    FwarmupTimer.start(warmup_ms);
                    break;
                }
            case TgainAdjustmentStages::GAIN_WARMUP:
                // still waiting on this stage to get completed?
                if (FgainAdjustment != TgainAdjustmentStages::GAIN_START && !_stageDone)
                    break;

                // reduce number of reads for quick gain adjustment
                FnormalSignalReads = hardware().signal.reads;
                hardware().signal.reads = 10;
                hardware().signal.reset();
                FgainAdjustment = TgainAdjustmentStages::READ_NO_GAIN;
                break;
            case TgainAdjustmentStages::READ_NO_GAIN: // first read signal stage
                FlastSignal = hardware().signal.value.value();
                // is the zero signal higher than the target? --> no way we can adjust to reach the garget
                if (FlastSignal > FtargetSignal)
                    Ferror = true;
                FlastSteps = FinitialGainSteps;
                hardware().setGainSteps(FlastSteps);
                FgainAdjustment = TgainAdjustmentStages::READ_INITIAL_GAIN;
                break;
            case TgainAdjustmentStages::READ_INITIAL_GAIN: // second read signal stage
                // was there signal increase with the higher gain? if not --> error
                if (FlastSignal > hardware().signal.value.value())
                    Ferror = true;
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
                dtypes::uint16 newDiff = abs(FtargetSignal - hardware().signal.value.value());
                if (newDiff > FsignalDifference)
                {
                    // previous setting was better, keep that one and FINISH
                    error = Terror::none;
                    hardware().signal.reads = FnormalSignalReads;
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
                error = Terror::failedGain;
                status = Tstatus::idle;
                hardware().signal.reads = FnormalSignalReads;
            }
            hardware().signal.reset();
        }
    }

public:
    // sdds variables
    sdds_var(Taction, action);                          // what to do
    sdds_var(Tuint16, signal_ppt, sdds::opt::readonly); // signal in parts per thousand
    sdds_var(Terror, error, sdds::opt::readonly);       // errors

    // FIXME: these are all the additional setings
    sdds_var(Tstatus, status, sdds::opt::readonly);
    sdds_var(Tuint16, read_S, sdds::opt::saveval, 200);                           // how often to read
    sdds_var(enums::TnoYes, vortex, sdds::opt::saveval, enums::TnoYes::no);       // vortex before reading?
    sdds_var(enums::TnoYes, stopStirrer, sdds::opt::saveval, enums::TnoYes::yes); // stop stirrer before read?
    sdds_var(Tuint16, wait_ms, sdds::opt::saveval, 1000);                         // how long to wait after vortex/stirrer stop
    sdds_var(Tuint16, warmup_ms, sdds::opt::saveval, 200);                        // beam warmup (how long to wait whenever the beam is turned on)
    // FIXME: are these in the right place?

    class Tzero : public TmenuHandle
    {
    public:
        sdds_var(Tstring, last, sdds_joinOpt(sdds::opt::saveval, sdds::opt::readonly), "never");
        sdds_var(enums::TnoYes, valid, sdds_joinOpt(sdds::opt::saveval, sdds::opt::readonly), enums::TnoYes::no);
        sdds_var(Tuint16, dark, sdds::opt::readonly, 0);
        sdds_var(Tuint16, value, sdds::opt::readonly, 0);
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
        sdds_var(enums::TnoYes, automatic, sdds::opt::saveval, enums::TnoYes::yes);
        sdds_var(Tuint16, max_ppt, sdds::opt::readonly);
        sdds_var(Tuint16, target_ppt, sdds::opt::saveval, 920);
        sdds_var(Tuint32, gain_Ohm, sdds::opt::saveval);
        sdds_enum(set, error) Tstatus;
        sdds_var(Tstatus, status, sdds::opt::readonly);
        sdds_var(Thardware::Ti2cError, error, sdds::opt::readonly);
    };
    sdds_var(Tgain, gain);

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

        // manual gain adjustments
        on(gain.gain_Ohm)
        {
            if (gain.automatic == enums::TnoYes::no)
            {
                // automatic gain is off --> set gain if it is not already this value
                if (gain.gain_Ohm != hardware().gain.total_Ohm)
                    hardware().setGain(gain.gain_Ohm.value());
            }
            else
            {
                // automatic gain --> jump back to original value
                // gain will be automatically adjusted when the zero action is executed
                hardware().gain.total_Ohm.signalEvents();
            }
        };

        // update gain status from hardware
        on(hardware().gain.total_Ohm)
        {
            // don't update if we're in the middle of optimizing gain
            if (status == Tstatus::optimizing)
                return;

            // update gain with actual hardware gain
            if (gain.gain_Ohm != hardware().gain.total_Ohm)
            {
                gain.gain_Ohm = hardware().gain.total_Ohm;
                // any changes in gain invalidate the last zeroing
                if (zero.valid != enums::TnoYes::no)
                    zero.valid = enums::TnoYes::no;
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
            signal_ppt = signalToPpt(hardware().signal.value.value());
            // process the signal
            process();
        };

        // update error if signal is saturated (only set update if the specific one is set since OpticalDensity also adds errors for zero and gain)
        on(hardware().signal.error)
        {
            if (hardware().signal.error == Thardware::TsignalError::saturated && error == Terror::none)
                error = Terror::saturated;
            else if (hardware().signal.error == Thardware::TsignalError::none && error == Terror::saturated)
                error = Terror::none;
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

        // actions
        on(action)
        {

            // stop if no action
            if (action == Taction::___)
                return;

            // reset error
            error = Terror::none;

            // process actions
            if (action == Taction::beamOn)
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
            }
            else if (action == Taction::zero && gain.automatic == enums::TnoYes::yes)
            {
                // auto-gain first then zero
                status = Tstatus::optimizing;
                FgainAdjustment = TgainAdjustmentStages::GAIN_START;
                Freading = TreadingStages::ZERO_WAIT_FOR_GAIN;
            }
            else if (action == Taction::zero && gain.automatic == enums::TnoYes::no)
            {
                // zero directly
                status = Tstatus::zeroing;
                Freading = TreadingStages::READ_START;
            }

            // reset action
            action = Taction::___;
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
    }

    // set pointer to stirrer component to control stirring
    void setStirrer(TcomponentStirrer *_stirrer)
    {
        Fstirrer = _stirrer;

        // register events
        on((*Fstirrer).event)
        {
            // check if vortexing is done
            if (FgainAdjustment == TgainAdjustmentStages::GAIN_VORTEX && (*Fstirrer).event == TcomponentStirrer::Tevent::none)
                process(true);
        };

        on((*Fstirrer).status)
        {
            // check if stopping is done
            if (FgainAdjustment == TgainAdjustmentStages::GAIN_STIR_STOP && (*Fstirrer).status == TcomponentStirrer::Tstatus::off)
                process(true);
        };
    }

    // set pointer to lights component to control lights
    void setLights(TcomponentLights *_lights)
    {
        Flights = _lights;
    }

    // resume the saved state
    void resumeState()
    {
        // beam off
        hardware().setBeam(enums::ToffOn::off);

        // signal reset
        hardware().signal.reset();

        // gain reset
        hardware().setGain(gain.gain_Ohm.value());
    }
};
