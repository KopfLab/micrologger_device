#pragma once

#include "uTypedef.h"
#include "Particle.h"
#include "uHardwareOledSSD1309.h"
#include "splash.h"

// hardware constants
#define DISPLAY_RESET_PIN D10
#define DISPLAY_I2C_ADDRESS 0x3C

class Tdisplay : public ThardwareOledSSD1309
{

public:
    // enumerations
    using align = ThardwareOledSSD1309::align;
    using valign = ThardwareOledSSD1309::valign;

    // display positions
    static const uint16_t dividerX = 63;
    static const uint16_t dividerY = 14;
    static const uint16_t offsetX = dividerX + 3; // where the second column starts
    static const uint16_t iconsY = 0;
    static const uint16_t headerY = 3;
    static const uint16_t line1Y = 17;
    static const uint16_t line2Y = line1Y + 8;
    static const uint16_t line3Y = line2Y + 8;
    static const uint16_t line4Y = line3Y + 8;
    static const uint16_t line5Y = line4Y + 8;
    static const uint16_t line6Y = line5Y + 8;

    // overall screen structure features
    void drawLayoutLines()
    {
        drawLine(0, dividerY, width(), dividerY, 1);
        drawLine(dividerX, dividerY, dividerX, height(), 1);
    }

    // indicators icons (always in the upper right corner)
    // default height is 13 pixels
    void drawIcon(const uint8_t *_icon, uint16_t _w, uint16_t _h = 13)
    {
        drawBitmap(Tdisplay::iconsY, _icon, _w, _h, Tdisplay::align::RIGHT, FiconXpadding);
        FiconXpadding += _w + 2;
    }

    // clear display
    void clearDisplay()
    {
        Adafruit_SSD1306::clearDisplay();
        FiconXpadding = 0;
    }

private:
    // icon x padding
    uint8_t FiconXpadding = 0;
    dtypes::uint16 Fversion = 0;

    // render splash screen content
    virtual void splash() override
    {
        printBitmap(height() / 2, splash1_data, splash1_width, splash1_height, align::CENTER, valign::CENTER);
        printLine(line5Y, "version " + String(Fversion), align::CENTER);
    }

public:
    // display refresh rate
    sdds_var(Tuint32, refresh_ms, sdds::opt::nothing, 1000);

    // initialize 128x64 oled display with width, height, reset pin, and lower clock speed
    // (screen does not get messed up as often at 100kHz instead of the 400kHz default)
    Tdisplay() : ThardwareOledSSD1309(128, 64, DISPLAY_RESET_PIN, 100000)
    {
    }

    // default i2c address in init
    void init(dtypes::uint16 _version, uint8_t _i2cAddress = DISPLAY_I2C_ADDRESS)
    {
        Fversion = _version;
        ThardwareOledSSD1309::init(_i2cAddress);
    }
};