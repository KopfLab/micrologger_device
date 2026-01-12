#pragma once
#include "uTypedef.h"
#include "uCoreEnums.h"
#include "uHardware.h"
#ifdef SDDS_ON_PARTICLE
#include "uParticleSystem.h"
#endif

/**
 * @brief stirrer class
 */
class Tlights : public TmenuHandle
{

public:
    // enumerations
    using TonOff = sdds::enums::OnOff;
    sdds_enum(on, off, schedule) Tmode;
    sdds_enum(on, off, paused) Tstate;

public:
    // sdds variables for light
    sdds_var(Tmode, mode, sdds::opt::saveval, Tmode::off);
    sdds_var(Tuint8, intensity, sdds::opt::nothing, 0);
    sdds_var(Tuint32, scheduleOn_SEC, sdds::opt::saveval, 60 * 60 * 12);  // 12 hours on
    sdds_var(Tuint32, scheduleOff_SEC, sdds::opt::saveval, 60 * 60 * 12); // 12 hours off
    sdds_var(Tfloat32, scheduleOnStart_HHMM, sdds::opt::saveval, 6.00);
    sdds_var(Tstate, state, sdds::opt::readonly, Tstate::off);

    // fan
    class Tfan : public TmenuHandle
    {
    public:
        using TonOff = sdds::enums::OnOff;
        sdds_enum(on, off, withLights) Tmode;
        sdds_var(Tmode, mode, sdds::opt::saveval, Tmode::withLights);
        sdds_var(TonOff, state, sdds::opt::readonly, TonOff::OFF);
    };
    sdds_var(Tfan, fan);

    // constructor
    Tlights()
    {
        // FIXME: consider timezone for the schedule
        // FIXME: show errors
        // FIXME: implement all of the schedule stuff

        // lights on/off
        on(mode)
        {
            if (mode == Tmode::on)
            {
                hardware().lights = TonOff::ON;
                if (fan.mode == Tfan::Tmode::withLights)
                    hardware().fan = TonOff::ON;
            }
            else if (mode == Tmode::off)
            {
                hardware().lights = TonOff::OFF;
                if (fan.mode == Tfan::Tmode::withLights)
                    hardware().fan = TonOff::OFF;
            }
            // FIXME: deal with Tmode::schedule
        };

        // update lights state
        on(hardware().lights)
        {
            if (hardware().lights == TonOff::ON && state != Tstate::on)
            {
                state = Tstate::on;
            }
            else if (hardware().lights == TonOff::OFF && state != Tstate::off)
            {
                state = Tstate::off;
            }
            // FIXME: deal with Tstate::paused
        };

        // light intensity
        on(intensity)
        {
            if (hardware().intensity != intensity)
                hardware().intensity = intensity;
        };
        on(hardware().intensity)
        {
            if (hardware().intensity != intensity)
                intensity = hardware().intensity;
        };

        // fan on/off
        on(fan.mode)
        {
            if (fan.mode == Tfan::Tmode::on)
            {
                hardware().fan = TonOff::ON;
            }
            else if (fan.mode == Tfan::Tmode::off)
            {
                hardware().fan = TonOff::OFF;
            }
            else if (fan.mode == Tfan::Tmode::withLights)
            {
                hardware().fan = hardware().lights;
            }
        };

        // update fan state
        on(hardware().fan)
        {
            if (hardware().fan == TonOff::ON && fan.state != TonOff::ON)
            {
                fan.state = TonOff::ON;
            }
            else if (hardware().fan == TonOff::OFF && fan.state != TonOff::OFF)
            {
                fan.state = TonOff::OFF;
            }
        };
    }
};