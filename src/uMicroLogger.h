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
            
                // this should not be saved in state!
                sdds_var(TonOff, beam, 0, TonOff::e::OFF);
                sdds_var(Thardware::Tbeam::Terror, beamError, sdds::opt::readonly);

                TopticalDensity() {

                    // make sure hardware is initalized
                    hardware();

                    // reset beam error when publish is turned on
                    // this ensures it gets logged when a new error occurs
                    // FIXME: use the final implementation for change detection in sdds
                    on(particleSystem().publishing.publish) {
                        if (particleSystem().publishing.publish == TonOff::e::ON && _changed) {
                            beamError = Thardware::Tbeam::Terror::e::none;
                        }
                    };

                    // update beam error from hardware
                    on(hardware().beam.error) {
                       beamError = hardware().beam.error;
                    };

                    // turn beam on and off
                    on(beam) {
                        if (beam == TonOff::e::ON) {
                            hardware().beam.turnOn();
                            if (!hardware().beam.isOn()) {
                                Log.error("could not turn beam ON, check beamError");
                                // FIXME: ideally should set Fvalue or use .__setValue() with signal = false
                                // NOTE: only matters if beam status interval is set to 1 (always publish)
                                beam = TonOff::e::OFF; 
                            }
                        } else {
                            hardware().beam.turnOff();
                            if (hardware().beam.isOn()) {
                                Log.error("could not turn beam OFF, check beamError");
                                // FIXME: ideally should set Fvalue or use .__setValue() with signal = false
                                // NOTE: only matters if beam status interval is set to 1 (always publish)
                                beam = TonOff::e::ON;
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
            // add hardware menu item?
            // --> no just updating in the other menus instead
            //addDescr(hardware());
        }
};
