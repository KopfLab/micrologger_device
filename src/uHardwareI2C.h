
/**
 * Base class for I2C connected devices.
 * Takes care of Wire.begin() and generally connectivity.
 * Definese 3 error codes for use in derived classes: failedRed, failedWrite, failedCheck
 * failedRed and failedWrite will disconnect the bus
 */
#pragma once

#include "uTypedef.h"
#include "uCoreEnums.h"
#include "Particle.h"

/**
 * @brief basic I2C class --> override write/read in derived classes and trigger all actions with action = Taction::connect/write/read/disconnect
 */
// basic I2C class
// @brief: override
class ThardwareI2C : public TmenuHandle
{

private:
    // global Wire initialization
    static bool FwireInitialized; // shared across ALL instances & derived classes
    static void initializeWire()
    {
        if (!FwireInitialized)
        {
            Wire.begin(); // do any global I2C setup here
            FwireInitialized = true;
        }
    }

    // how long should it maximally take to get an answer from the I2C [in ms]
    uint16_t i2cMaxResponseTime = 200;

    // connection activity and checks
    Ttimer FautoConnectTimer;

    /**
     * @brief checks if there is a connection to the I2C device
     */
    bool checkConnection()
    {
        // device no initialized
        if (!Finitialized)
            return false;

        // lock for thread safety
        WITH_LOCK(Wire)
        {
            unsigned long requestStart = millis();
            Wire.beginTransmission(Fi2cAddress);
            uint8_t transmitCode = Wire.endTransmission();
            if (transmitCode == SYSTEM_ERROR_NONE)
            {
                // success
                if (status != Tstatus::connected)
                {
                    connections++;
                    status = Tstatus::connected;
                }
                return true;
            }
            else if (millis() - requestStart > i2cMaxResponseTime)
            {
                // special case: response took so long the I2C is likely broken
                Log.trace("timed out when trying connect to I2C device at 0x%x (there is likely a bigger issue with the I2C bus) - error code %d", Fi2cAddress, transmitCode);
                if (error != Terror::damagedI2CBus)
                    error = Terror::damagedI2CBus;
            }
            else
            {
                // connection failed for some other reason
                // error codes: https://docs.particle.io/reference/device-os/api/wire-i2c/endtransmission/
                Log.trace("could not connect to I2C device at 0x%x - error code %d", Fi2cAddress, transmitCode);
                if (error != Terror::failedConnect)
                    error = Terror::failedConnect;
            }
        }
        return false;
    }

    // read from I2C device
    virtual bool read()
    {
        return false;
    }

    // write to I2C device
    virtual bool write()
    {
        return false;
    }

protected:
    // connect to I2C (init() needs to get called first)
    virtual bool connect()
    {
        // are we initialized?
        if (!Finitialized)
        {
            Log.trace("I2C device is NOT initialized yet! make sure to call init() before trying to connect.");
            return false;
        }

        return checkConnection();
    }

    // protected information (address and initialization status)
    uint8_t Fi2cAddress;
    bool Finitialized = false;

    // helper function to assemble register byte
    static uint8_t bitsToByte(
        bool b0, bool b1, bool b2, bool b3,
        bool b4, bool b5, bool b6, bool b7)
    {
        return (static_cast<uint8_t>(b0) << 0) |
               (static_cast<uint8_t>(b1) << 1) |
               (static_cast<uint8_t>(b2) << 2) |
               (static_cast<uint8_t>(b3) << 3) |
               (static_cast<uint8_t>(b4) << 4) |
               (static_cast<uint8_t>(b5) << 5) |
               (static_cast<uint8_t>(b6) << 6) |
               (static_cast<uint8_t>(b7) << 7);
    }

    // helper funciton to check if a bit is set
    static bool isBitSet(uint8_t _byte, uint8_t _bit)
    {
        return _bit < 8 && (_byte >> _bit) & 0x01;
    }

    // helper function to print register byte
    static String byteBits(uint8_t _b, const char _0 = '0', const char _1 = '1', bool _reverse = true)
    {
        String s;
        s.reserve(15);
        int i = (_reverse) ? 7 : 0;
        while (i >= 0 && i <= 7)
        {
            if ((_reverse && i < 7) || (!_reverse && i > 0))
                s += ' ';
            s += ((_b >> i) & 1) ? _1 : _0;
            (_reverse) ? i-- : i++;
        }
        return s;
    }

public:
    // sdds vars
    using TonOff = sdds::enums::OnOff;
    sdds_enum(___, connect, disconnect, read, write) Taction;
    sdds_var(Taction, action);
    sdds_enum(disconnected, connected) Tstatus;
    sdds_var(Tstatus, status, sdds::opt::readonly);
    sdds_var(TonOff, autoConnect, sdds::opt::nothing, TonOff::OFF);
    sdds_var(Tuint16, checkInterval_MS, sdds::opt::nothing, 1000);
    sdds_enum(none, damagedI2CBus, failedConnect, failedRed, failedWrite) Terror;
    sdds_var(Terror, error, sdds::opt::readonly);
    sdds_var(Tuint32, connections, sdds::opt::readonly, 0);
    sdds_var(Tuint32, writes, sdds::opt::readonly, 0);
    sdds_var(Tuint32, reads, sdds::opt::readonly, 0);
    sdds_var(Tuint32, errors, sdds::opt::readonly, 0);

    // constructor
    ThardwareI2C()
    {
        // basic i2c actions
        on(action)
        {
            // trigger actions (override connect/read/write methods in derived classes)
            if (action != Taction::___)
            {
                bool success = false;

                if (action == Taction::connect)
                {
                    success = connect();
                    if (!success && error == Terror::none)
                        error = Terror::failedConnect;
                }
                else if (action == Taction::disconnect)
                {
                    success = true;
                    status = Tstatus::disconnected;
                }
                else if (action == Taction::read)
                {
                    success = read();
                    if (success)
                        reads++;
                    else if (!success && error == Terror::none)
                        error = Terror::failedRed;
                }
                else if (action == Taction::write)
                {
                    success = write();
                    if (success)
                        writes++;
                    else if (!success && error == Terror::none)
                        error = Terror::failedWrite;
                }

                // was the action successful?
                if (success)
                    error = Terror::none;
                action = Taction::___;
            }
        };

        // connection
        on(status)
        {
            // write and read the values if we're autoconnecting
            if (autoConnect == TonOff::ON)
            {
                if (status == Tstatus::connected)
                    action = Taction::write;
                if (status == Tstatus::connected)
                    action = Taction::read;
            }
        };

        on(autoConnect)
        {
            // start connection checks?
            if (Finitialized && autoConnect == TonOff::ON)
            {
                FautoConnectTimer.start(checkInterval_MS);
            }
            else
            {
                FautoConnectTimer.stop();
            }
        };

        on(FautoConnectTimer)
        {
            if (Finitialized && autoConnect == TonOff::ON)
            {
                if (status == Tstatus::connected)
                    checkConnection(); // check connection
                else
                    action = Taction::connect; // start connection
                // restart timer
                FautoConnectTimer.start(checkInterval_MS);
            }
        };

        // error
        on(error)
        {
            // mark as disconnected whenever there is an error
            if (error != Terror::none)
            {
                errors++;
                if (status != Tstatus::disconnected)
                    status = Tstatus::disconnected;
            }
        };
    }

    // initialize
    void init(uint8_t _i2cAddress)
    {
        if (!Finitialized)
        {
            initializeWire();
            Fi2cAddress = _i2cAddress;
            Finitialized = true;

            // start auto connect check timer
            if (autoConnect == TonOff::ON)
                FautoConnectTimer.start(checkInterval_MS);
        }
    }
};

// define the static wire initialized
bool ThardwareI2C::FwireInitialized = false;