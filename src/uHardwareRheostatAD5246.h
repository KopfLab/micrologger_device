#pragma once

#include "uTypedef.h"
#include "Particle.h"
#include "uHardwareRheostat.h"

// hardware constants
#define RHEOSTAT_AD5246_I2C_ADDRESS 0x2E
#define RHEOSTAT_AD5246_MAX_STEPS 127

// AD5246 rheostat
class ThardwareRheostatAD5246 : public ThardwareRheostat
{

public:
    // default i2c address in init
    void init(Resistance _resistance)
    {
        ThardwareRheostat::init(RHEOSTAT_AD5246_I2C_ADDRESS, RHEOSTAT_AD5246_MAX_STEPS, _resistance);
        action = ThardwareI2C::Taction::write;
    }
};