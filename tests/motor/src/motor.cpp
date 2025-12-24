// this program tests the motor hardware
#include "Particle.h"
#include "uTypedef.h"
#include "uHardwareMotorNidec24H.h"

// manual mode, no wifi
SYSTEM_MODE(MANUAL);

// log handler
SerialLogHandler logHandler(LOG_LEVEL_TRACE);

// self-describing data structure (SDDS) tree
class TsddsTree : public TmenuHandle
{
private:
    Ttimer FtestingTimer;

public:
    // motor itself
    sdds_var(ThardwareMotor, motor);

    // testing vars
    sdds_enum(manual, testing) Tmode;
    sdds_var(Tmode, mode);
    sdds_enum(up, down) Tdirection;
    sdds_var(Tdirection, testingDirection);
    sdds_var(Tuint16, testingStepSize, sdds::opt::nothing, 128);
    sdds_var(Tuint16, testingStepWait, sdds::opt::nothing, 6200);

    // constructor
    TsddsTree()
    {

        // output the decoder reads
        on(motor.measuredSpeed)
        {
            Log.trace("step=%d, rep=%d, rpm=%d", motor.targetSteps.Fvalue, motor.speedCheckCounter, motor.measuredSpeed.Fvalue);
        };

        // testing mode
        on(mode)
        {
            // start/stop the tests
            if (mode == Tmode::testing)
            {
                motor.autoAdjust = ThardwareMotor::TonOff::OFF;
                FtestingTimer.start(testingStepWait);
            }
            else
            {
                motor.autoAdjust = ThardwareMotor::TonOff::ON;
                FtestingTimer.stop();
            }
        };

        on(FtestingTimer)
        {
            // continue?
            if (mode == Tmode::testing)
            {
                if (testingDirection == Tdirection::up && motor.targetSteps + testingStepSize > 4095)
                {
                    // at the max, change direction
                    testingDirection = Tdirection::down;
                }
                else if (testingDirection == Tdirection::down && motor.targetSteps < testingStepSize)
                {
                    // at the min, change direction
                    testingDirection = Tdirection::up;
                }
                // change steps
                motor.targetSteps = motor.targetSteps + testingStepSize * ((testingDirection == Tdirection::up) ? 1 : -1);
                // restart
                FtestingTimer.start(testingStepWait);
            }
        };
    };

} sddsTree;

// serial spike for communication via serial (with baud rate)
#include "uSerialSpike.h"
TserialSpike serialSpike(sddsTree, 115200);

// setup
void setup()
{
    // init with correct speed and decoder pins
    sddsTree.motor.init(A2, D6);

    // start testing mode
    sddsTree.motor.targetSteps = 20;
    sddsTree.mode = TsddsTree::Tmode::testing;
}

// loop
void loop()
{
    // handle all events
    TtaskHandler::handleEvents();
}