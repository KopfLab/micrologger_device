
/**
 * Base class for I2C connected devices.
 * Takes care of Wire.begin() and generally connectivity.
 * Definese 3 error codes for use in derived classes: failedRead, failedWrite, failedCheck
 * failedRead and failedWrite will disconnect the bus
 */
#pragma once

#include "uTypedef.h"
#include "Particle.h"

// basic I2C class
class ThardwareI2C : public TmenuHandle
{

protected:
    bool Finitialized = false;
    byte Fi2cAddress;

    // Wire
    static bool FwireInitialized; // shared across ALL instances & derived classes
    static void initializeWire()
    {
        if (!FwireInitialized)
        {
            Wire.begin(); // do any global I2C setup here
            FwireInitialized = true;
        }
    }

    // helper function to print register bits
    static String byteBits(uint8_t b)
    {
        String s;
        s.reserve(9);
        for (int i = 7; i >= 0; i--)
        {
            s += ((b >> i) & 1) ? '1' : '0';
            if (i == 4)
                s += ' ';
        }
        return s;
    }

    /**
     * @brief reset the state of the I2C device (usually returning assumed values to the device defaults)
     */
    virtual void reset()
    {
    }

private:
    // how long should it maximally take to get an answer from the I2C [in ms]
    uint16_t i2cMaxResponseTime = 200;

public:
    // sdds vars
    sdds_enum(___, connect, disconnect, read, write) Taction;
    sdds_var(Taction, action);
    sdds_enum(disconnected, connected) Tstatus;
    sdds_var(Tstatus, status, sdds::opt::readonly);
    sdds_enum(none, damagedI2CBus, failedConnect, failedRead, failedWrite, failedCheck) Terror;
    sdds_var(Terror, error, sdds::opt::readonly);
    sdds_var(Tuint32, connections, sdds::opt::readonly, 0);
    sdds_var(Tuint32, connectErrors, sdds::opt::readonly, 0);

    // constructor
    ThardwareI2C()
    {
        // basic i2c actions
        on(action)
        {
            if (action == Taction::connect)
            {
                connect();
                action = Taction::___;
            }
            else if (action == Taction::disconnect)
            {
                disconnect();
                action = Taction::___;
            }
        };

        // error
        on(error)
        {
            if (isConnected() && error != Terror::none && error != Terror::failedCheck)
            {
                // disconnect on error to make sure reconnect starts from scratch
                // except for failedCheck which means the read was okay but the values
                // are inconsistent for some reason (we still know the right values though)
                disconnect();
            }
        };
    }

    // initialize
    void init(byte _i2cAddress)
    {
        if (!Finitialized)
        {
            initializeWire();
            Fi2cAddress = _i2cAddress;
            Finitialized = true;
        }
    }

    // connect to I2C
    virtual bool connect()
    {
        // are we initialized?
        if (!Finitialized)
            return false;

        // are we already connected?
        if (isConnected())
            return true;

        // lock for thread safety
        WITH_LOCK(Wire)
        {
            unsigned long requestStart = millis();
            Wire.beginTransmission(Fi2cAddress);
            byte transmitCode = Wire.endTransmission();
            if (transmitCode == SYSTEM_ERROR_NONE)
            {
                // success
                error = Terror::none;
            }
            else if (millis() - requestStart > i2cMaxResponseTime)
            {
                // response took so long the I2C is likely broken
                connectErrors++;
                Log.trace("timed out when trying connect to I2C device at 0x%x (there is likely a bigger issue with the I2C bus) - error code %d", Fi2cAddress, transmitCode);
                error = Terror::damagedI2CBus;
                return false;
            }
            else
            {
                // connection failed for some other reason
                connectErrors++;
                Log.trace("could not connect to I2C device at 0x%x - error code %d", Fi2cAddress, transmitCode);
                error = Terror::failedConnect;
                return false;
            }
        }

        // success --> flag as connected
        status = Tstatus::connected;
        connections++;
        return true;
    }

    /**
     * @brief convenience function to check for connected status
     */
    bool isConnected()
    {
        return status == Tstatus::connected;
    }

    // disconnect from I2C
    virtual void disconnect()
    {
        status = Tstatus::disconnected;
        reset();
    }

    // reconnect I2C
    bool reconnect()
    {
        if (status != Tstatus::disconnected)
            disconnect();
        return connect();
    }
};

// define the static wire initialized
bool ThardwareI2C::FwireInitialized = false;