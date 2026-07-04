[![microloggger](https://github.com/KopfLab/micrologger_device/actions/workflows/compile-micrologger.yaml/badge.svg?branch=main)](https://github.com/KopfLab/micrologger_device/actions/workflows/compile-micrologger.yaml)

# *µLogger*

This repository holds the code, schematics, and documentation to build a ***M**odular **I**nternet **C**ontrollable **R**eal-time **O**ptical-density Logger* (***microLogger*** or ***µLogger*** for short). All content of this repository is free to use for noncommercial research and development efforts as outlined in the terms of the included [LICENSE file](LICENSE.md).

To cite ***µLogger*** in publications use:

> Kopf S and Younkin A, 2025. _µLogger: a Modular Internet Controllable Real-time Optical-density Logger_. <https://github.com/KopfLab/micrologger_device>

A BibTeX entry for LaTeX users is

```
@Manual{
  microLogger,
  title = {µLogger: a Modular Internet Controllable Real-time Optical-density Logger},
  author = {Sebastian Kopf and Adam Younkin},
  year = {2025},
  url = {https://github.com/KopfLab/micrologger_device}
}
```

## About

Modular Internet Controllable Real-time Optical-density Loggers (µLoggers) are optical density measurement devices for continuously recording the growth of a microbial culture directly in its culturing vessel. Each microLogger consists of a compact high strength carbon fibre culture vessel holder that has a built-in light source at a narrow band wavelength and a light sensor with automatically adjustable gain that measures the light transmission across the culture vessel to automatically and continuously quantify optical density at user defined time intervals. A magnetically coupled stirrer built into the vessel holder can agitate the culture both continuously as well as in vortexing mode  (= pulse agitation) at user-set stir/vortexing speeds from 50 to 5000 rpm. Culture vessel adapters that are magnetically secured to the vessel holder allow for quick and easy swapping for differently sized culture vessels and snugly hold the culture vessel in place using a radial spring mechanism. Adapters can be custom sized for the preferred culture vessel from standard 18mm diameter 15mL culture tubes up to 58mm GL45 100mL media bottles. Typical adapter sizes also cover culture vessels typically used in anaerobic microbial culture including balch tubes and serum bottles of varying sizes. Additional optional adapters that are magnetically secured on top of the vessel adapter can provide additional functionality such as user-controlled illumination for experiments with phototrophic microorganisms. Each µLogger is connected to a compact controller (the micrologger "brain") that connects to the culture vessel holder via a single cable such that the vessel holder can be placed inside an incubator (e.g. for temperature or gas modulation) while the controller sits outside the incubator and provides full control and power to the culture vessel holder for stirring, optical density measurements, and (optionally) lighting. The controller has built-in WiFi connectivity and receives/sends data wirelessly wirelessly. This makes it possible to monitor optical density of the microbial cultures in real-time from any location. The controller also has a built-in screen that displays the state of the micrologger (e.g. stir speed, read interval, internet connectivity) as well as the real-time data from the culture vessel so that users can check on their experiments with a glance at the controller without opening the incubator and without handling the culture vessel in any way to avoid potentially disturbing an experiment. For convenience, the controller can also be attached magnetically to the outside of the incubator (if metal) or any other metallic surface.

## Features

- **Continuous optical-density logging** — a narrow-band beam LED and an OPT101 photodetector with automatically optimized amplifier gain measure the light transmission across the culture vessel; the firmware computes optical density (−log₁₀ of transmittance) at user-defined intervals, propagates a standard-deviation error onto every reading, and stores the per-vessel zero reference on the device so it survives power outages.
- **Magnetic stirring & vortexing** — a built-in brushless motor drives a magnetically coupled stirrer with configurable speed, acceleration/deceleration ramps, and a vortex (pulse-agitation) mode; stirring can automatically pause or vortex around each optical-density read.
- **Swappable, magnetically secured vessel adapters** — 3D-printed adapters hold vessels from 18 mm / 15 mL culture tubes up to GL45 100 mL bottles (including Balch/Hungate tubes and serum bottles) with a radial spring mechanism, and accept stacking add-ons such as the illumination adapter.
- **Optional illumination for phototrophs** — a dimmable light adapter with permanent on/off or scheduled (repeating daily-cycle) operation and a light-synced cooling fan.
- **Temperature & power monitoring** — on-board TMP117 temperature sensing at the vessel holder plus supply-voltage monitoring with under-voltage warnings on the display.
- **At-a-glance local display** — the WiFi controller sits outside the incubator, links to the reader over a single cable, and shows live optical density, stir speed, temperature, connectivity, and publishing status on an OLED; it mounts magnetically to any metal surface.
- **Real-time cloud connectivity** — built on the Particle [Photon 2](https://docs.particle.io/reference/datasheets/wi-fi/photon-2-datasheet/); publishes JSON data logs via cloud webhooks (the `microloggerData` event) for recording into spreadsheets or databases, averages data over configurable intervals, and caches logs during offline periods to bridge device downtime.
- **Self-describing & remotely controllable** — the entire device state is exposed as a self-describing data structure (SDDS) that the [sddsParticle](https://github.com/KopfLab/sddsParticle) web GUI (or the Particle CLI/API) can browse and control, with persisted settings, one-shot actions, and per-variable publish intervals.
- **Open & modular** — component-based, easily extensible C++ firmware (separate `sensor`, `stirrer`, `lights`, and `environment` components), open [KiCad](https://www.kicad.org/) hardware, and open [OpenSCAD](https://openscad.org/) 3D-printable mechanical parts.

## Hardware

A µLogger is built from four custom circuit boards, designed in [KiCad](https://www.kicad.org/) with sources under [`pcbs/`](pcbs). For each board the board layout is shown below (click it to open the full schematic / wiring diagram as a PDF).

### Controller board — the µLogger "brain"

A Particle Photon 2 for WiFi and control, the OLED status display, the 24 V power input with supply-voltage monitoring, the controller-version code pins, and the single-cable connector to the reader (the vessel holder).

<a href="pcbs/controller_board/controller_board.pdf"><img src="pcbs/controller_board/controller_board.svg" alt="controller board layout" width="480"></a>

*→ [Controller board wiring diagram (PDF)](pcbs/controller_board/controller_board.pdf)*

### Sensor board

Mounted in the vessel holder: the OPT101 photodiode/transimpedance amplifier with a two-stage digital-potentiometer gain circuit (MCP4017 + AD5246), the TCA9534 I2C IO expander (reader-connection detection plus beam and indicator LEDs), and the measurement-beam LED. The board version is read back over the IO expander.

<a href="pcbs/sensor_board/sensor_board.pdf"><img src="pcbs/sensor_board/sensor_board.svg" alt="sensor board layout" width="480"></a>

*→ [Sensor board wiring diagram (PDF)](pcbs/sensor_board/sensor_board.pdf)*

### LED board

The optional illumination adapter: the light LEDs driven by a PCA9633 PWM controller for dimming, together with the cooling fan.

<a href="pcbs/led_board/led_board.pdf"><img src="pcbs/led_board/led_board.svg" alt="LED board layout" width="480"></a>

*→ [LED board wiring diagram (PDF)](pcbs/led_board/led_board.pdf)*

### Temperature board

A TMP117 precision temperature sensor placed at the vessel holder.

<a href="pcbs/temperature_board/temperature_board.pdf"><img src="pcbs/temperature_board/temperature_board.svg" alt="temperature board layout" width="480"></a>

*→ [Temperature board wiring diagram (PDF)](pcbs/temperature_board/temperature_board.pdf)*

## 3D printed parts

The vessel holder, stirrer, controller enclosure, and all vessel adapters are 3D-printable. The STL files live in [`prints/stl/`](prints/stl) and the parametric source models are the [OpenSCAD](https://openscad.org/) files [`prints/micrologger_device.scad`](prints/micrologger_device.scad) and [`prints/micrologger_controller.scad`](prints/micrologger_controller.scad). File names ending in `_nozzle_0.4mm` are optimized for a 0.4 mm printer nozzle.

[![µLogger device](prints/png/micrologger_device_nozzle_0.4mm.png)](prints/README.md)

The parts fall into a few groups:

- **Device body & stirrer** — the high-strength vessel-holder body (`micrologger_device_*`, designed to be printed in carbon-fibre-reinforced filament) that houses the sensor/beam optics, and the magnetically coupled `micrologger_stirrer_*`.
- **Vessel adapters** — magnetically secured, radially sprung adapters sized for specific culturing vessels: `balch`, `hungate`, `culture60`, `gl45`, `serum60`/`serum120`/`serum160`, plus the `base_*` adapters that go into the base of the vessel holder for small vial sizes.
- **Light adapters** — `micrologger_light_adapter_up`/`_down` for the optional illumination add-on.
- **Controller enclosure** — `micrologger_controller_body`, `micrologger_controller_back`, and `micrologger_controller_magnet_holder` for the "brain" controller box.

👉 See [**`prints/README.md`**](prints/README.md) for the complete list of all printable parts with rendered previews.

## Building, flashing & monitoring

The firmware is written for the Particle Photon 2 (platform `p2`) and built with the [Particle Cloud command line interface (CLI)](https://github.com/spark/particle-cli). Builds are driven by Ruby [Rake](https://ruby.github.io/rake/) (see [`particle_rake.rb`](particle_rake.rb), imported by the [`Rakefile`](Rakefile)) rather than `make`.

First-time setup:

- clone this repository **with submodules** (the `lib/` libraries are git submodules): `git clone --recurse-submodules ...`, or run `git submodule update --init --recursive` in an existing clone
- install the [Particle CLI](https://github.com/spark/particle-cli) and log into your account with `particle login`
- install the ruby gems with `bundle install`

Common Rake shortcuts (run `rake help` for the full list and `rake programs` for the list of compilable programs):

- `rake micrologger` — compile the main firmware in the Particle cloud (`bin/micrologger-p2-<version>-cloud.bin`)
- `rake local micrologger` — compile locally instead (requires the [Particle Workbench](https://docs.particle.io/getting-started/developer-tools/workbench/) local toolchain installed under `~/.particle`)
- `rake dev micrologger` — local compile + flash + watch sources and auto-recompile/-flash on change (via [Guard](Guardfile))
- `rake flash` — flash the newest binary in `bin/` to a USB-connected device (must be in DFU / blinking-yellow mode)
- `rake flash DEVICE=<name>` — flash the newest binary over-the-air via the cloud
- `rake monitor` — open the USB serial monitor
- targets chain, e.g. `rake micrologger flash monitor` compiles, flashes, then monitors

The `tests/` folder holds standalone test programs that build the same way (e.g. `rake blink`, `rake motor`). Which sources/libraries each program compiles is defined in its `.github/workflows/compile-<program>.yaml` file, which drives both local `rake` builds and continuous integration.

## Communicating with the device

The µLogger firmware is built on **self-describing data structures (SDDS)** using the [SDDS library](https://github.com/mLamneck/SDDS) and the [SDDS particleSpike](https://github.com/KopfLab/SDDS_particleSpike). The entire device — every setting, action, and live reading — is exposed as a single SDDS tree (see [The SDDS structure tree](#the-sdds-structure-tree) below).

The primary way to view and control a µLogger is the web-based [sddsParticle](https://github.com/KopfLab/sddsParticle) GUI, which reads a device's structure tree and automatically builds an editor for it. You can also interact with a device directly via the [Particle CLI](https://github.com/spark/particle-cli) (or the equivalent [Particle Cloud API](https://docs.particle.io/reference/api/) requests). All calls require being logged in (`particle login`) and only work for devices registered to your account.

**Reading the tree** (Particle cloud variables):

- `particle get <deviceID> getSddsValues` — the current values of all SDDS variables
- `particle get <deviceID> getSdds` — the full structure tree (types + values)
- `particle get <deviceID> getSddsSystem` — just the `SYSTEM` subtree
- `particle get <deviceID> getSddsCommandLog` — the log of recently received commands

**Setting variables / issuing commands** (Particle cloud function `sdds`): assign values with a `path=value` syntax where the path uses `.` as the separator. Issue several assignments at once by separating them with spaces:

```sh
particle call <deviceID> sdds "sensor.action=zero"
particle call <deviceID> sdds "stirrer.setpoint_rpm=500 stirrer.action=start"
particle call <deviceID> sdds "lights.action=schedule"
```

The return value is `0` when all assignments succeed. For a single failed assignment it is the negated error code (e.g. `-2` when the path could not be resolved); for a batch of assignments it is a bitmask flagging which ones failed. Special codes: `-200` = empty command, `-201` = more than 31 assignments in one call.

**Pushing data to the cloud on demand** (Particle cloud functions): `sendSdds`, `sendSddsValues`, and `sendSddsState` publish the structure tree, all values, or just the saveable state, respectively. Regular data logging is published on the **`microloggerData`** cloud event and configured through the `SYSTEM` structure (see the [SDDS particleSpike documentation](https://github.com/KopfLab/SDDS_particleSpike#the-system-structure)).

## The SDDS structure tree

Every µLogger exposes its complete state as an SDDS tree. Fields can be flagged as _read-only_ (reported by the device but not meant to be edited by the user) or _saveable_ (persisted across restarts, i.e. restored on boot and re-saved with the `SYSTEM` `saveState` action). Numeric fields carry their unit as a name suffix (e.g. `_ms`, `_sec`, `_rpm`, `_V`, `_C`, `_Ohm`, `_percent`, `_ppt` for parts-per-thousand ‰, `_dt` for a date/time, `_HHMM` for a 24h clock time). Actions are one-shot triggers that automatically reset to `___` (no action) after they run.

The top level of the tree contains the `SYSTEM` structure, the `device` connection flag, the four functional components (`sensor`, `stirrer`, `lights`, `environment`), and a low-level `HARDWARE` menu.

### SYSTEM

The `SYSTEM` structure is added automatically by the SDDS particleSpike and carries the device's identity, connection status, health/vitals, persisted-state handling, and all data recording/publishing settings. It is documented in full in the [SDDS particleSpike documentation](https://github.com/KopfLab/SDDS_particleSpike#the-system-structure).

### device

- **`device`** — _read-only_ — whether the controller (the "brain") is connected to the reader (the culture vessel holder) via RJ45 cable: `connected` or `disconnected` (detected via the reader's I2C bus).

### sensor

Optical-density measurement — the core function of the µLogger.

- **`action`** — trigger a one-off measurement action:
  - `zero`: establish the OD = 0 reference for the current culture vessel (typically done once before inoculation). If `gain.automatic` is `yes`, the gain is auto-optimized first, then paused for `gain.zeroPause_ms`, then the zero is recorded. The zero is persisted so it survives power outages.
  - `beamOn`: turn the measurement beam LED permanently on (e.g. to watch the live `reading.saturation_ppt`)
  - `beamOff`: turn the measurement beam off
  - `optimizeGain`: run the automatic amplifier-gain optimization to bring the signal to `gain.target_ppt`
  - `reset`: clear the zero reference and return to idle
- **`status`** — _read-only_ — `idle`, `reading`, `optimizing`, `waiting`, or `zeroing`
- **`reading`** — the latest measurement plus read timing/behavior settings
  - **`saturation_ppt`** — _read-only_ — the current live beam signal as a fraction of the ADC maximum, in ‰; watch this to avoid saturating the detector
  - **`readInterval_ms`** — _saveable_ — how often to take an OD reading (default: 2 minutes)
  - **`vortex`** — _saveable_ — vortex (pulse-agitate) the culture before each read (default: no)
  - **`stopStirrer`** — _saveable_ — stop the stirrer before each read (default: yes)
  - **`stopLights`** — _saveable_ — turn the light off during each read (default: yes)
  - **`wait_ms`** — _saveable_ — settling time after vortex/stirrer stop before reading (default: 0)
  - **`warmup_ms`** — _saveable_ — beam warm-up time whenever the beam turns on (default: 3000)
  - **`cooldown_ms`** — _saveable_ — wait after the beam turns off before reading the dark background (default: 500)
  - **`nextRead`** — _read-only_ — human-readable countdown to the next read
  - **`signal`** / **`signalSd`** — _read-only_ — mean and standard deviation of the beam (lit) signal of the last read
  - **`bgrd`** / **`bgrdSd`** — _read-only_ — mean and standard deviation of the dark-background signal of the last read
  - **`transmittance`** / **`transmittanceSd`** — _read-only_ — light transmittance relative to the zero reference, and its error
  - **`OD`** / **`ODSd`** — _read-only_ — optical density (−log₁₀ of transmittance) and its error
- **`zero`** — the zero reference, persisted so it survives power outages
  - **`last_dt`** — _read-only, saveable_ — timestamp of the last zeroing (`never` if never zeroed)
  - **`valid`** — _read-only, saveable_ — whether a valid zero exists; OD is only computed once this is `yes`
  - **`signal`** / **`signalSd`** — _read-only, saveable_ — beam signal (and SD) recorded at zeroing
  - **`bgrd`** / **`bgrdSd`** — _read-only, saveable_ — dark background (and SD) recorded at zeroing
- **`beam`** — the measurement-beam LED
  - **`status`** — _read-only_ — `on`, `off`, or `error`
  - **`error`** — _read-only_ — I2C error of the beam control
- **`gain`** — the signal amplifier gain
  - **`automatic`** — auto-optimize the gain before zeroing (default: yes; not persisted, so it must be re-enabled each session)
  - **`zeroPause_ms`** — _saveable_ — settling pause after automatic gain optimization before zeroing (default: 60000)
  - **`max_ppt`** — _read-only_ — the maximum attainable signal, in ‰
  - **`target_ppt`** — _saveable_ — target signal level for gain optimization, in ‰ (default: 920)
  - **`gain_Ohm`** — _read-only, saveable_ — the current total amplifier resistance (i.e. the gain), persisted across restarts
  - **`status`** — _read-only_ — `set` or `error`
  - **`error`** — _read-only_ — I2C error of the gain control
- **`error`** — _read-only_ — top-level OD error: `none`, `saturated`, `failedGain`, `failedZero`, or `failedRead`

### stirrer

The magnetically coupled stirrer built into the vessel holder.

- **`action`** — trigger a one-off stirrer action:
  - `start` / `stop`: turn continuous stirring on/off (also updates the persisted `state`)
  - `pause` / `resume`: temporarily stop/restart the motor **without** changing `state` (used internally during reads)
  - `vortex`: run a single vortex pulse — ramp up to `settings.vortexSpeed_rpm`, hold for `settings.vortexTime_sec`, then return to the previous speed
- **`state`** — _read-only, saveable_ — the persisted on/off intent of the stirrer (default: off)
- **`status`** — _read-only_ — `off`, `accelerating`, `decelerating`, `running`, or `error`
- **`error`** — _read-only_ — motor error
- **`event`** — _read-only_ — transient activity: `none`, `paused`, or `vortexing`
- **`setpoint_rpm`** — _saveable_ — target stir speed in rpm (default: 500)
- **`speed_rpm`** — _read-only_ — the measured actual speed in rpm
- **`settings`**
  - **`acceleration_rpm_s`** — _saveable_ — acceleration ramp in rpm per second (default: 500)
  - **`deceleration_rpm_s`** — _saveable_ — deceleration ramp in rpm per second (default: 3000)
  - **`maxSpeed_rpm`** — _saveable_ — maximum allowed speed (capped by the motor hardware limit)
  - **`vortexSpeed_rpm`** — _saveable_ — the speed of a vortex pulse (default: 3000)
  - **`vortexTime_sec`** — _saveable_ — how long to hold at vortex speed (default: 2)

### lights

The optional illumination adapter (for experiments with phototrophic microorganisms) and its cooling fan.

- **`action`** — trigger a one-off light action:
  - `on` / `off`: turn the light permanently on/off
  - `schedule`: run the light on a repeating on/off schedule (see the `schedule*` settings below)
  - `pause` / `resume`: temporarily turn the light off/back on without changing `state` (used during reads when `sensor.reading.stopLights` is yes)
- **`state`** — _read-only, saveable_ — the persisted light mode: `on`, `off`, or `schedule` (default: off)
- **`status`** — _read-only_ — the actual light output: `on`, `off`, or `error`
- **`event`** — _read-only_ — `none` or `paused`
- **`error`** — _read-only_ — I2C error of the light/fan driver
- **`intensity_percent`** — _saveable_ — light brightness, 1–100% (default: 100)
- **`scheduleOn_sec`** — _saveable_ — how long the light stays on per schedule cycle (default: 12 hours)
- **`scheduleOff_sec`** — _saveable_ — how long the light stays off per schedule cycle (default: 12 hours)
- **`scheduleOnStart_HHMM`** — _saveable_ — the time of day the on-phase begins, as a 24h `HHMM` value (default: 1200 = noon)
- **`scheduleInfo`** — _read-only_ — human-readable countdown to the next on/off switch
- **`fan`** — the cooling fan for the light adapter
  - **`action`** — `on`, `off`, or `withLight`
  - **`state`** — _read-only, saveable_ — the persisted fan mode: `on`, `off`, or `withLight` (run the fan only while the light is on) (default: withLight)
  - **`status`** — _read-only_ — `on`, `off`, or `error`

### environment

Temperature and supply-power monitoring.

- **`readInterval_ms`** — _saveable_ — how often to read the temperature sensor (default: 1000)
- **`temperature_C`** — _read-only_ — temperature measured at the vessel holder, in °C
- **`power_V`** — _read-only_ — supply voltage measured at the controller, in V
- **`powerReq_V`** — _saveable_ — the minimum required supply voltage; below this the display warns "not enough power" (default: 20.0)
- **`error`** — _read-only_ — I2C error of the temperature sensor

### HARDWARE

An advanced, low-level menu (added at the end of the tree) that exposes direct access to every peripheral — the I2C IO expander, the digital potentiometers and amplifier gain circuit, the OPT101 light sensor, the TMP117 temperature sensor, the supply-voltage divider, the PCA9633 light/fan PWM driver, the Nidec motor, the OLED display, and the detected controller/sensor PCB versions. It is intended for diagnostics and calibration only; the `sensor`, `stirrer`, `lights`, and `environment` components above are the normal interface.
