#pragma once

#include "uTypedef.h"
#include "uCoreEnums.h"
#include "uHardware.h"
#include "uComponentStirrer.h"
#include "uComponentOpticalDensity.h"
#include "uComponentLights.h"
#include "symbols.h"

/**
 * @brief
 */
class TmicroLogger : public TmenuHandle
{

public:
    // enumeriations
    using TfanState = TcomponentLights::Tfan::Tstate;
    using TlightsState = TcomponentLights::Tstate;
    using TlightsStatus = TcomponentLights::Tstatus;
    using TlightsEvent = TcomponentLights::Tevent;
    using TstirrerEvent = TcomponentStirrer::Tevent;
    using TstirrerStatus = TcomponentStirrer::Tstatus;
    using TodError = TcomponentOpticalDensity::Terror;

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
            hardware().display.printLine(ThardwareDisplay::headerY, "connecting...");
        else
            hardware().display.printLine(ThardwareDisplay::headerY, particleSystem().name);

        // device connected? --> print message if not
        if (device == enums::TconStatus::disconnected)
        {
            hardware().display.drawIcon(alert_icon, alert_icon_width);
            hardware().display.printLine(ThardwareDisplay::line3Y, "connect to reader", ThardwareDisplay::align::CENTER);
            return;
        }

        // alert / FIXME when does this appear?
        hardware().display.drawIcon(alert_icon, alert_icon_width);

        // optical density
        hardware().display.printLine(ThardwareDisplay::line1Y, "SAT:" + OD.signal_ppt.to_string());

        if (OD.error == TodError::none)
            hardware().display.printLine(ThardwareDisplay::line1Y, "SDEV:" + hardware().signal.sdev.to_string(), ThardwareDisplay::offsetX);
        else
            hardware().display.printLine(ThardwareDisplay::line1Y, OD.error.to_string(), ThardwareDisplay::offsetX);
        // hardware().display.printLine(Tdisplay::line1Y, "OD:0.234", Tdisplay::offsetX);

        // stirrer speed
        if (stirrer.status == TstirrerStatus::off)
            hardware().display.printLine(ThardwareDisplay::line2Y, "RPM:off");
        else
            hardware().display.printLine(ThardwareDisplay::line2Y, "RPM:" + stirrer.speed_rpm.to_string());

        // stirrer event
        if (stirrer.event != TstirrerEvent::none)
            hardware().display.printLine(ThardwareDisplay::line2Y, stirrer.event.to_string(), ThardwareDisplay::offsetX);
        else
            hardware().display.printLine(ThardwareDisplay::line2Y, "SP:" + stirrer.setpoint_rpm.to_string() + "rpm", ThardwareDisplay::offsetX);

        hardware().display.printLine(ThardwareDisplay::line3Y, "Temp:35.1C always on");
        // hardware().display.printLine(Tdisplay::line3Y, hardware().display.error.to_string(), Tdisplay::align::RIGHT);
        hardware().display.printLine(ThardwareDisplay::line4Y, Time.format(TIME_FORMAT_ISO8601_FULL), ThardwareDisplay::align::CENTER);

        // light status
        if (lights.status == TlightsStatus::on)
            hardware().display.printLine(ThardwareDisplay::line5Y, "Light:" + lights.intensity.to_string() + "%");
        else if (lights.status == TlightsStatus::off)
            hardware().display.printLine(ThardwareDisplay::line5Y, "Light:off");
        else if (lights.status == TlightsStatus::error)
            hardware().display.printLine(ThardwareDisplay::line5Y, "Light:ERR");

        // light state
        if (lights.event != TlightsEvent::none)
            hardware().display.printLine(ThardwareDisplay::line5Y, lights.event.to_string(), ThardwareDisplay::offsetX);
        else if (lights.state == TlightsState::on)
            hardware().display.printLine(ThardwareDisplay::line5Y, "always on", ThardwareDisplay::offsetX);
        else if (lights.state == TlightsState::off)
            hardware().display.printLine(ThardwareDisplay::line5Y, "always off", ThardwareDisplay::offsetX);
        else if (lights.state == TlightsState::schedule)
            hardware().display.printLine(ThardwareDisplay::line5Y, lights.scheduleInfo.c_str(), ThardwareDisplay::offsetX);

        // fan status
        if (lights.fan.status == TlightsStatus::on)
            hardware().display.printLine(ThardwareDisplay::line6Y, "Fan:on");
        else if (lights.fan.status == TlightsStatus::off)
            hardware().display.printLine(ThardwareDisplay::line6Y, "Fan:off");
        else if (lights.fan.status == TlightsStatus::error)
            hardware().display.printLine(ThardwareDisplay::line6Y, "Fan:ERR");

        // fan state
        if (lights.fan.state == TfanState::on)
            hardware().display.printLine(ThardwareDisplay::line6Y, "always on", ThardwareDisplay::offsetX);
        else if (lights.fan.state == TfanState::off)
            hardware().display.printLine(ThardwareDisplay::line6Y, "always off", ThardwareDisplay::offsetX);
        else if (lights.fan.state == TfanState::withLight)
            hardware().display.printLine(ThardwareDisplay::line6Y, "with light", ThardwareDisplay::offsetX);

        hardware().display.drawLayoutLines();
    }

public:
    // enumerations
    using TonOff = sdds::enums::OnOff;

    // sdds variables
    sdds_var(enums::TconStatus, device, sdds::opt::readonly);
    sdds_var(TcomponentOpticalDensity, OD);
    sdds_var(TcomponentStirrer, stirrer);
    sdds_var(TcomponentLights, lights);

    TmicroLogger()
    {

        // make sure hardware is initalized
        hardware();

        // resume components' state on startup complete
        on(particleSystem().startup)
        {
            if (particleSystem().startup == TparticleSystem::TstartupStatus::complete)
            {
                OD.resumeState();
                stirrer.resumeState();
                lights.resumeState();
            }
        };

        // display startup complete
        on(hardware().display.startup)
        {
            if (hardware().display.startup == ThardwareDisplay::Tstartup::complete)
            {
                FdisplayTimer.start(0);
            }
        };

        // regular display refresh
        on(FdisplayTimer)
        {
            refreshDisplay();
            hardware().display.action = ThardwareDisplay::Taction::write;
            FdisplayTimer.start(hardware().display.refresh_ms);
        };

        // keep connection status updated and resume state whenever we reconnect
        on(hardware().i2cValue)
        {
            if (hardware().i2cValue == Thardware::TioValue::ON)
            {
                if (device != enums::TconStatus::connected)
                    device = enums::TconStatus::connected;
                if (particleSystem().startup == TparticleSystem::TstartupStatus::complete)
                {
                    OD.resumeState();
                    stirrer.resumeState();
                    lights.resumeState();
                }
            }
            else if (hardware().i2cValue != Thardware::TioValue::ON)
            {
                if (device != enums::TconStatus::disconnected)
                    device = enums::TconStatus::disconnected;
            }
        };
    }
};
