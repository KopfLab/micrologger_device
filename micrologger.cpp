// #define SDDS_PARTICLE_DEBUG 1

// self-describing data structure (SDDS) tree
#include "uMicroLogger.h"
TmicroLogger micrologger;

// serial spike for communication via serial (with baud rate)
#include "uSerialSpike.h"
TserialSpike serialSpike(micrologger, 115200);

// particle spike for particle communication (with sdds name and version)
#include "uParticleSpike.h"
static TparticleSpike particleSpike(
    micrologger,       // SDDS tree
    "micrologger",     // device type
    "microloggerData", // data event
    10501              // device version (1.0.0= 10000, 2.23.2 = 22302)
);

// logging
SerialLogHandler logHandler(
    LOG_LEVEL_WARN, // WARN for non-app messages
    {
        {"app",
#ifdef SDDS_PARTICLE_DEBUG
         LOG_LEVEL_TRACE
#else
         LOG_LEVEL_INFO
#endif
        }});

// setup
void setup()
{
    // setup particle communication
    using publish = TparticleSpike::publish;
    particleSpike.setup(
        {// set default publishing intervals for anything that should be different from publish::OFF
         // --> all variables that are stored in EEPROM (saveeval option) should report all changes
         {publish::EACH, sdds::opt::saveval},
         // --> last save should always be reported
         {publish::ALWAYS, &particleSystem().state.lastSave_dt},
         // --> device connect/disconnect should be published immediately
         {publish::EACH, &micrologger.device},
         // --> top level errors should always be published
         {publish::EACH, &micrologger.stirrer.error},
         {publish::EACH, &micrologger.lights.error},
         {publish::EACH, &micrologger.sensor.error},
         {publish::EACH, &micrologger.environment.error},
         // --> all floats should inherit from the globalInterval
         {publish::AVG_GLOBAL, {sdds::Ttype::FLOAT32, sdds::Ttype::FLOAT64}},
         // --> motor speed is an integer but should still inherit from the globalInterval
         {publish::AVG_GLOBAL, &micrologger.stirrer.speed_rpm},
         // --> don't report transmittance, and power, and standard devs by default
         {publish::OFF, &micrologger.environment.power_V},
         {publish::OFF, &micrologger.environment.powerReq_V},
         {publish::OFF, &micrologger.sensor.reading.transmittance},
         {publish::OFF, &micrologger.sensor.reading.transmittanceSd},
         {publish::OFF, &micrologger.sensor.reading.ODSd}});

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