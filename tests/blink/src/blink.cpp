#include "Particle.h"
#include "uTypedef.h"
#include "uCoreEnums.h"
#include "uMultask.h"

// LED controller
class Tled : public TmenuHandle{

    using TonOff = sdds::enums::OnOff;
    Ttimer timer;
    public:

        // on by default
        sdds_var(TonOff, led, 0)
        sdds_var(TonOff, blink, 0, TonOff::e::ON)
        sdds_var(Tuint16, onTime, 0, 1000)
        sdds_var(Tuint16, offTime, 0, 1000)

        Tled(){
            pinMode(LED_BUILTIN, OUTPUT);

            on(led){
                (led == TonOff::e::ON) ?
                    digitalWrite(LED_BUILTIN, HIGH):
                    digitalWrite(LED_BUILTIN, LOW);
            };

            on(blink){
                (blink == TonOff::e::ON) ?
                    timer.start(0):
                    timer.stop();
            };

            on(timer){
                if (led == TonOff::e::ON){
                    led = TonOff::e::OFF;
                    timer.start(offTime);
                } else {
                    led = TonOff::e::ON;
                    timer.start(onTime);
                }
            };
        }
};

// self-describing data structure (SDDS) tree
class TsddsTree : public TmenuHandle{
    public:
        sdds_var(Tled,led)
        TsddsTree(){}
} sddsTree;

// serial spike for communication via serial (with baud rate)
#include "uSerialSpike.h"
TserialSpike serialSpike(sddsTree, 115200);

// particle spike for particle communication (with sdds name and version)
#include "uParticleSpike.h"
static TparticleSpike particleSpike(sddsTree, "blink", 1);

// setup
void setup(){
    // setup particle spike
    particleSpike.setup();
}

// loop
void loop(){
    // handle all events
    TtaskHandler::handleEvents();
}
