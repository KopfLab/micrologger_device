#pragma once
#include "uTypedef.h"
#include "uCoreEnums.h"
#include "enums.h"
#include "uRunningStats.h"
#include "uHardware.h"
#ifdef SDDS_ON_PARTICLE
#include "uParticleSystem.h"
#endif

/**
 * @brief stirrer class
 */
class TcomponentOpticalDensity : public TmenuHandle
{

public:
    // enumerations
    sdds_enum(___, zero, beamOn, beamOff, optimizeGain) Taction;
    sdds_enum(off, idle, vortexing, waiting, reading, optimizing, zeroing) Tstatus;
    sdds_enum(none, saturated, failedGain, failedZero) Terror;

private:
    // keep track of publishing to detect when it switches from OFF to ON
    bool FisPublishing = particleSystem().publishing.publish == sdds::enums::OnOff::ON;

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
        IDLE_GAIN,
        START_GAIN,
        WARMUP_GAIN,
        NO_GAIN,
        INITIAL_GAIN,
        FINE_ADJUSTMENT
    } FgainAdjustment;
    const dtypes::uint8 FinitialGainSteps = 5;

    // temporary values for zero-ing
    enum TzeroingStages
    {
        IDLE_ZERO,
        START_ZERO,
        WAIT_FOR_GAIN
    } Fzeroing;

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
    void processSignal(dtypes::uint16 _signal)
    {

        if (status == Tstatus::reading)
        {
            // do something with the value
        }
        else if (status == Tstatus::optimizing)
        {
            // in gain adjustment
            bool Ferror = gain.status == Tgain::Tstatus::error || beam.status == Tbeam::Tstatus::error;
            if (FgainAdjustment == TgainAdjustmentStages::START_GAIN)
            {
                // activate gain adjustment
                FtargetSignal = pptToSignal(gain.target_ppt.value());

                // put gain to 0
                hardware().setGainSteps(0);

                // turn beam on if not already on and wait for warmup period
                FbeamWasOn = beam.status == Tbeam::Tstatus::on;
                if (!FbeamWasOn)
                {
                    hardware().setBeam(enums::ToffOn::on);
                    FgainAdjustment = TgainAdjustmentStages::WARMUP_GAIN;
                    FwarmupTimer.start(warmup_ms);
                }
                else
                {
                    FgainAdjustment = TgainAdjustmentStages::NO_GAIN;
                }

                // reduce number of reads for quick gain adjustment
                FnormalSignalReads = hardware().signal.reads;
                hardware().signal.reads = 10;
                hardware().signal.reset();
            }
            else if (FgainAdjustment == TgainAdjustmentStages::NO_GAIN)
            {
                FlastSignal = _signal;
                // is the zero signal higher than the target? --> no way we can adjust to reach the garget
                if (FlastSignal > FtargetSignal)
                    Ferror = true;
                FlastSteps = FinitialGainSteps;
                hardware().setGainSteps(FlastSteps);
                FgainAdjustment = TgainAdjustmentStages::INITIAL_GAIN;
            }
            else if (FgainAdjustment == TgainAdjustmentStages::INITIAL_GAIN)
            {
                // was there signal increase with the higher gain? if not --> error
                if (FlastSignal > _signal)
                    Ferror = true;
                // calculate slope (gain / step)
                dtypes::float64 slope = static_cast<dtypes::float64>(_signal - FlastSignal) / FinitialGainSteps;
                // cadjust gain to reach target approximately
                dtypes::uint16 newGain = static_cast<dtypes::uint16>(round((FtargetSignal - FlastSignal) / slope));
                // update last signal for fine-tuning
                FgainAdjustment = TgainAdjustmentStages::FINE_ADJUSTMENT;
                FlastSignal = _signal;
                FsignalDifference = abs(FtargetSignal - FlastSignal);
                hardware().setGainSteps(newGain);
            }
            else if (FgainAdjustment == TgainAdjustmentStages::FINE_ADJUSTMENT)
            {
                // finish
                dtypes::uint16 newDiff = abs(FtargetSignal - _signal);
                if (newDiff > FsignalDifference)
                {
                    // previous setting was better, keep that one and finish
                    error = Terror::none;
                    status = Tstatus::idle;
                    hardware().signal.reads = FnormalSignalReads;
                    hardware().setGainSteps(FlastSteps);
                    if (!FbeamWasOn)
                        hardware().setBeam(enums::ToffOn::off);
                    FgainAdjustment = TgainAdjustmentStages::IDLE_GAIN;
                    // should zeroing start now?
                    if (Fzeroing == TzeroingStages::WAIT_FOR_GAIN)
                    {
                        status = Tstatus::zeroing;
                        Fzeroing = TzeroingStages::START_ZERO;
                    }
                }
                else
                {
                    // new setting is better --> repeat fine adjustment
                    FlastSignal = _signal;
                    FlastSteps = hardware().gain.steps.value();
                    FsignalDifference = newDiff;
                    hardware().setGainSteps(FlastSteps + ((FtargetSignal > FlastSignal) ? +1 : -1));
                }
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
    sdds_var(Tuint16, read_S, sdds::opt::saveval, 200);                          // how often to read
    sdds_var(enums::ToffOn, vortex, sdds::opt::saveval, enums::ToffOn::off);     // vortex before reading?
    sdds_var(enums::ToffOn, stopStirrer, sdds::opt::saveval, enums::ToffOn::on); // stop stirrer before read?
    sdds_var(Tuint16, wait_MS, sdds::opt::saveval, 1000);                        // how long to wait after vortexing
    sdds_var(Tuint16, warmup_ms, sdds::opt::saveval, 200);                       // beam warmup (how long to wait whenever the beam is turned on)
    // FIXME: are these in the right place?

    class Tzero : public TmenuHandle
    {
    public:
        sdds_var(Tstring, last, sdds_joinOpt(sdds::opt::saveval, sdds::opt::readonly), "never");
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
        sdds_var(enums::ToffOn, automatic, sdds::opt::saveval, enums::ToffOn::on);
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
            if (gain.automatic == enums::ToffOn::off)
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
                gain.gain_Ohm = hardware().gain.total_Ohm;
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
            processSignal(hardware().signal.value.value());
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
                FgainAdjustment = TgainAdjustmentStages::START_GAIN;
                Fzeroing = TzeroingStages::IDLE_ZERO;
            }
            else if (action == Taction::zero && gain.automatic == enums::ToffOn::on)
            {
                // auto-gain first then zero
                status = Tstatus::optimizing;
                FgainAdjustment = TgainAdjustmentStages::START_GAIN;
                Fzeroing = TzeroingStages::WAIT_FOR_GAIN;
            }
            else if (action == Taction::zero && gain.automatic == enums::ToffOn::off)
            {
                // zero directly
                status = Tstatus::zeroing;
                Fzeroing = TzeroingStages::START_ZERO;
            }

            // reset action
            action = Taction::___;
        };

        // on(FreadTimer)
        // {
        //     // time to read again
        //     if (status != Tstatus::off)
        //     {
        //         if (vortex == enums::ToffOn::on)
        //             status = Tstatus::vortexing; // trigger preparing mode
        //         else
        //             FwaitTimer.start(0); // read right away
        //         FreadTimer.start(read_S.value() * 1000);
        //     }
        // };

        // on(status)
        // {
        //     if (status == Tstatus::waiting)
        //     {
        //         FwaitTimer.start(wait_MS.value());
        //     }
        // };

        // on(FwaitTimer)
        // {
        //     // done with wait
        //     if (status != Tstatus::off)
        //     {
        //         // hardware().beam = Tenums::offOn::on;
        //         FwarmupTimer.start(warmup_ms.value());
        //     }
        // };

        on(FwarmupTimer)
        {
            // reset signal
            hardware().signal.reset();
            if (status == Tstatus::optimizing && FgainAdjustment == TgainAdjustmentStages::WARMUP_GAIN)
            {
                FgainAdjustment = TgainAdjustmentStages::NO_GAIN;
            }
        };
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
