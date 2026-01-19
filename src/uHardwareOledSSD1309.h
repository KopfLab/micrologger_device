#pragma once

#include "uTypedef.h"
#include "Particle.h"

// SSD1306 OLED
#include "uHardwareI2C.h"
#define SSD1306_NO_SPLASH
#include "Adafruit_SSD1306.h"

class ThardwareOledSSD1309 : public ThardwareI2C, public Adafruit_SSD1306
{

public:
    // enumerations
    using TonOff = sdds::enums::OnOff;
    sdds_enum(___, complete) Tstartup;
    enum class align
    {
        LEFT,
        CENTER,
        RIGHT
    };
    enum class valign
    {
        TOP,
        CENTER,
        BOTTOM
    };

protected:
    Ttimer FstartupTimer;

    // I2C write function
    virtual bool write() override
    {
        if (status != Tstatus::connected && !connect())
            return false;

        // send over I2C
        WITH_LOCK(Wire)
        {
            // make sure vertical offset stays at 0 (tends to get messed up after a while)
            ssd1306_command1(SSD1306_SETSTARTLINE | 0x0);
            // display actual text
            display();
        }
        return true;
    }

    // reset  display
    virtual bool reset() override
    {
        // reset screen
        WITH_LOCK(Wire)
        {
            if (!begin(SSD1306_SWITCHCAPVCC, Fi2cAddress, true, false))
                return false;
        }
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
    sdds_var(Tstartup, startup, sdds::opt::readonly);

    // constructor
    ThardwareOledSSD1309(
        uint8_t _width, const uint8_t _height, int8_t _rstPin,
        uint32_t _clkDuring = 400000, uint32_t _clkAfter = 100000) : Adafruit_SSD1306(_width, _height, &Wire, _rstPin, _clkDuring, _clkAfter)
    {
        // startup timer
        on(FstartupTimer)
        {
            action = ThardwareI2C::Taction::write;
            startup = Tstartup::complete;
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
                // begin with reset = true and periphBegin = false (Wire is already started separately in the I2C base class)
                begin(SSD1306_SWITCHCAPVCC, Fi2cAddress, true, false);
            }

            setTextWrap(0);      // don't wrap text by default
            setTextSize(1);      // smallest text size by default
            setTextColor(WHITE); // default color
            clearDisplay();
            if (splashscreen == TonOff::ON)
            {
                splash();
                FstartupTimer.start(startup_MS);
            }
            WITH_LOCK(Wire)
            {
                display();
            }
            // for the next print
            clearDisplay();
        }
    }

    // print bitmap
    void printBitmap(uint16_t _y, const uint8_t *_bitmap, uint16_t _w, uint16_t _h, uint16_t _xpad = 0, valign _valign = valign::TOP)
    {
        drawBitmap(_y, _bitmap, _w, _h, align::LEFT, _xpad, _valign);
    }

    void printBitmap(uint16_t _y, const uint8_t *_bitmap, uint16_t _w, uint16_t _h, align _align, valign _valign)
    {
        drawBitmap(_y, _bitmap, _w, _h, _align, 0, _valign);
    }

    void drawBitmap(uint16_t _y, const uint8_t *_bitmap, uint16_t _width, uint16_t _height, align _align, uint16_t _xpad = 0, valign _valign = valign::TOP)
    {
        uint16_t x = _xpad;
        if (_align == align::CENTER)
        {
            x = (width() - _width) / 2;
        }
        else if (_align == align::RIGHT)
        {
            x = width() - _width - _xpad;
        }
        uint16_t y = _y;
        if (_valign == valign::CENTER)
        {
            y = (_y < _height / 2) ? 0 : _y - _height / 2;
        }
        else if (_valign == valign::BOTTOM)
        {
            y = (_y < _height) ? 0 : _y - _height;
        }
        Adafruit_SSD1306::drawBitmap(x, y, _bitmap, _width, _height, 1);
    }

    // print text line
    void printLine(uint16_t _y, dtypes::string _line, uint16_t _xpad = 0, valign _valign = valign::TOP)
    {
        printLine(_y, _line, align::LEFT, _xpad, _valign);
    }

    void printLine(uint16_t _y, dtypes::string _line, align _align, valign _valign)
    {
        printLine(_y, _line, _align, 0, _valign);
    }

    void printLine(uint16_t _y, dtypes::string _line, align _align, uint16_t _xpad = 0, valign _valign = valign::TOP)
    {
        uint16_t x = _xpad;
        if (_align == align::CENTER)
        {
            x = (width() - _line.length() * 6) / 2;
        }
        else if (_align == align::RIGHT)
        {
            x = width() - _line.length() * 6 - _xpad;
        }
        uint16_t y = _y;
        if (_valign == valign::CENTER)
        {
            y = (_y < 4) ? 0 : _y - 4;
        }
        else if (_valign == valign::BOTTOM)
        {
            y = (_y < 8) ? 0 : _y - 8;
        }
        setCursor(x, y);
        print(_line);
    }
};