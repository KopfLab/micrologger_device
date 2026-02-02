#pragma once
#include "uTypedef.h"
#include "enums.h"
#include "uHardware.h"
#include "uParticleSystem.h"

/**
 * @brief stirrer class
 */
class TcomponentLights : public TmenuHandle
{

public:
    // enumerations
    sdds_enum(___, on, off, schedule, pause, resume) Taction;
    sdds_enum(on, off, schedule) Tstate;
    sdds_enum(on, off, error) Tstatus;
    sdds_enum(none, paused) Tevent;

public:
    // sdds variables for light
    sdds_var(Taction, action);
    sdds_var(Tstate, state, sdds_joinOpt(sdds::opt::saveval, sdds::opt::readonly), Tstate::off);
    sdds_var(Tstatus, status, sdds::opt::readonly, Tstatus::off);
    sdds_var(Tevent, event, sdds::opt::readonly, Tevent::none);
    sdds_var(Thardware::Ti2cError, error, sdds::opt::readonly);
    sdds_var(Tuint8, intensity, sdds::opt::saveval, 100);
    sdds_var(Tuint32, scheduleOn_sec, sdds::opt::saveval, 60 * 60 * 12);  // 12 hours on
    sdds_var(Tuint32, scheduleOff_sec, sdds::opt::saveval, 60 * 60 * 12); // 12 hours off
    sdds_var(Tuint16, scheduleOnStart_HHMM, sdds::opt::saveval, 1200);
    sdds_var(Tstring, scheduleInfo, sdds::opt::readonly);

    // sdds variables for fan
    class Tfan : public TmenuHandle
    {
    public:
        sdds_enum(___, on, off, withLight) Taction;
        sdds_enum(on, off, withLight) Tstate;
        sdds_var(Taction, action);
        sdds_var(Tstate, state, sdds_joinOpt(sdds::opt::saveval, sdds::opt::readonly), Tstate::withLight);
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
        const uint16_t start_hour = scheduleOnStart_HHMM.value() / 100;
        const uint16_t start_min = scheduleOnStart_HHMM.value() % 100;
        const uint32_t start_sec = start_hour * 60 * 60 + start_min * 60;
        const uint32_t now_sec = static_cast<uint32_t>(Time.hour()) * 3600 + Time.minute() * 60 + Time.second();
        const uint32_t delta = (start_sec > now_sec) ? 24 * 60 * 60 + now_sec - start_sec : now_sec - start_sec;

        // figure out which phase we are in and when the next phase starts
        const uint32_t period = scheduleOn_sec.value() + scheduleOff_sec.value();
        const uint32_t phase = delta % period;
        if (phase < scheduleOn_sec.value())
        {
            r.isOn = true;
            r.secondsToSwitch = scheduleOn_sec.value() - phase; // ON -> OFF
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

        // deal with pause event
        if (event == Tevent::paused)
            light = false;

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
    TcomponentLights()
    {

        // make sure hardware is initalized
        hardware();

        // timer
        on(FscheduleTimer)
        {
            update();
        };

        // light action events
        on(action)
        {
            // stop if no action
            if (action == Taction::___)
                return;

            // update event
            if (action == Taction::pause && state != Tstate::off && event != Tevent::paused)
            {
                // pause if we're on/schedule and not paused yet
                event = Tevent::paused;
            }
            else if (action != Taction::pause && event != Tevent::none)
            {
                event = Tevent::none;
            }

            // process action
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
                // no state change but event paused is now active
                update();
            }
            else if (action == Taction::resume)
            {
                // no state change, just resuming
                update();
            }

            // reset action
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
            if (hardware().fanLightError != Thardware::Ti2cError::none && status != Tstatus::error)
                status = Tstatus::error;
            else if ((hardware().lightValue == Thardware::TlightValue::ON || hardware().lightValue == Thardware::TlightValue::DIMMED) && status != Tstatus::on)
                status = Tstatus::on;
            else if ((hardware().lightValue != Thardware::TlightValue::ON && hardware().lightValue != Thardware::TlightValue::DIMMED) && status != Tstatus::off)
                status = Tstatus::off;
        };

        // update light error from hardware (same i2c error as the fan)
        on(hardware().fanLightError)
        {
            if (error != hardware().fanLightError)
                error = hardware().fanLightError;
            // also update light and fan status
            hardware().lightValue.signalEvents();
            hardware().fanValue.signalEvents();
        };

        // schedule
        on(scheduleOn_sec)
        {
            if (scheduleOn_sec == 0)
                scheduleOn_sec = 1;
            else
                update();
        };
        on(scheduleOff_sec)
        {
            if (scheduleOff_sec == 0)
                scheduleOff_sec = 1;
            else
                update();
        };
        on(scheduleOnStart_HHMM)
        {
            if (scheduleOnStart_HHMM > 2359)
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
            if (hardware().fanLightError != Thardware::Ti2cError::none && fan.status != Tstatus::error)
                fan.status = Tstatus::error;
            else if ((hardware().fanValue == Thardware::TfanValue::ON || hardware().fanValue == Thardware::TfanValue::DIMMED) && fan.status != Tstatus::on)
                fan.status = Tstatus::on;
            else if ((hardware().fanValue != Thardware::TfanValue::ON && hardware().fanValue != Thardware::TfanValue::DIMMED) && fan.status != Tstatus::off)
                fan.status = Tstatus::off;
        };
    }

    // pause state if disconnected
    void pauseState()
    {
    }

    // resume the saved state after power cycling or reconnection
    void resumeState()
    {
        update();
    }
};