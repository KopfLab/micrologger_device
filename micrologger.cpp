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
    10800              // device version (1.0.0= 10000, 2.23.2 = 22302)
);

// stirrer speed data: the published (averaged) stirrer speed should only average steady stirring,
// every speed transition (acceleration/deceleration for pauses, vortexing, setpoint changes,
// start/stop) is delimited by single data points (n = 1) instead of being averaged in:
//  1. transition starts: the average so far (if it has at least 2 values), then the last speed
//     before the transition as a single point; averaging is suspended
//  2. transition finished + one motor read interval (so the measured speed is a fresh reading):
//     the speed as a single point; averaging resumes (also during a pause or at the vortex peak)
// These data points pause the burst timer, i.e. they ride along with the next regular burst instead
// of triggering additional publishes. The live value of speed_rpm is not affected.
class TstirrerSpeedData
{
    using Tstirrer = TcomponentStirrer;
    Ttimer FsettleTimer;
    bool Framping = false;

    static bool isRamping()
    {
        auto &status = micrologger.stirrer.status;
        return status == Tstirrer::Tstatus::accelerating || status == Tstirrer::Tstatus::decelerating;
    }

    static void point() { particleSpike.publishValue(micrologger.stirrer.speed_rpm, true); }

public:
    TstirrerSpeedData()
    {
        on(micrologger.stirrer.status)
        {
            auto &speed = micrologger.stirrer.speed_rpm;
            bool ramping = isRamping();
            if (ramping && !Framping)
            {
                // 1. transition starts
                if (FsettleTimer.running())
                {
                    // previous transition still settling (averaging not resumed yet) --> just the point
                    FsettleTimer.stop();
                }
                else
                {
                    particleSpike.suspendAveraging(speed, true);
                    particleSpike.publishAverage(speed, true, 2);
                }
                point();
            }
            else if (!ramping && Framping)
            {
                // transition finished --> wait for a fresh speed reading
                FsettleTimer.start(hardware().motor.readInterval_ms);
            }
            Framping = ramping;
        };
        on(FsettleTimer)
        {
            // 2. settled
            point();
            particleSpike.suspendAveraging(micrologger.stirrer.speed_rpm, false);
        };
    }
} stirrerSpeedData;

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
         // --> motor speed is not recorded by default (set SYSTEM.publishing.varIntervals_ms.stirrer.speed_rpm
         //     to -1 to record its average over the global interval with the transition data points, see above)
         {publish::OFF, &micrologger.stirrer.speed_rpm},
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