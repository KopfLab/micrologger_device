#pragma once
#include "uTypedef.h"
#include "uCoreEnums.h"
#include "enums.h"
#include "uHardware.h"
#include "uParticleSystem.h"

/**
 * @brief stirrer class
 */
class TcomponentStirrer : public TmenuHandle
{

public:
    // enumerations
    sdds_enum(___, start, stop, pause, resume, vortex) Taction;
    sdds_enum(off, accelerating, decelerating, running, error) Tstatus;
    sdds_enum(none, paused, vortexing) Tevent;

private:
    // keep track of publishing to detect when it switches from OFF to ON
    bool FisPublishing = particleSystem().publishing.publish == sdds::enums::OnOff::ON;

    // current speed setting
    dtypes::uint16 FspeedTarget = 0;
    dtypes::uint16 FspeedNow = 0;

    // timers
    const dtypes::TtickCount FrestartDelay = 1000; // ms
    Ttimer FrestartTimer;
    const dtypes::TtickCount FspeedChangeInterval = 100; // ms
    Ttimer FspeedChangeTimer;
    Ttimer FvortexEndTimer;

    // reached vortex peak?
    bool vortexPeak = false;

    /**
     * @brief set a new target speed
     */
    void changeSpeed(dtypes::uint16 _speed)
    {
        FvortexEndTimer.stop();
        FspeedChangeTimer.stop();
        FspeedTarget = _speed;
        adjustSpeed();
    }

    /**
     * @brief adjust speed towards the speed target
     */
    void adjustSpeed()
    {

        if (FspeedNow == FspeedTarget)
        {
            // already at target, nothing to do
            return;
        }

        if ((FspeedNow < FspeedTarget && settings.acceleration == 0) || (FspeedNow > FspeedTarget && settings.deceleration == 0))
        {
            // no accelerationg/decellration setting, change speed immediately
            setMotorSpeed(FspeedTarget);
            return;
        }

        // change speed by the acceleration/deceleration amount
        if (FspeedNow < FspeedTarget)
        {
            dtypes::uint16 change = settings.acceleration * FspeedChangeInterval / 1000;
            if (FspeedNow + change > FspeedTarget)
            {
                // reached target speed
                setMotorSpeed(FspeedTarget);
                return;
            }
            // time to increase the speed
            setMotorSpeed(FspeedNow + change);
        }
        else if (FspeedNow > FspeedTarget)
        {
            dtypes::uint16 change = settings.deceleration * FspeedChangeInterval / 1000;
            if (FspeedNow < FspeedTarget + change)
            {
                // reached target speed
                setMotorSpeed(FspeedTarget);
                return;
            }
            // time to decrease the speed
            setMotorSpeed(FspeedNow - change);
        }
        // check for issues
        if (hardware().motor.error != Thardware::TmotorError::none)
        {
            return; // there are motor errors
        }

        // there's more to do, start timer
        FspeedChangeTimer.start(FspeedChangeInterval);
    }

    /**
     * @brief set the actual motor speed
     */
    void setMotorSpeed(dtypes::uint16 _speed)
    {
        // tell the hardware the speed
        hardware().motor.targetSpeed_rpm = _speed;

        // check for issues
        if (hardware().motor.error != Thardware::TmotorError::none)
        {
            // there are motor errors
            return;
        }

        // update speed info
        FspeedNow = _speed;
        if (FspeedNow == FspeedTarget)
        {
            // reached target
            status = (FspeedNow > 0) ? Tstatus::running : Tstatus::off;
            if (event == Tevent::vortexing)
            {
                if (!vortexPeak)
                {
                    // we're vortexing and just reached the peak so let's start the vortex timer
                    vortexPeak = true;
                    FvortexEndTimer.start(settings.vortexTime_sec * 1000);
                }
                else
                {
                    // vortex event finished
                    vortexPeak = false;
                    event = Tevent::none;
                }
            }
            else if (event == Tevent::paused && FspeedNow > 0)
            {
                // pausing is done
                event = Tevent::none;
            }
        }
        else if (FspeedNow < FspeedTarget)
        {
            // still accelerating
            status = Tstatus::accelerating;
        }
        else if (FspeedNow > FspeedTarget)
        {
            // still decelerating
            status = Tstatus::decelerating;
        }
    }

public:
    // user actions
    sdds_var(Taction, action);

    // keeps track in memory where the microcontroller is at
    sdds_var(enums::ToffOn, state, sdds_joinOpt(sdds::opt::saveval, sdds::opt::readonly), enums::ToffOn::off);

    // details on what the motor is doing
    sdds_var(Tstatus, status, sdds::opt::readonly, Tstatus::off);

    // motor error
    sdds_var(Thardware::TmotorError, error, sdds::opt::readonly);

    // stir events
    sdds_var(Tevent, event, sdds::opt::readonly, Tevent::none);

    // speed details
    sdds_var(Tuint16, setpoint_rpm, sdds::opt::saveval, 500);
    sdds_var(Tuint16, speed_rpm, sdds::opt::readonly);

    // additional settings
    class Tsettings : public TmenuHandle
    {
    public:
        sdds_var(Tuint16, acceleration, sdds::opt::saveval, 500);  // rpm/second
        sdds_var(Tuint16, deceleration, sdds::opt::saveval, 3000); // rpm/second
        sdds_var(Tuint16, maxSpeed_rpm, sdds::opt::saveval, hardware().motor.maxSpeed_rpm);
        sdds_var(Tuint16, vortexSpeed_rpm, sdds::opt::saveval, 3000);
        sdds_var(Tuint16, vortexTime_sec, sdds::opt::saveval, 5);
    };
    sdds_var(Tsettings, settings);

    // constructor
    TcomponentStirrer()
    {

        // make sure hardware is initalized
        hardware();

        // action events
        on(action)
        {
            if (action == Taction::start)
            {
                state = enums::ToffOn::on;
                event = Tevent::none;
                changeSpeed(setpoint_rpm);
            }
            else if (action == Taction::stop)
            {
                state = enums::ToffOn::off;
                event = Tevent::none;
                changeSpeed(0);
            }
            else if (action == Taction::pause)
            {
                if (state == enums::ToffOn::on)
                {
                    // no state change but stopping the motor
                    event = Tevent::paused;
                    changeSpeed(0);
                }
            }
            else if (action == Taction::resume)
            {
                if (state == enums::ToffOn::on)
                {
                    // no state change but restarting the motor
                    changeSpeed(setpoint_rpm);
                }
            }
            else if (action == Taction::vortex)
            {
                // no state change but running the vortex
                event = Tevent::vortexing;
                vortexPeak = false;
                changeSpeed(settings.vortexSpeed_rpm);
            }
            if (action != Taction::___)
                action = Taction::___;
        };

        // vortex end timer
        on(FvortexEndTimer)
        {
            // resume where we were before the vortex
            (state == enums::ToffOn::on) ? changeSpeed(setpoint_rpm) : changeSpeed(0);
        };

        // update speed from hardware
        on(hardware().motor.measuredSpeed_rpm)
        {
            speed_rpm = hardware().motor.measuredSpeed_rpm;
        };

        // reset motor error when publish is turned on
        // this ensures it gets logged when the next error occurs
        on(particleSystem().publishing.publish)
        {
            if (!FisPublishing && particleSystem().publishing.publish == sdds::enums::OnOff::ON)
            {
                error = Thardware::TmotorError::none;
            }
            FisPublishing = particleSystem().publishing.publish == sdds::enums::OnOff::ON;
        };

        // update motor error from hardware
        on(hardware().motor.error)
        {
            if (hardware().motor.error != Thardware::TmotorError::none)
            {
                // error, stop operations
                if (error != hardware().motor.error)
                    error = hardware().motor.error;
                if (status != Tstatus::error)
                    status = Tstatus::error;
                if (event != Tevent::none)
                    event = Tevent::none;
                FvortexEndTimer.stop();
                FspeedChangeTimer.stop();
                // set to min speed so it keeps checking for connectivity
                // but also does not speed up too fast when error resolves
                FspeedNow = hardware().motor.minSpeed_rpm;
                setMotorSpeed(FspeedNow);
            }
            else if (error != Thardware::TmotorError::none)
            {
                // no error anymore --> resume state (this is in case errors not related to device connection get resolved)
                error = Thardware::TmotorError::none;
                resumeState();
            }
        };

        // change setpoint
        on(setpoint_rpm)
        {
            if (setpoint_rpm > settings.maxSpeed_rpm)
                setpoint_rpm = settings.maxSpeed_rpm;
            if (state == enums::ToffOn::on && event == Tevent::none)
            {
                // don't adjust if we're off, paused or vortexing
                changeSpeed(setpoint_rpm);
            }
        };

        // change vortex speed
        on(settings.vortexSpeed_rpm)
        {
            if (settings.vortexSpeed_rpm > settings.maxSpeed_rpm)
                settings.vortexSpeed_rpm = settings.maxSpeed_rpm;
            if (event == Tevent::vortexing && !vortexPeak)
            {
                // currently vortexing and not yet at the peak -- change to the new speed
                changeSpeed(settings.vortexSpeed_rpm);
            }
        };

        // change max speed
        on(settings.maxSpeed_rpm)
        {
            if (settings.maxSpeed_rpm > hardware().motor.maxSpeed_rpm)
            {
                // maxed out from hardware side
                settings.maxSpeed_rpm = hardware().motor.maxSpeed_rpm;
            }
            else
            {
                if (setpoint_rpm > settings.maxSpeed_rpm)
                    setpoint_rpm = settings.maxSpeed_rpm;
                if (settings.vortexSpeed_rpm > settings.maxSpeed_rpm)
                    settings.vortexSpeed_rpm = settings.maxSpeed_rpm;
            }
        };

        // on speed change timer
        on(FspeedChangeTimer)
        {
            adjustSpeed();
        };
    }

    // pause state if disconnected
    void pauseState()
    {
    }

    // resume the saved state of the motor after power cycling or reconnection
    void resumeState()
    {
        if (state == enums::ToffOn::on && (status == Tstatus::off || status == Tstatus::error))
        {
            // should be on but is not
            if (status == Tstatus::error)
                status = Tstatus::off; // reset from error
            changeSpeed(setpoint_rpm);
        }
        else if (state == enums::ToffOn::off && status != Tstatus::off)
        {
            // should be off but is not
            status = Tstatus::off;
            changeSpeed(0);
        }
    }
};