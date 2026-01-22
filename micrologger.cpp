#define SDDS_PARTICLE_DEBUG 1

// self-describing data structure (SDDS) tree
#include "uMicroLogger.h"
TmicroLogger micrologger;

// serial spike for communication via serial (with baud rate)
#include "uSerialSpike.h"
TserialSpike serialSpike(micrologger, 115200);

// particle spike for particle communication (with sdds name and version)
#include "uParticleSpike.h"
static TparticleSpike particleSpike(
    micrologger,   // SDDS tree
    "micrologger", // device type
    1              // device version
);

// logging
SerialLogHandler logHandler(
    LOG_LEVEL_WARN, // WARN for non-app messages
    {
        {"app", LOG_LEVEL_TRACE} // TRACE for app
    });

// setup
void setup()
{
    // setup particle communication
    using publish = TparticleSpike::publish;
    particleSpike.setup(
        {// set default publishing intervals for anything that should be different from publish::OFF
         // --> all variables that are stored in EEPROM (saveeval option) should report all changes
         {publish::IMMEDIATELY, sdds::opt::saveval},
         // --> device connect/disconnect should be published immediately
         {publish::IMMEDIATELY, &micrologger.device},
         // --> errors should always be published
         {publish::IMMEDIATELY, &micrologger.stirrer.error},
         {publish::IMMEDIATELY, &micrologger.lights.error},
         //{publish::IMMEDIATELY, &micrologger.OD.beamError},
         // --> all floats should inherit from the globalInterval
         {publish::INHERIT, {sdds::Ttype::FLOAT32, sdds::Ttype::FLOAT64}},
         // --> motor speed is an integer but should still inherit from the globalInterval
         {publish::INHERIT, &micrologger.stirrer.speed_rpm}});

    // add hardware menu after the particle spike so it does not have publish options
    micrologger.addDescr(hardware());
}

// loop: task handler
#include "uMultask.h"
void loop()
{
    // handle all events
    TtaskHandler::handleEvents();
}