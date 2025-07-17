// self-describing data structure (SDDS) tree
#include "uMicroLogger.h"
TmicroLogger micrologger;

// serial spike for communication via serial (with baud rate)
#include "uSerialSpike.h"
TserialSpike serialSpike(micrologger, 115200);

// particle spike for particle communication (with sdds name and version)
#include "uParticleSpike.h"
static TparticleSpike particleSpike(micrologger, "project", 1);

// logging
SerialLogHandler logHandler(
    LOG_LEVEL_WARN, { // WARN for non-app messages
    { "app", LOG_LEVEL_TRACE } // TRACE for app
});

// setup
void setup(){
    // setup particle communication
    using publish = sdds::particle::publish;
    particleSpike.setup({
        // set default publishing intervals for anything that should be different from publish::OFF
        // --> all variables that are stored in EEPROM (saveeval option) should report all changes
        {publish::ALWAYS, sdds::opt::saveval},
        // --> all floats should inherit from the globalInterval
        {publish::GLOBAL, {sdds::Ttype::FLOAT32, sdds::Ttype::FLOAT64}},
        // --> stirrer speed should inherit from the globalInterval
        {publish::GLOBAL, &micrologger.stirrer.speed}
    });
}

// loop: task handler
#include "uMultask.h"
void loop(){
    // handle all events
    TtaskHandler::handleEvents();
}