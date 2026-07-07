#pragma once

#include "uTypedef.h"
#include "Particle.h"
#include "uHardwareRheostat.h"

// hardware constants
#define RHEOSTAT_MCP4017_I2C_ADDRESS 0x2F
#define RHEOSTAT_MCP4017_MAX_STEPS 127

// MCP4017 rheostat
class ThardwareRheostatMCP4017 : public ThardwareRheostat
{

public:
    // default i2c address in init
    void init(Resistance _resistance)
    {
        ThardwareRheostat::init(RHEOSTAT_MCP4017_I2C_ADDRESS, RHEOSTAT_MCP4017_MAX_STEPS, _resistance);
        action = ThardwareI2C::Taction::write;
    }
};