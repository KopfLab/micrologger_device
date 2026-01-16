#pragma once

#include "uTypedef.h"
#include "Particle.h"

// SSD1306 OLED
#include "uHardwareI2C.h"
#define SSD1306_NO_SPLASH
#include "Adafruit_SSD1306.h"
#include "splash.h"

class ThardwareOledSSD1309 : public ThardwareI2C
{

public:
    // enumerations
    using TonOff = sdds::enums::OnOff;

protected:
    // // display object
    Adafruit_SSD1306 display;
    dtypes::uint16 Fwidth;
    dtypes::uint16 Fheight;

private:
    Ttimer FstartupTimer;

    // I2C write function
    virtual bool write() override
    {
        if (status != Tstatus::connected && !connect())
            return false;

        // clear
        display.clearDisplay();

        // fill
        if (splashscreen == TonOff::ON)
            splash();
        else
            screen();

        // send over I2C
        WITH_LOCK(Wire)
        {
            display.display();
        }
        return true;
    }

    // read from I2C device (not applicable)
    virtual bool read() override
    {
        return true;
    }

    // render screen content
    virtual void screen()
    {
    }

    // render splash screen content
    virtual void splash()
    {
    }

public:
    // splashscreen
    sdds_var(TonOff, splashscreen, sdds::opt::nothing, TonOff::ON);
    sdds_var(Tuint32, startup_MS, sdds::opt::nothing, 6000);

    // constructor
    ThardwareOledSSD1309(const uint16_t _width, const uint16_t _height) : Fwidth(_width), Fheight(_height), display(_width, _height)
    {

        // startup timer
        on(FstartupTimer)
        {
            if (splashscreen == TonOff::ON)
                splashscreen = TonOff::OFF;
        };

        // splashscreen switch
        on(splashscreen)
        {
            action = ThardwareI2C::Taction::write;
        };
    }

    // default i2c address in init
    void init(byte _i2cAddress = 0x3C)
    {
        if (!Finitialized)
        {
            WITH_LOCK(Wire)
            {
                ThardwareI2C::init(_i2cAddress);
                display.begin(SSD1306_SWITCHCAPVCC, Fi2cAddress);
            }

            display.setTextWrap(0);      // don't wrap text by default
            display.setTextSize(1);      // smallest text size by default
            display.setTextColor(WHITE); // default color
            display.clearDisplay();
            if (splashscreen == TonOff::ON)
            {
                splash();
                FstartupTimer.start(startup_MS);
            }
            WITH_LOCK(Wire)
            {
                display.display();
            }
        }
    }
};