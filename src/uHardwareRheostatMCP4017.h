#pragma once

#include "uTypedef.h"
#include "Particle.h"
#include "uHardwareRheostat.h"

// hardware constants
#define RHEOSTAT_MCP4017_I2C_ADDRESS 0x2F

// MCP4017 rheostat
class ThardwareRheostatMCP4017 : public ThardwareRheostat
{

public:
    // default i2c address in init
    void init(Resistance _resistance)
    {
        ThardwareRheostat::init(_resistance, RHEOSTAT_MCP4017_I2C_ADDRESS);
    }
};