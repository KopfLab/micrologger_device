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
class Tstirrer : public TmenuHandle{

    private:

        // on off state
        using TonOff = sdds::enums::OnOff;
        
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
        void changeSpeed(dtypes::uint16 _speed) {
            FvortexEndTimer.stop();
            FspeedChangeTimer.stop();
            FspeedTarget = _speed;
            adjustSpeed();
        }

        /**
         * @brief adjust speed towards the speed target
         */
        void adjustSpeed() {

            if (FspeedNow == FspeedTarget) {
                // already at target, nothing to do
                return;
            }

            if (settings.acceleration == 0) {
                // no accelerationg setting, change speed immediately
                setMotorSpeed(FspeedTarget);
                return;
            }

            // change speed by the acceleration amount
            dtypes::uint16 change = settings.acceleration * FspeedChangeInterval / 1000;
            if (FspeedNow < FspeedTarget) {
                if (FspeedNow + change > FspeedTarget) {
                    // reached target speed
                    setMotorSpeed(FspeedTarget);
                    return;
                }
                // time to increase the speed
                setMotorSpeed(FspeedNow + change);
            } else if (FspeedNow > FspeedTarget) {
                if (FspeedNow < FspeedTarget + change) {
                    // reached target speed
                    setMotorSpeed(FspeedTarget);
                    return;
                }
                // time to decrease the speed
                setMotorSpeed(FspeedNow - change);
            }
            // check for issues
            if (hardware().motor.error != Thardware::Tmotor::Terror::e::none) {
                return; // there are motor errors
            }

            // there's more to do, start timer
            FspeedChangeTimer.start(FspeedChangeInterval);
        }

        /**
         * @brief set the actual motor speed
         */
        void setMotorSpeed(dtypes::uint16 _speed) {
            // tell the hardware the speed (it will fine tune)
            hardware().motor.setSpeed(_speed);

            // check for issues
            if (hardware().motor.error != Thardware::Tmotor::Terror::e::none) {
                // there are motor errors
                return; 
            }

            // update speed info
            FspeedNow = _speed;
            if (FspeedNow == FspeedTarget) {
                // reached target
                motor = (FspeedNow > 0) ? Tmotor::e::running : Tmotor::e::idle;
                if (event == TstirEvent::e::vortexing) {
                    if (!vortexPeak) {
                        // we're vortexing and just reached the peak so let's start the vortex timer
                        vortexPeak = true;
                        FvortexEndTimer.start(settings.vortexTimeS * 1000);
                    } else {
                        // vortex event finished
                        vortexPeak = false;
                        event = TstirEvent::e::none;
                    }
                } else if (event == TstirEvent::e::pausing && FspeedNow > 0) {
                    // pausing is done
                    event = TstirEvent::e::none;
                }
            } else if (FspeedNow < FspeedTarget) {
                // still accelerating
                motor = Tmotor::e::accelerating;
            } else if (FspeedNow > FspeedTarget) {
                // still decelerating
                motor = Tmotor::e::decelerating;
            }
        }

    public:

        // user actions
        sdds_enum(___, start, stop, pause, resume, vortex) Taction;
        sdds_var(Taction, action);

        // keeps track in memory where the microcontroller is at
        sdds_var(TonOff, state, sdds_joinOpt(sdds::opt::saveval, sdds::opt::readonly), TonOff::e::OFF);

        // details on what the motor is doing
        sdds_enum(idle, accelerating, decelerating, running, error) Tmotor; 
        sdds_var(Tmotor, motor, sdds::opt::readonly, Tmotor::e::idle);

        // motor error
        sdds_var(Thardware::Tmotor::Terror, motorError, sdds::opt::readonly);

        // stir events
        sdds_enum(none, pausing, vortexing) TstirEvent;
        sdds_var(TstirEvent, event, sdds::opt::readonly, TstirEvent::e::none);
        
        // speed details
        sdds_var(Tuint16, setpoint, sdds::opt::saveval, 500);
        sdds_var(Tuint16, speed, sdds::opt::readonly); 
        sdds_var(Tstring, unit, sdds::opt::readonly, "rpm");
        
        // additional settings
        class Tsettings : public TmenuHandle{
            public:
            sdds_var(Tuint16, acceleration, sdds::opt::saveval, 500); // rpm/second
            sdds_var(Tuint16, maxSpeed, sdds::opt::saveval, 5000);
            sdds_var(Tuint16, vortexSpeed, sdds::opt::saveval, 3000);
            sdds_var(Tuint16, vortexTimeS, sdds::opt::saveval, 5);
        };
        sdds_var(Tsettings, settings);

        Tstirrer() {

            // make sure hardware is initalized
            hardware();

            #ifdef SDDS_ON_PARTICLE
                // if on a particle system, start motor when startup is complete
                on(particleSystem().startup) {
                    if (particleSystem().startup == TparticleSystem::TstartupStatus::e::complete) {
                        (state == TonOff::e::ON) ? changeSpeed(setpoint) : changeSpeed(0);
                    }
                };
            #else
                // start motor during setup
                on(sdds::setup()) {
                    (state == TonOff::e::ON) ? changeSpeed(setpoint) : changeSpeed(0);
                };
            #endif

            // action events
            on(action) {
                if (action == Taction::e::start) {
                    state = TonOff::e::ON;
                    event = TstirEvent::e::none;
                    changeSpeed(setpoint);
                } else if (action == Taction::e::stop) {
                    state = TonOff::e::OFF;
                    event = TstirEvent::e::none;
                    changeSpeed(0);
                } else if (action == Taction::e::pause) {
                    if (state == TonOff::e::ON) {
                        // no state change but stopping the motor
                        event = TstirEvent::e::pausing;
                        changeSpeed(0);
                    }
                } else if (action == Taction::e::resume) {
                    if (state == TonOff::e::ON) {
                        // no state change but restarting the motor
                        changeSpeed(setpoint);
                    }
                } else if (action == Taction::e::vortex) {
                    // no state change but running the vortex
                    event = TstirEvent::e::vortexing;
                    vortexPeak = false;
                    changeSpeed(settings.vortexSpeed);
                }
                if (action != Taction::e::___) action = Taction::e::___;
            };

            // vortex end timer
            on(FvortexEndTimer) {
                // resume where we were before the vortex
                (state == TonOff::e::ON) ? changeSpeed(setpoint) : changeSpeed(0);
            };

            // update speed from hardware
            on(hardware().motor.speed) {
                speed = hardware().motor.speed;
            };

            // reset motor error when publish is turned on
            // this ensures it gets logged when the next error occurs
            // FIXME: use the final implementation for change detection in sdds
            on(particleSystem().publishing.publish) {
                if (particleSystem().publishing.publish == TonOff::e::ON && _changed) {
                    motorError = Thardware::Tmotor::Terror::e::none;
                }
            };

            // update motor error from hardware
            on(hardware().motor.error) {
                motorError = hardware().motor.error;
                if (motorError != Thardware::Tmotor::Terror::e::none) {
                    // error, stop operations
                    FvortexEndTimer.stop();
                    FspeedChangeTimer.stop();
                    motor = Tmotor::e::error;
                    event = TstirEvent::e::none;
                    if (!FrestartTimer.running())
                        FrestartTimer.start(FrestartDelay);
                }
            };

            // restart motor after error
            on(FrestartTimer) {
                (state == TonOff::e::ON) ? changeSpeed(setpoint) : changeSpeed(0);
            };

            // change setpoint
            on(setpoint) {
                if (setpoint > settings.maxSpeed) setpoint = settings.maxSpeed;
                if (state == TonOff::e::ON && event == TstirEvent::e::none) {
                    // don't adjust if we're off, paused or vortexing
                    changeSpeed(setpoint);
                }
            };

            // change vortex speed
            on(settings.vortexSpeed) {
                if (settings.vortexSpeed > settings.maxSpeed) settings.vortexSpeed = settings.maxSpeed;
                if (event == TstirEvent::e::vortexing && !vortexPeak) {
                    // currently vortexing and not yet at the peak -- change to the new speed
                    changeSpeed(settings.vortexSpeed);
                }
            };

            // change max speed
            on(settings.maxSpeed) {
                if (setpoint > settings.maxSpeed) setpoint = settings.maxSpeed;
                if (settings.vortexSpeed > settings.maxSpeed) settings.vortexSpeed = settings.maxSpeed;
            };

            // on speed change timer
            on(FspeedChangeTimer) {
                adjustSpeed();
            };

        }

};