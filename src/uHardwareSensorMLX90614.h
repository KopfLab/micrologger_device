#pragma once

#include "uTypedef.h"
#include "Particle.h"

// TODO: needs offset and potentially slope calibration, not sure this is really the best sensor for what we're trying to do here

// IO expander: TCA9534 8-channel multiplexer with 1 wire (I2C) commms
#include "uHardwareI2C.h"
class ThardwareSensorMLX90614 : public ThardwareI2C
{

private:
    // calculate crc checksum
    static uint8_t calculate_crc8_pec(const uint8_t *data, size_t len)
    {
        uint8_t crc = 0x00;
        for (size_t i = 0; i < len; ++i)
        {
            crc ^= data[i];
            for (uint8_t b = 0; b < 8; ++b)
            {
                crc = (crc & 0x80) ? uint8_t((crc << 1) ^ 0x07) : uint8_t(crc << 1);
            }
        }
        return crc;
    }

    // read data
    // * @return error code from Wire.endTransmission() or custom error code 0xff if not the correct number of values read back
    uint8_t readData(uint8_t command, uint16_t &rawOut, bool checkPEC = true)
    {
        // start transmission
        Wire.beginTransmission(Fi2cAddress);
        Wire.write(command);
        // false = keep connection alive instead of stop
        uint8_t transmitCode = Wire.endTransmission(false);

        // if it worked, request the uint8_t data
        if (transmitCode != SYSTEM_ERROR_NONE)
            return transmitCode;

        // Read 3 bytes: LSB, MSB, PEC
        const uint8_t n = 3;
        uint8_t got = Wire.requestFrom(Fi2cAddress, n);

        if (got != n)
            return 0xff;

        // read the 3 values;
        uint8_t lsb = Wire.read();
        uint8_t msb = Wire.read();
        uint8_t pec = Wire.read();

        Log.trace("lsb: %x, msb: %x, pec: %x", lsb, msb, pec);

        if (checkPEC)
        {
            // PEC over: [SA+W, command, SA+R, LSB, MSB]
            uint8_t sa_w = (Fi2cAddress << 1) | 0;
            uint8_t sa_r = (Fi2cAddress << 1) | 1;
            uint8_t buf[5] = {sa_w, command, sa_r, lsb, msb};
            uint8_t calc = calculate_crc8_pec(buf, sizeof(buf));
            if (calc != pec)
                return 0xff;
        }

        rawOut = (uint16_t(msb) << 8) | lsb;
        return SYSTEM_ERROR_NONE;
    }

    static bool rawToC(uint16_t raw, float &tempCOut)
    {
        if (raw & 0x8000)
        { // error flag :contentReference[oaicite:5]{index=5}
            return false;
        }
        float kelvin = float(raw) * 0.02f; // :contentReference[oaicite:6]{index=6}
        tempCOut = kelvin - 273.15f;
        return true;
    }

    bool readAmbientC(float &tempC)
    {
        const uint8_t TA_REG = 0x06; // :contentReference[oaicite:7]{index=7}
        uint16_t raw;
        if (readData(TA_REG, raw) != SYSTEM_ERROR_NONE)
            return false;
        return rawToC(raw, tempC);
    }

    bool readObjectC(float &tempC)
    {
        const uint8_t TOBJ1_REG = 0x07; // :contentReference[oaicite:8]{index=8}
        uint16_t raw;
        if (readData(TOBJ1_REG, raw) != SYSTEM_ERROR_NONE)
            return false;
        return rawToC(raw, tempC);
    }

    // I2C read function
    virtual bool read() override
    {
        if (status != enums::TconStatus::connected && !connect())
            return false;

        float buffer;
        if (!readAmbientC(buffer))
            return false;

        ambient_C = buffer;

        if (!readObjectC(buffer))
            return false;

        object_C = buffer;

        return true;
    }

    // I2C write function
    virtual bool write() override
    {
        if (status != enums::TconStatus::connected && !connect())
            return false;
        return true;
    }

public:
    // sdds variables
    sdds_var(Tfloat32, ambient_C, sdds::opt::readonly);
    sdds_var(Tfloat32, object_C, sdds::opt::readonly);

    // constructor
    ThardwareSensorMLX90614()
    {
    }

    // default i2c address in init
    void init(uint8_t _i2cAddress = 0x5a)
    {
        ThardwareI2C::init(_i2cAddress);
    }
};
