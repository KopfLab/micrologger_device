#pragma once

#include "uTypedef.h"
#include "Particle.h"

// IO expander: TCA9534 8-channel multiplexer with 1 wire (I2C) commms
#include "uHardwareI2C.h"
class ThardwareRheostatMCP4652 : public ThardwareI2C
{

public:
    // enumerations

    // Enumeration for allowed resistor values (in kOhm)
    enum class Resistance
    {
        R5k = 5,
        R10k = 10,
        R50k = 50,
        R100k = 100
    };

    // Enumeration for possible number of steps
    enum class Resolution
    {
        S128 = 128,
        S256 = 256
    };

private:
    // wiper resistance
    const dtypes::uint16 FwiperResistance = 75; // see spec sheet

    // maximal resistance (in Ohm)
    dtypes::uint32 FmaxResistance = 0;

    // number of steps
    dtypes::uint16 FmaxSteps = 1;

    // low level interactions with the chip via I2C

    /**
     * @brief read registers
     * @param _first register to start at
     * @param _n number of register values to read
     * @param _values pointer to array of size _n to read into
     * @return error code from Wire.endTransmission() or custom error code 0xff if not the correct number of values read back
     */
    uint8_t readRegisters()
    {

        // Command Byte:
        // AD = 0x00 (Wiper0)
        // C1:C0 = 0b11 (READ)
        uint8_t cmd = (0x00 << 4) | (0b11 << 2);

        // lock for thread safety
        WITH_LOCK(Wire)
        {
            Wire.beginTransmission(Fi2cAddress);
            Wire.write(cmd);
            uint8_t transmitCode = Wire.endTransmission(false);
            if (transmitCode != SYSTEM_ERROR_NONE)
            {
                Log.trace("could not read Rheostat register values");
                return transmitCode;
            }

            // request to read both wiper registers (2 bytes per wiper)
            const uint8_t bytesRequested = 4;
            uint8_t request = Wire.requestFrom(Fi2cAddress, bytesRequested);
            if (request != bytesRequested || !Wire.available())
            {
                Log.trace("could not read the expected number of Rheostat register values");
                return 0xff;
            }

            // Read bytes
            uint8_t b0 = Wire.read(); // Wiper0 MSB
            uint8_t b1 = Wire.read(); // Wiper0 LSB
            uint8_t b2 = Wire.read(); // Wiper1 MSB
            uint8_t b3 = Wire.read(); // Wiper1 LSB

            // Decode 9-bit values (D8:D0)
            uint16_t w0 = ((uint16_t)(b0 & 0x01) << 8) | b1;
            uint16_t w1 = ((uint16_t)(b2 & 0x01) << 8) | b3;

            Log.trace("wiper1: %d (%s %s), wiper2: %d (%s %s)", w0, byteBits(b0).c_str(), byteBits(b1).c_str(), w1, byteBits(b2).c_str(), byteBits(b3).c_str());
        }
        return SYSTEM_ERROR_NONE;
    }

    bool mcp4652SetWiper(uint8_t wiper, uint16_t value)
    {

        Log.trace("BEFORE:");
        readRegisters();

        if (wiper > 1)
            return false; // only 2 wipers
        if (value > 256)
            value = 256; // clamp to max

        // Internal memory address: 0x00 = wiper0, 0x01 = wiper1
        uint8_t ad = (wiper == 0) ? 0x00 : 0x01;

        // Split 9-bit value into D8 (in command byte) and D7..D0 (data byte)
        uint8_t d8 = (value >> 8) & 0x01;       // 0 or 1
        uint8_t data = (uint8_t)(value & 0xFF); // lower 8 bits

        // Command byte layout (MSB..LSB): [C1 C0 AD3 AD2 AD1 AD0 D9 D8]
        // For normal write: C1:C0 = 00, D9 = 0
        uint8_t C1C0 = 0b00; // Write Data command
        uint8_t dField = d8; // D9=0, D8=d8  -> 0b0d8
        uint8_t cmd = (C1C0 << 6) | ((ad & 0x0F) << 2) | dField;

        Wire.beginTransmission(Fi2cAddress); // 7-bit I2C address
        Wire.write(cmd);                     // command byte
        Wire.write(data);                    // data byte (D7..D0)
        uint8_t err = Wire.endTransmission();

        Log.trace("AFTER:");
        readRegisters();

        return (err == 0); // true on success
    }

    // calculate current resistance (in Ohm)
    dtypes::uint32 calculateResistance()
    {
        return (FmaxResistance * wiper1.Fvalue + FmaxResistance * wiper2.Fvalue) / FmaxSteps + FwiperResistance;
    }

public:
    // sdds vars
    sdds_var(Tuint16, steps, 0, 0);
    sdds_var(Tuint16, wiper1, 0, 0);
    sdds_var(Tuint16, wiper2, 0, 0);
    sdds_var(Tuint32, resistance_Ohm, sdds::opt::readonly);

    // constructor
    ThardwareRheostatMCP4652()
    {

        on(steps)
        {
            if (steps > 2 * FmaxSteps)
            {
                steps = 2 * FmaxSteps;
            }
            else
            {
                if (steps > FmaxResistance)
                {
                    wiper1 = FmaxSteps;
                    wiper2 = steps - FmaxResistance;
                }
                else
                {
                    wiper1 = steps;
                    wiper2 = 0;
                }
            }
        };

        on(wiper1)
        {
            if (wiper1 > FmaxSteps)
            {
                wiper1 = FmaxSteps;
            }
            else
            {
                mcp4652SetWiper(0, wiper1.Fvalue);
                resistance_Ohm = calculateResistance();
            }
        };

        on(wiper2)
        {
            if (wiper2 > FmaxSteps)
            {
                wiper2 = FmaxSteps;
            }
            else
            {
                mcp4652SetWiper(1, wiper2.Fvalue);
                resistance_Ohm = calculateResistance();
            }
        };
    }

    // default i2c address in init
    void init(Resolution _resolution, Resistance _resistance, byte _i2cAddress = 0x2C)
    {
        FmaxSteps = static_cast<dtypes::uint16>(_resolution);
        FmaxResistance = static_cast<dtypes::uint32>(_resistance) * 1000;
        ThardwareI2C::init(_i2cAddress);
    }
};

// // Set one wiper of an MCP4652 dual rheostat.
// //  devAddr : 7-bit I2C address of the chip (NOT shifted).
// //  wiper   : 0 for P0, 1 for P1.
// //  value   : 0..256 (257 steps). Typically you just use 0..255.
// bool mcp4652SetWiper(uint8_t devAddr, uint8_t wiper, uint16_t value)
// {
//     if (wiper > 1) return false;       // only 2 wipers
//     if (value > 256) value = 256;      // clamp to max

//     // Internal memory address: 0x00 = wiper0, 0x01 = wiper1
//     uint8_t ad = (wiper == 0) ? 0x00 : 0x01;

//     // Split 9-bit value into D8 (in command byte) and D7..D0 (data byte)
//     uint8_t d8   = (value >> 8) & 0x01;      // 0 or 1
//     uint8_t data = (uint8_t)(value & 0xFF);  // lower 8 bits

//     // Command byte layout (MSB..LSB): [C1 C0 AD3 AD2 AD1 AD0 D9 D8]
//     // For normal write: C1:C0 = 00, D9 = 0
//     uint8_t C1C0   = 0b00;                  // Write Data command
//     uint8_t dField = d8;                    // D9=0, D8=d8  -> 0b0d8
//     uint8_t cmd    = (C1C0 << 6) | ((ad & 0x0F) << 2) | dField;

//     Wire.beginTransmission(devAddr);  // 7-bit I2C address
//     Wire.write(cmd);                  // command byte
//     Wire.write(data);                 // data byte (D7..D0)
//     uint8_t err = Wire.endTransmission();

//     return (err == 0);                // true on success
// }

// void setup() {
//     Wire.begin();

//     uint8_t mcpAddr = 0x2C;  // whatever your A2..A0 wiring gives

//     // Set wiper 0 to midscale:
//     mcp4652SetWiper(mcpAddr, 0, 128);

//     // Set wiper 1 near full-scale:
//     mcp4652SetWiper(mcpAddr, 1, 240);
// }