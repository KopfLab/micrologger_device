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
class TopticalDensity : public TmenuHandle
{

public:
    // enumerations
    sdds_enum(___, beamOn, beamOff, zero) Taction;
    sdds_enum(off, idle, vortexing, waiting, reading) Tstatus;

    using TsignalError = Thardware::TsignalError;

private:
    // keep track of publishing to detect when it switches from OFF to ON
    bool FisPublishing = particleSystem().publishing.publish == sdds::enums::OnOff::ON;

    // signal stats
    TrunningStats rs;

    // timers
    Ttimer FreadTimer;
    Ttimer FwaitTimer;
    Ttimer FwarmupTimer;

    dtypes::uint16 signalToPpt(dtypes::uint16 _signal)
    {
        return static_cast<dtypes::uint16>(round(_signal * 1000. / hardware().signal.adcResolution));
    }

public:
    // sdds variables
    sdds_var(Taction, action); // what to do
    sdds_var(Tuint16, signal_ppt, sdds::opt::readonly);
    sdds_var(TsignalError, error, sdds::opt::readonly); // signal error

    // FIXME: these are all the additional setings
    sdds_var(Tstatus, status, sdds::opt::readonly);
    sdds_var(Tuint16, read_S, sdds::opt::saveval, 200);                          // how often to read
    sdds_var(enums::ToffOn, vortex, sdds::opt::saveval, enums::ToffOn::off);     // vortex before reading?
    sdds_var(enums::ToffOn, stopStirrer, sdds::opt::saveval, enums::ToffOn::on); // stop stirrer before read?
    sdds_var(Tuint16, wait_MS, sdds::opt::saveval, 1000);                        // how long to wait after vortexing
    sdds_var(Tuint16, warmup_MS, sdds::opt::saveval, 200);                       // beam warmup
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
    TopticalDensity()
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

        // gain adjustments
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
            signal_ppt = signalToPpt(hardware().signal.value.value());

            if (status == Tstatus::reading)
            {
                // do something with the value
            }
        };

        // update signal error from hardware
        on(hardware().signal.error)
        {
            if (error != hardware().signal.error)
                error = hardware().signal.error;
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

            if (action == Taction::beamOn)
            {
                hardware().setBeam(enums::ToffOn::on);
            }
            else if (action == Taction::beamOff)
            {
                hardware().setBeam(enums::ToffOn::off);
            }

            // // process action
            // if (action == Taction::start)
            // {
            //     status = Tstatus::idle;
            //     FreadTimer.start(0); // read righ away
            // }
            // else if (action == Taction::stop)
            // {
            //     status = Tstatus::off;
            // }

            // reset action
            action = Taction::___;
        };

        on(FreadTimer)
        {
            // time to read again
            if (status != Tstatus::off)
            {
                if (vortex == enums::ToffOn::on)
                    status = Tstatus::vortexing; // trigger preparing mode
                else
                    FwaitTimer.start(0); // read right away
                FreadTimer.start(read_S.value() * 1000);
            }
        };

        on(status)
        {
            if (status == Tstatus::waiting)
            {
                FwaitTimer.start(wait_MS.value());
            }
        };

        on(FwaitTimer)
        {
            // done with wait
            if (status != Tstatus::off)
            {
                // hardware().beam = Tenums::offOn::on;
                FwarmupTimer.start(warmup_MS.value());
            }
        };

        on(FwarmupTimer)
        {
            // done with warmup
            if (status != Tstatus::off)
            {
                rs.reset();
                status = Tstatus::reading;
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
