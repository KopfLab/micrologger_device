#pragma once

#include "uTypedef.h"
#include "uStirrer.h"
#include "Wire.h"

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

                // BEGIN HARDWARE CONFIG could be abstracted out //
                // beam runs through a TCA9534 multiplexer over 1 wire commms
                byte FmplexAddress = 0x20; // default TCA9534 address
                byte FmplexPinConfigRegister = 0x03; // pin config register
                byte FmplexPinStateRegister = 0x01; // pin state register
                uint8_t FmplexPinConfig = 0xFF; // all pins are inputs by default
                uint8_t FmplexPinState = 0x00; // all pins are off by default
                uint8_t FbeamPin = 0; // pin address

                bool init() {
                    // initialize
                    Wire.begin();

                    // check if device is present
                    Wire.beginTransmission(FmplexAddress);
                    if(Wire.endTransmission() != 0) return false;

                    // configure pin inputs/outputs (only beam pin as output)
                    FmplexPinConfig = FmplexPinConfig & ~(0x01 << FbeamPin);
                    if (!transmit(FmplexPinConfigRegister, FmplexPinConfig)) {
                        // couldn't set inputs/outputs mode
                        return false;
                    }

                    // set all pins to off
                    if (!transmit(FmplexPinStateRegister, FmplexPinState)) {
                        // couldn't set all to off
                        return false;
                    }
                    return true;
                }

                bool transmit(byte _register, uint8_t _config) {
                    Wire.beginTransmission(FmplexAddress);
                    Wire.write(_register);
                    Wire.write(_config);
                    return Wire.endTransmission() == 0;
                }

                bool turnBeamOn() {
                    FmplexPinState = FmplexPinState | (0x01 << FbeamPin);
                    return transmit(FmplexPinStateRegister, FmplexPinState);
                }

                bool turnBeamOff() {
                    FmplexPinState = FmplexPinState & ~(0x01 << FbeamPin);
                    return transmit(FmplexPinStateRegister, FmplexPinState);
                }
                // END HARDWARE CONFIG //

            public:

                // this should not be saved
                sdds_var(TonOff, beam, 0, TonOff::e::OFF);

                TopticalDensity() {

                    // setup
                    on(sdds::setup()) {
                        init();
                    };

                    // turn beam on and off
                    // FIXME: what to do if there is a failed beam on/off?
                    on(beam) {
                        if(beam == TonOff::e::ON) 
                            Log.info("beam on %s", turnBeamOn() ? "success" : "fail");
                        else 
                            Log.info("beam off %s", turnBeamOff() ? "success" : "fail");
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
