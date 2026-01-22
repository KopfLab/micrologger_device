#pragma once
#include "uTypedef.h"
#include "enums.h"
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
    sdds_enum(___, on, off, schedule, pause, resume) Taction;
    sdds_enum(on, off, schedule) Tstate;
    sdds_enum(on, off, error) Tstatus;

public:
    // sdds variables for light
    sdds_var(Taction, action);
    sdds_var(Tstate, state, sdds_joinOpt(sdds::opt::saveval, sdds::opt::readonly), Tstate::off);
    sdds_var(Tstatus, status, sdds::opt::readonly, Tstatus::off);
    sdds_var(Thardware::Ti2cError, error, sdds::opt::readonly);
    sdds_var(Tuint8, intensity, sdds::opt::saveval, 100);
    sdds_var(Tuint32, scheduleOn_SEC, sdds::opt::saveval, 60 * 60 * 12);  // 12 hours on
    sdds_var(Tuint32, scheduleOff_SEC, sdds::opt::saveval, 60 * 60 * 12); // 12 hours off
    sdds_var(Tfloat32, scheduleOnStart_HHMM, sdds::opt::saveval, 6.00);
    sdds_var(Tstring, scheduleInfo, sdds::opt::readonly);

    // sdds variables for fan
    class Tfan : public TmenuHandle
    {
    public:
        sdds_enum(___, on, off, withLight) Taction;
        sdds_enum(on, off, withLight) Tstate;
        sdds_var(Taction, action);
        sdds_var(Tstate, state, sdds::opt::saveval, Tstate::withLight);
        sdds_var(Tstatus, status, sdds::opt::readonly, Tstatus::off);
    };
    sdds_var(Tfan, fan);

private:
    // schedule timer
    Ttimer FscheduleTimer;

    // schedule structure
    struct Schedule
    {
        bool isOn;
        uint32_t secondsToSwitch;
        uint8_t h, m, s;
    };

    Schedule calculateSchedule()
    {
        // schedule object
        Schedule r{false, 0, 0, 0, 0};

        // do we have a valid time?
        if (!Time.isValid())
            return r;

        // figure out how much time has past since the start (today or yesterday)
        const uint16_t start_hour = static_cast<uint16_t>(floor(scheduleOnStart_HHMM.value()));
        const uint16_t start_min = static_cast<uint16_t>(round((scheduleOnStart_HHMM.value() - start_hour) * 100));
        const uint32_t start_sec = start_hour * 60 * 60 + start_min * 60;
        const uint32_t now_sec = static_cast<uint32_t>(Time.hour()) * 3600 + Time.minute() * 60 + Time.second();
        const uint32_t delta = (start_sec > now_sec) ? 24 * 60 * 60 + now_sec - start_sec : now_sec - start_sec;

        // figure out which phase we are in and when the next phase starts
        const uint32_t period = scheduleOn_SEC.value() + scheduleOff_SEC.value();
        const uint32_t phase = delta % period;
        if (phase < scheduleOn_SEC.value())
        {
            r.isOn = true;
            r.secondsToSwitch = scheduleOn_SEC.value() - phase; // ON -> OFF
        }
        else
        {
            r.isOn = false;
            r.secondsToSwitch = period - phase; // OFF -> ON
        }

        // determine time to next in H:M:S
        uint32_t t = r.secondsToSwitch;
        r.h = (uint8_t)(t / 3600);
        t %= 3600;
        r.m = (uint8_t)(t / 60);
        r.s = (uint8_t)(t % 60);
        return r;
    }

    // update with current light and fan state
    void update()
    {
        update(state, fan.state);
    }

    // update hardware based on settings
    void update(Tstate::e _lightState, Tfan::Tstate::e _fanState)
    {
        // light
        bool light = false;
        if (_lightState == Tstate::off)
        {
            // off
            if (scheduleInfo != "")
                scheduleInfo = "";
        }
        else if (_lightState == Tstate::on)
        {
            // on
            light = true;
            if (scheduleInfo != "")
                scheduleInfo = "";
        }
        else if (_lightState == Tstate::schedule)
        {
            // schedule
            if (!Time.isValid())
            {
                // don't have valid time yet - keep off and reschedule check
                if (scheduleInfo != "pending")
                    scheduleInfo = "pending";
                FscheduleTimer.start(1000);
            }
            else
            {

                // determine schedule
                Schedule s = calculateSchedule();
                light = s.isOn;
                const dtypes::string next = (s.isOn) ? "off:" : "on:";

                // when to scheck back in?
                if (s.h == 0 && s.m == 0)
                {
                    scheduleInfo = next + dtypes::string(s.s) + "s";
                    FscheduleTimer.start(1000); // once a second
                }
                else if (s.h == 0)
                {
                    scheduleInfo = next + dtypes::string(s.m) + "m" + dtypes::string(s.s) + "s";
                    FscheduleTimer.start(1000); // once a second
                }
                else
                {
                    scheduleInfo = next + dtypes::string(s.h) + "h" + dtypes::string(s.m) + "m";
                    FscheduleTimer.start((s.s + 1) * 1000); // once we're down a minute
                }
            }
        }

        // update light
        (light) ? hardware().setLight(enums::ToffOn::on, intensity.value()) : hardware().setLight(enums::ToffOn::off);

        // fan
        bool fan = false;
        if (_fanState == Tfan::Tstate::on)
            fan = true;
        else if (_fanState == Tfan::Tstate::withLight)
            fan = light;

        // update fan
        (fan) ? hardware().setFan(enums::ToffOn::on) : hardware().setFan(enums::ToffOn::off);
    }

public:
    // constructor
    Tlights()
    {

        // make sure hardware is initalized
        hardware();

        // resume state when startup is complete
        on(particleSystem().startup)
        {
            resumeState();
        };

        // timer
        on(FscheduleTimer)
        {
            update();
        };

        // light action events
        on(action)
        {
            if (action == Taction::on)
            {
                state = Tstate::on;
                update();
            }
            else if (action == Taction::off)
            {
                state = Tstate::off;
                update();
            }
            else if (action == Taction::schedule)
            {
                state = Tstate::schedule;
                update();
            }
            else if (action == Taction::pause)
            {
                // no state change but turning off the light (fan follows if it's "withLight")
                update(Tstate::off, fan.state);
            }
            else if (action == Taction::resume)
            {
                // no state change, resuming light with current state
                update();
            }
            if (action != Taction::___)
                action = Taction::___;
        };

        // change light intensity
        on(intensity)
        {
            if (intensity == 0)
                intensity = 1;
            else if (intensity > 100)
                intensity = 100;
            else
                update();
        };

        // update light status from hardware
        on(hardware().lightValue)
        {
            if (hardware().fanLightError != Thardware::Ti2cError::none)
            {
                // error
                error = hardware().fanLightError;
                status = Tstatus::error;
            }
            else if ((hardware().lightValue == Thardware::TlightValue::ON || hardware().lightValue == Thardware::TlightValue::DIMMED) && status != Tstatus::on)
            {
                // on
                error = Thardware::Ti2cError::none;
                status = Tstatus::on;
            }
            else if ((hardware().lightValue != Thardware::TlightValue::ON && hardware().lightValue != Thardware::TlightValue::DIMMED) && status != Tstatus::off)
            {
                // off
                error = Thardware::Ti2cError::none;
                status = Tstatus::off;
            }
        };

        // update light error from hardware (same i2c error as the fan)
        on(hardware().fanLightError)
        {
            hardware().lightValue.signalEvents();
            hardware().fanValue.signalEvents();
        };

        // schedule
        on(scheduleOn_SEC)
        {
            if (scheduleOn_SEC == 0)
                scheduleOn_SEC = 1;
            else
                update();
        };
        on(scheduleOff_SEC)
        {
            if (scheduleOff_SEC == 0)
                scheduleOff_SEC = 1;
            else
                update();
        };
        on(scheduleOnStart_HHMM)
        {
            if (scheduleOnStart_HHMM > 23.59)
                scheduleOnStart_HHMM = 0;
            else
                update();
        };

        // fan action events
        on(fan.action)
        {
            if (fan.action == Tfan::Taction::on)
            {
                fan.state = Tfan::Tstate::on;
                update();
            }
            else if (fan.action == Tfan::Taction::off)
            {
                fan.state = Tfan::Tstate::off;
                update();
            }
            else if (fan.action == Tfan::Taction::withLight)
            {
                fan.state = Tfan::Tstate::withLight;
                update();
            }
            if (fan.action != Tfan::Taction::___)
                fan.action = Tfan::Taction::___;
        };

        // update fan status from hardware
        on(hardware().fanValue)
        {
            if (hardware().fanLightError != Thardware::Ti2cError::none)
                fan.status = Tstatus::error;
            else if ((hardware().fanValue == Thardware::TfanValue::ON || hardware().fanValue == Thardware::TfanValue::DIMMED) && fan.status != Tstatus::on)
                fan.status = Tstatus::on;
            else if ((hardware().fanValue != Thardware::TfanValue::ON && hardware().fanValue != Thardware::TfanValue::DIMMED) && fan.status != Tstatus::off)
                fan.status = Tstatus::off;
        };
    }

    // resume the saved state of the motor after power cycling or reconnection
    void resumeState()
    {
        update();
    }
};