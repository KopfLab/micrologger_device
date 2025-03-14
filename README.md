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

## Features

- easily extensible framework for implementing cloud-connected µLoggers based on the secure and well-established Particle [Photon 2](https://docs.particle.io/reference/datasheets/wi-fi/photon-2-datasheet/) platform
- constructs JSON-formatted data logs for flexible recording in spreadsheets or databases via cloud webhooks
- build-in data averaging and error calculation
- built-in support for remote control via cloud commands
- built-in support for device state management (device locking, logging behavior, data read and log frequency, etc.)
- built-in connectivity management with data caching during offline periods - Photon memory typically allows caching of 50-100 logs to bridge device downtime of several hours

## Makefile

Install the [Particle Cloud command line interface (CLI)](https://github.com/spark/particle-cli), and log into your account with `particle login`. Then the following make shortcuts are available:

- to compile: `make PROGRAM` (e.g. `make tests/blink` and `make devices/micrologger`)
- to flash latest compile via USB: `make flash`
- to flash latest compile via cloud: `make flash device=DEVICE`
- to start serial monitor: `make monitor`

For additional options and make targets, see the documentation in the [makefile](makefile).

## Web commands

To run any web commands, you need to either have the [Particle Cloud command line interface (CLI)](https://github.com/spark/particle-cli) installed, or format the appropriate POST request to the [Particle Cloud API](https://docs.particle.io/reference/api/). Here only the currently implemented CLI calls are listed but they translate directly into the corresponding API requests. You only have access to the microcontrollers that are registered to your account.

### requesting information via CLI

The state of the device can be requested by calling `particle get <deviceID> state` where `<deviceID>` is the name of the photon you want to get state information from. The latest data can be requested by calling `particle get <deviceID> data`. The return values are always array strings (ready to be JSON parsed) that include information on the state or last registered data of the device, respectively. Requires being logged in (`particle login`) to have access to the photons.

### issuing commands via CLI

All calls are issued from the terminal and have the format `particle call <deviceID> device "<cmd>"` where `<deviceID>` is the name of the photon you want to issue a command to and `<cmd>` is the command (and should always be in quotes) - e.g. `particle call my-logger device "data-log on"`. If the command was successfully received and executed, `0` is returned, if the command was received but caused an error, a negative number (e.g. `-1` for generic error, `-2` for device is locked, `-3` for unknown command, etc.) is the return value. Positive return values mean executed with warning (e.g. `1` to note that the command did not change anything). The command's exact wording and all the return codes are defined in the header files of the controller and components (e.g. `LoggerController.h` and `ExampleLoggerComponent.h`). Issuing commands also requires being logged in (`particle login`) to have access to the photons.

Some common state variables are displayed in short notation in the upper right corner of the LCD screen (same line as the device name) - called **state overview**. The state overview starts with a `W` if the photon has internet connection and `!` if it currently does not (yet). Additoinal letters and their meanings are noted in the following command lists when applicable.

### available commands

See the full documentation in [docs/commands.md](docs/commands.md) for a list of available commands for the microloggers.
