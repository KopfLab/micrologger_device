#pragma once

#include "uTypedef.h"
#include "uMultask.h"
#ifdef SDDS_ON_PARTICLE
#include "uParticleSystem.h"
#endif

/**
 * @brief hardware handler
 * uses a TmenuHandle for the convenience of sdds_vars but is not usually
 * included in the tree except for stand-alone testing
 */
class Thardware : public TmenuHandle{

    private:

        bool initialized = false;
		
        // in case it is used in a tree
		Tmeta meta() override { return Tmeta{TYPE_ID, 0, "hardware"}; }

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

    public:

        // stirrer motor
        class Tmotor : public TmenuHandle {

            private:

                // speed
                dtypes::uint16 FtargetSpeed = 0;
                Ttimer FspeedFinetuneTimer;

                // decoder for speed measurement
                TisrEvent decoderISR; 
                dtypes::TtickCount FdecoderStart = 0;
                dtypes::uint32 FdecoderCounter = 0;
                const dtypes::uint8 FdecoderPin = D6;

                void decoderChangeISR(){ 
                    decoderISR.signal(); 
                };

                void resetDecoder() {
                    FdecoderStart = millis();
                    FdecoderCounter = 0;
                }

            public:

                sdds_var(Tuint16, speed, sdds::opt::readonly);
                sdds_enum(none, noResponse) Terror;
                sdds_var(Terror, error, sdds::opt::readonly);
                
                Tmotor() {
                    // decoder interrupt triggered
                    on(decoderISR){
                        FdecoderCounter++;
                    };
                }

                void init() {
                    // attach the encoder interrupt
                    attachInterrupt(FdecoderPin, &Tmotor::decoderChangeISR, this, RISING);
                }

                void setSpeed(dtypes::uint16 _speed) {
                    FtargetSpeed = _speed;
                    // FIXME: set the speed and finetune with the speed measurement
                    // every time a new speed is set, the decoder resets
                    // calculate speed each time
                    speed = _speed; // FIXME read the actual speed
                    resetDecoder();
                }

                dtypes::uint16 getSpeed() {
                    // return the speed
                        // decoder info: rpm = freq (in Hz) / 100 * 60 = counts / dt.ms * 1000 * 1/100 * 60 = count / dt.ms * 600.
                    //float rpm = (600. * decoder_steps) / (millis() - last_speed);
                    return 0;
                }

        };

        sdds_var(Tmotor, motor);

        // beam
        class Tbeam : public TmenuHandle {

            private:

                // beam runs through a TCA9534 multiplexer over 1 wire commms
                bool initialized = false;
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

                sdds_enum(none, failedInit, failedSet) Terror;
                sdds_var(Terror, error, sdds::opt::readonly);

                /**
                 * @brief is beam on?
                 */
                bool isOn() {
                    return on;
                }

                bool init() {
                    initialized = true;
                    // check if device is present
                    Wire.beginTransmission(FmplexAddress);
                    if(Wire.endTransmission() != 0) {
                        error = Terror::e::failedInit;
                        return false;
                    }
                
                    // configure pin inputs/outputs (only beam pin as output)
                    FmplexPinConfig = FmplexPinConfig & ~(0x01 << FbeamPin);
                    if (!transmit(FmplexPinConfigRegister, FmplexPinConfig)) {
                        error = Terror::e::failedInit;
                        return false;
                    }

                    // set all pins to off
                    if (!transmit(FmplexPinStateRegister, FmplexPinState)) {
                        error = Terror::e::failedSet;
                        return false;
                    }
            
                    // successfully initialized, set error to none
                    error = Terror::e::none;
                    return true;
                }

                void turnOn() {
                    // init if needed
                    if (!initialized || error == Terror::e::failedInit) {
                        if (!init()) return;
                    }
                    // set if off or last command failed
                    if (!on || error == Terror::e::failedSet) {
                        FmplexPinState = FmplexPinState | (0x01 << FbeamPin);
                        on = transmit(FmplexPinStateRegister, FmplexPinState);
                        error = !on ? 
                            Terror::e::failedSet :
                            Terror::e::none;
                    }
                }

                void turnOff() {
                    // init if needed
                    if (!initialized || error == Terror::e::failedInit) {
                        if (!init()) return;
                    }
                    // set if on or last command failed
                    if (on || error == Terror::e::failedSet) {
                        FmplexPinState = FmplexPinState & ~(0x01 << FbeamPin);
                        on = !transmit(FmplexPinStateRegister, FmplexPinState);
                        error = on ? 
                            Terror::e::failedSet :
                            Terror::e::none;
                    }
                }

        };

        sdds_var(Tbeam, beam);

        Thardware() {

            #ifdef SDDS_ON_PARTICLE
                // if on a particle system, initialize once startup is complete
                on(particleSystem().startup) {
                    if (particleSystem().startup == TparticleSystem::TstartupStatus::e::complete) {
                        init();
                    }
                };
            #else
                // initialize during setup
                on(sdds::setup()) {
                    init();
                };
            #endif

        }

};

/**
 * @brief get the static hadware instance
 */
Thardware& hardware(){
    static Thardware hardware;
    return hardware;
}