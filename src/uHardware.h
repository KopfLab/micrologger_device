#pragma once

#include "uTypedef.h"

/**
 * @brief hardware handler
 */
class Thardware{

    private:

        bool initialized = false;

    public:

        // stirrer motor
        class Tmotor {

            public:

                void init() {
                }

                void setSpeed(dtypes::uint16) {
                    // FIXME: set the speed and finetune with the speed measurement
                }

                dtypes::uint16 getSpeed() {
                    // return the speed
                    // FIXME: should this habe a Tdescr* pointer to set whenever the
                    // speed is updated or should that happen on the otherside with
                    // an update timer? maybe tending towards the latte
                    return 0;
                }

        } motor;

        // beam
        class Tbeam {

            private:

                // beam runs through a TCA9534 multiplexer over 1 wire commms
                bool working = false;
                bool on = false;
                byte FmplexAddress = 0x20; // default TCA9534 address
                byte FmplexPinConfigRegister = 0x03; // pin config register
                byte FmplexPinStateRegister = 0x01; // pin state register
                uint8_t FmplexPinConfig = 0xFF; // all pins are inputs by default
                uint8_t FmplexPinState = 0x00; // all pins are off by default
                uint8_t FbeamPin = 0; // pin address

                bool transmit(byte _register, uint8_t _config) {
                    Wire.beginTransmission(FmplexAddress);
                    Wire.write(_register);
                    Wire.write(_config);
                    return Wire.endTransmission() == 0;
                }

            public:

                void init() {
                    // check if device is present
                    Wire.beginTransmission(FmplexAddress);
                    if(Wire.endTransmission() != 0) return;

                    // configure pin inputs/outputs (only beam pin as output)
                    FmplexPinConfig = FmplexPinConfig & ~(0x01 << FbeamPin);
                    if (!transmit(FmplexPinConfigRegister, FmplexPinConfig)) {
                        // couldn't set inputs/outputs mode
                        return;
                    }

                    // set all pins to off
                    if (!transmit(FmplexPinStateRegister, FmplexPinState)) {
                        // couldn't set all to off
                        return;
                    }
                    // fully initialized, setting working to true
                    working = true;
                }

                bool isWorking() {
                    return working;
                }

                bool turnOn() {
                    if (!isWorking()) return false;
                    if (!on) {
                        FmplexPinState = FmplexPinState | (0x01 << FbeamPin);
                        on = transmit(FmplexPinStateRegister, FmplexPinState);
                    }
                    return on;
                }

                bool turnOff() {
                    if (!isWorking()) return false;
                    if (on) {
                        FmplexPinState = FmplexPinState & ~(0x01 << FbeamPin);
                        on = !transmit(FmplexPinStateRegister, FmplexPinState);
                    }
                    return !on;
                }

        } beam;

        

        /**
         * @brief initialize the hardware
         */
        void init() {
            if (!initialized) {
                // initialize
                Wire.begin();
                motor.init();
                beam.init();
            }
        }

};

/**
 * @brief get the static hadware instance
 */
Thardware& hardware(){
    static Thardware hardware;
    return hardware;
}