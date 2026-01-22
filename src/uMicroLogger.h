#pragma once

#include "uTypedef.h"
#include "uCoreEnums.h"
#include "uHardware.h"
#include "uStirrer.h"
#include "uOpticalDensity.h"
#include "uLights.h"
#include "symbols.h"

/**
 * @brief
 */
class TmicroLogger : public TmenuHandle
{

private:
    // timers
    Ttimer FvortexTimer;
    Ttimer FdisplayTimer;

    // display update function
    void refreshDisplay()
    {
        hardware().display.clearDisplay();
        hardware().display.setCursor(0, 0);

        // wifi?
        if (particleSystem().internet == TparticleSystem::TinternetStatus::connected)
            hardware().display.drawIcon(wifi_icon, wifi_icon_width);
        else
            hardware().display.drawIcon(no_wifi_icon, no_wifi_icon_width);

        // publishing? (don't show the "not publishing", it's not as informative)
        if (particleSystem().publishing.publish == TonOff::ON)
            hardware().display.drawIcon(publishing_icon, publishing_icon_width);

        // device name
        if (particleSystem().name == "")
            hardware().display.printLine(Tdisplay::headerY, "connecting...");
        else
            hardware().display.printLine(Tdisplay::headerY, particleSystem().name);

        // device connected? --> print message if not
        if (device == enums::TconStatus::disconnected)
        {
            hardware().display.drawIcon(alert_icon, alert_icon_width);
            hardware().display.printLine(Tdisplay::line3Y, "connect to reader", Tdisplay::align::CENTER);
            return;
        }

        // alert / FIXME when does this appear?
        hardware().display.drawIcon(alert_icon, alert_icon_width);

        // stirrer speed
        if (stirrer.status == Tstirrer::Tstatus::off)
            hardware().display.printLine(Tdisplay::line1Y, "RPM:off");
        else
            hardware().display.printLine(Tdisplay::line1Y, "RPM:" + stirrer.speed_rpm.to_string());

        // stirrer event
        if (stirrer.event != Tstirrer::Tevent::none)
            hardware().display.printLine(Tdisplay::line1Y, stirrer.event.to_string(), Tdisplay::offsetX);
        else
            hardware().display.printLine(Tdisplay::line1Y, "SP:" + stirrer.setpoint_rpm.to_string() + "rpm", Tdisplay::offsetX);

        hardware().display.printLine(Tdisplay::line2Y, "OD:0.234", Tdisplay::offsetX);
        hardware().display.printLine(Tdisplay::line3Y, "Temp:35.1C always on");
        // hardware().display.printLine(Tdisplay::line3Y, hardware().display.error.to_string(), Tdisplay::align::RIGHT);
        hardware().display.printLine(Tdisplay::line4Y, Time.format(TIME_FORMAT_ISO8601_FULL), Tdisplay::align::CENTER);

        // light status
        if (lights.status == Tlights::Tstatus::on)
            hardware().display.printLine(Tdisplay::line5Y, "Light:" + lights.intensity.to_string() + "%");
        else if (lights.status == Tlights::Tstatus::off)
            hardware().display.printLine(Tdisplay::line5Y, "Light:off");
        else if (lights.status == Tlights::Tstatus::error)
            hardware().display.printLine(Tdisplay::line5Y, "Light:ERR");

        // light state
        if (lights.event != Tlights::Tevent::none)
            hardware().display.printLine(Tdisplay::line5Y, lights.event.to_string(), Tdisplay::offsetX);
        else if (lights.state == Tlights::Tstate::on)
            hardware().display.printLine(Tdisplay::line5Y, "always on", Tdisplay::offsetX);
        else if (lights.state == Tlights::Tstate::off)
            hardware().display.printLine(Tdisplay::line5Y, "always off", Tdisplay::offsetX);
        else if (lights.state == Tlights::Tstate::schedule)
            hardware().display.printLine(Tdisplay::line5Y, lights.scheduleInfo.c_str(), Tdisplay::offsetX);

        // fan status
        if (lights.fan.status == Tlights::Tstatus::on)
            hardware().display.printLine(Tdisplay::line6Y, "Fan:on");
        else if (lights.fan.status == Tlights::Tstatus::off)
            hardware().display.printLine(Tdisplay::line6Y, "Fan:off");
        else if (lights.fan.status == Tlights::Tstatus::error)
            hardware().display.printLine(Tdisplay::line6Y, "Fan:ERR");

        // fan state
        if (lights.fan.state == Tlights::Tfan::Tstate::on)
            hardware().display.printLine(Tdisplay::line6Y, "always on", Tdisplay::offsetX);
        else if (lights.fan.state == Tlights::Tfan::Tstate::off)
            hardware().display.printLine(Tdisplay::line6Y, "always off", Tdisplay::offsetX);
        else if (lights.fan.state == Tlights::Tfan::Tstate::withLight)
            hardware().display.printLine(Tdisplay::line6Y, "with light", Tdisplay::offsetX);

        hardware().display.drawLayoutLines();
    }

public:
    // enumerations
    using TonOff = sdds::enums::OnOff;

    // sdds variables
    sdds_var(enums::TconStatus, device, sdds::opt::readonly);
    sdds_var(Tstirrer, stirrer);
    // sdds_var(TopticalDensity, OD);
    sdds_var(Tlights, lights);

    TmicroLogger()
    {

        // make sure hardware is initalized
        hardware();

/**
 * DISPLAY
 */
#pragma region Display

        // display startup complete
        on(hardware().display.startup)
        {
            if (hardware().display.startup == Tdisplay::Tstartup::complete)
            {
                FdisplayTimer.start(0);
            }
        };

        // regular display refresh
        on(FdisplayTimer)
        {
            refreshDisplay();
            hardware().display.action = Tdisplay::Taction::write;
            FdisplayTimer.start(hardware().display.refresh_ms);
        };

/**
 * CONNECTION
 */
#pragma region Device

        // keep connection status updated
        on(hardware().i2cValue)
        {
            if (hardware().i2cValue == Thardware::TioValue::ON)
            {
                if (device != enums::TconStatus::connected)
                    device = enums::TconStatus::connected;
                lights.resumeState();
            }
            else if (hardware().i2cValue != Thardware::TioValue::ON)
            {
                if (device != enums::TconStatus::disconnected)
                    device = enums::TconStatus::disconnected;
            }
        };
    }
};
