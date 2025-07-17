#pragma once
#include "uTypedef.h"
#include "uCoreEnums.h"

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
        const dtypes::TtickCount FspeedChangeInterval = 100; // ms
        Ttimer FspeedChangeTimer;
        Ttimer FvortexEndTimer;

        /**
         * @brief set a new target speed
         */
        void changeSpeed(dtypes::uint16 _speed) {
            FspeedChangeTimer.stop();
            FspeedTarget = _speed;
            adjustSpeed();
        }

        /**
         * @brief adjust speed towards the speed target
         */
        void adjustSpeed() {
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
                // time to accelerate
                setMotorSpeed(FspeedNow + change);
            } else if (FspeedNow > FspeedTarget) {
                if (FspeedNow < FspeedTarget + change) {
                    // reached target speed
                    setMotorSpeed(FspeedTarget);
                    return;
                }
                // time to decelerate
                setMotorSpeed(FspeedNow - change);
            }
            // there's more to do
            FspeedChangeTimer.start(FspeedChangeInterval);
        }

        /**
         * @brief set the actual motor speed
         */
        void setMotorSpeed(dtypes::uint16 _speed) {
            FspeedNow = _speed;
            if (FspeedNow == FspeedTarget) {
                // reached target
                if (motor == Tmotor::e::vortexing) {
                    // we were vortexing so let's start the vortex timer if it isn't already running
                    if (!FvortexEndTimer.running())
                        FvortexEndTimer.start(settings.vortexTimeS * 1000);
                }
            }
            // FIXME: this is just to debug, this should actually show the measured speed
            speed = _speed;
        }

    public:

        // keeps track in memory where the microcontroller is at
        sdds_var(TonOff, state, sdds_joinOpt(sdds::opt::saveval, sdds::opt::readonly), TonOff::e::OFF);

        // details on what the motor is doing
        sdds_enum(idle, running, paused, vortexing) Tmotor; 
        sdds_var(Tmotor, motor, sdds::opt::readonly, Tmotor::e::idle);

        // user actions
        sdds_enum(___, start, stop, vortex) Taction;
        sdds_var(Taction, action);
        
        // speed details
        sdds_var(Tuint16, setpoint, sdds::opt::saveval, 0);
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

            // startup/state change
            on(state) {
                (state == TonOff::e::ON) ? start() : stop();
            };

            // action events
            on(action) {
                if (action == Taction::e::start) {
                    // state change
                    state = TonOff::e::ON;
                } else if (action == Taction::e::stop) {
                    // state change
                    state = TonOff::e::OFF;
                } else if (action == Taction::e::vortex) {
                    // no state change but running a vortex
                    vortex();
                }
                if (action != Taction::e::___) action = Taction::e::___;
            };

            // change setpoint
            on(setpoint) {
                if (setpoint > settings.maxSpeed) setpoint = settings.maxSpeed;
                if (state == TonOff::e::ON && motor == Tmotor::e::running) {
                    // don't adjust if we're off, paused or vortexing
                    changeSpeed(setpoint);
                }
            };

            // change vortex speed
            on(settings.vortexSpeed) {
                if (settings.vortexSpeed > settings.maxSpeed) settings.vortexSpeed = settings.maxSpeed;
                if (motor == Tmotor::e::vortexing) {
                    // currently vortexing -- change the speed
                    vortex();
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

            // vortex end timer
            on(FvortexEndTimer) {
                (state == TonOff::e::ON) ? start() : stop();
            };
        }

        // actions
        void start() {
            FvortexEndTimer.stop();
            motor = Tmotor::e::running;
            changeSpeed(setpoint);
        }

        void stop() {
            FvortexEndTimer.stop();
            motor = Tmotor::e::idle;
            changeSpeed(0);
        }

        void pause() {
            if (state == TonOff::e::ON) {
                FvortexEndTimer.stop();
                motor = Tmotor::e::paused;
                changeSpeed(0);
            }
        }

        void resume() {
            if (state == TonOff::e::ON) start();
        }

        void vortex() {
            FvortexEndTimer.stop();
            motor = Tmotor::e::vortexing;
            changeSpeed(settings.vortexSpeed);
        }

};