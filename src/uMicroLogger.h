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

            private:

                // keep track of publishing to detect when it switches from OFF to ON
		        bool FisPublishing = particleSystem().publishing.publish == TonOff::ON;

            public:
            
                // this should not be saved in state!
                sdds_var(TonOff, beam, 0, TonOff::OFF);
                sdds_var(Thardware::Tbeam::Terror, beamError, sdds::opt::readonly);

                TopticalDensity() {

                    // make sure hardware is initalized
                    hardware();

                    // reset beam error when publish is turned on
                    // this ensures it gets logged when a new error occurs
                    on(particleSystem().publishing.publish) {
                        if (!FisPublishing && particleSystem().publishing.publish == TonOff::ON) {
                            beamError = Thardware::Tbeam::Terror::none;
                        }
                        FisPublishing = particleSystem().publishing.publish == TonOff::ON;
                    };

                    // update beam error from hardware
                    on(hardware().beam.error) {
                       beamError = hardware().beam.error;
                    };

                    // turn beam on and off
                    on(beam) {
                        if (beam == TonOff::ON) {
                            hardware().beam.turnOn();
                            if (!hardware().beam.isOn()) {
                                Log.error("could not turn beam ON, check beamError");
                                beam = TonOff::OFF; 
                            }
                        } else {
                            hardware().beam.turnOff();
                            if (hardware().beam.isOn()) {
                                Log.error("could not turn beam OFF, check beamError");
                                beam = TonOff::ON;
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
