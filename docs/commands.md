# Available commands

## Controller State

  - `state-log on` to turn web logging of state changes on (letter `S` shown in the state overview)
  - `state-log off` to turn web logging of state changes off (no letter `S` in state overview)
  - `data-log on` to turn web logging of data on (letter `D` in state overview)
  - `data-log off` to turn web logging of data off
  - `log-period <options>` to specify how frequently data should be logged (after letter `D` in state overview, although the `D` only appears if data logging is actually enabled), `<options>`:
    - `3 x` log after every 3rd (or any other number) successful data read (`D3x`), works with `manual` or time based `read-period`, set to `1 x` in combination with `manual` to log every externally triggered data event immediately (**FIXME**: not fully implemented)
    - `2 s` log every 2 seconds (or any other number), must exceed the `read-period` (`D2s` in state overview)
    - `8 m` log every 8 minutes (or any other number)
    - `1 h` log every hour (or any other number)
  - `read-period <options>` to specify how frequently data should be read (letter `R` + subsequent in state overview), only applicable if the controller is set up to be a data reader, `<options>`:
    - `manual` don't read data unless externally triggered in some way (device specific) - `RM` in state overview
    - `200 ms` read data every 200 (or any other number) milli seconds (`R200ms` in state overview)
    - `5 s` read data every 5 (or any other number) seconds (`R5s` in state overview)
  - `lock on` to safely lock the device (i.e. no commands will be accepted until `lock off` is called) - letter `L` in state overview
  - `lock off` to unlock the device if it is locked
  - `debug on` to turn debug mode on which leads to more data reporting in newer loggers (older loggers don't have this option yet)
  - `debug off` to turn debug mode off
  - `restart` to force a restart
  - `reset state` to completely reset the state back to the default values (forces a restart after reset is complete)
  - `reset data` to reset the data currently being collected
  - `page` to switch to the next page on the LCD screen (**FIXME**: not fully implemented)

## Optical Density

  - `reference yes/no` : whether the optical density reader has a beam splitter and reference beam optode
    - `yes` : OD reader has a beam splitter and reference optode and it should be used to correct the signals (`OD+R` will show up on the LCD as data prefix)
    - `no` : OD reader does not have a reference optode or it has one but it should not be used to correct the signals (`OD` will show up on the LCD as data prefix)
  - `beam on/auto/pause/off` 
    - `on` : to turn the beam permanently on to see the signal/reference intensity (as percentages of the maximum detector signal)
    - `auto` : to have the beam turn on/off automatically as needed for OD readings. This mode needs to be active for OD readings to occur.
    - `pause` : to pause the beam and thus OD readings (e.g. to remove the bottle and bring it back later). To resume OD readings, switch the beam back to `auto`. 
    - `off` : to turn the beam permnantely off which also resets the `zero` information for the logger (i.e. requires zero-ing again before reading ODs).
  - `zero` + `next` to zero the optical density logger for the current media bottle (typically this is done once before inoculation). When the `zero` command is sent, the display shows the current beam reading to allow adjustment of the variable resistor on the sensor (ideally to a signal reading of ~95% but careful not to go too high and saturate the sensor) until the `next` command is sent which will then do the actual zero-ing. The zero values are the reference against which OD is calculated (i.e. it becomes OD = 0.0) and is stored in the logger to protect against power outtages (i.e. it will remember its own zero) until `beam off` or another `zero` command is sent.
  - `read-length <options>` how long should dark background and beam be integrated for each read. NOTE: this command is not yet implemented but always uses the defaults of a 200 ms warmup and 500 ms read. `read-length * 2 + warmup * 2` must be shorter than the `read-period` to allow for enough time to cool down LED,read dark background, warmup the LED and read the beam. 
     - examples: `500 ms` read for 500ms, `1 s` read for 1s
  - `warmup <options>` how long to warm up the LEd for before taking a reading, example values: `100 ms`, `1 s`. NOTE: this command is not yet implemented but always uses the defaults of a 200 ms warmup and 500 ms read.

## Stirrer

  - all `LoggerController` commands PLUS:
  - `speed manual` to let the overhead stirrer speed be set manually from the stirrer panel (records what the user sets it too)
  - `speed <value> rpm` to set the speed of the overhead stirrer in rotations per minute (for the JKem stirrer this is between 50 and 750 rpm)
  - `start` start the stirrer at the specified rpm
  - `stop` stop the stirrer
