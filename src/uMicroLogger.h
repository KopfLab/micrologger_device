#pragma once

#include "uTypedef.h"
#include "uStirrer.h"
#include "uHardware.h"

/**
 * @brief 
 */
class TmicroLogger : public TmenuHandle {

    private:

        // on off state
        using TonOff = sdds::enums::OnOff;

    public:

        /**
         * @brief beam class
         */
        class TopticalDensity : public TmenuHandle{

            public:

                // this should not be saved
                sdds_var(TonOff, beam, 0, TonOff::e::OFF);

                TopticalDensity() {

                    // setup
                    on(sdds::setup()) {
                        // initialize hardware
                        hardware().init();
                        if (!hardware().beam.isWorking()) {
                            // FIXME: what to do if the beam is not working?
                        }
                    };

                    // turn beam on and off
                    on(beam) {
                        if (beam == TonOff::e::ON) {
                            if (!hardware().beam.turnOn()) {
                                // FIXME: what to do if turning on the beam is not working?
                            }
                        } else {
                            if (!hardware().beam.turnOff()) {
                                // FIXME: what to do if turning off the beam is not working
                            }
                        }
                            
                    };
                }

        };

        /*** stirrer  ***/
        #pragma region stirrer

        sdds_var(Tstirrer, stirrer);
        sdds_var(TopticalDensity, OD);

        #pragma endregion

        /*** OD reader ***/
        #pragma region OD



        #pragma endregion


        TmicroLogger() {
        }
};
