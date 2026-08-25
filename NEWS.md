
NB:
 - version updates at the 0.x level usually require a state reset because they change/add/remove state variables, i.e. should never be done in the middle of an experiment. 
 - version changes at the 0.0.x level can be flashed at any time. They do make state changes and thus will resume their current state correctly after flash+restart.


# version 1.6.1

- fix `sensor.gain.gain_Ohm` not being restored correctly after a restart on sensor board version 2+. The components resumed their state at the moment the I2C expander first connected, which happened just *before* the sensor board version is detected. State resume now waits until `HARDWARE.pcbVersions.sensor` is known.
- the gain readback no longer writes back into `sensor.gain.gain_Ohm` while the gain hardware is in an error state, so a failed dpot write can never erase the stored gain
- note: devices running 1.6.0 on a version 2+ sensor board have a corrupted gain saved in their state. After flashing, run `sensor.action=optimizeGain` once and save the state to restore a sensible value.

# version 1.6.0

- support for sensor board version 2+, where the secondary gain potentiometer is a 1 MOhm AD5241 (`HARDWARE.dpot3`) instead of the 100 kOhm AD5246 (`HARDWARE.dpot2`) used on version 1 boards. The sensor board version is detected over I2C whenever the IO expander connects and only the potentiometer actually populated on the board is ever sent I2C commands.
- new hardware variable `HARDWARE.dpot3` (AD5241) added to the tree
- maximum gain extended from ~256 kOhm to ~1.11 MOhm on sensor board version 2+
- gain steps are now always counted in fine (`dpot1`-sized) units so the gain scale stays uniform and monotonic no matter which secondary potentiometer is populated
- `HARDWARE.gain.base_Ohm` default changed from 56 kOhm (sensor board version 1) to 10 kOhm (sensor board version 2+)
- fixes the sensor board version bit order (expander pins 6 and 8 were swapped, so versions were read back reversed)

# version 1.5.1

- publish on the `microloggerData` event instead of `sddsData` for microloggers
- rename `publishing.publish` to `publishing.record` to clarify how it relates to GUI recording on/off
- hardware `signal.calculation` added with two possible values: `mean` (default, average signal values) or `mode` (determine peak of the signal values)
- optical density `reading.stopLights` added for users to decide whether lights turn off or stay on during OD reads (default is stopLights = yes)
- system `state.autoSendOnStartup` added for users to decide whether to send the state snapshot whenenver a device that's recording starts up (e.g. after power cycling), default is ON
- introduce particle variable `getSddsSystem` as convenience access piont that returns the system variable tree only (~800 bytes in size) except for the variable publish intervals (which can be any size)
- change default read interavl to 2 min and default publish interval to 20 min

# version 1.4.0

- `cooldown` (wait time until dark measurement after LED beam turned off, default: 500 ms) added - this value is saved in the micrologger state
- `zeroPause` (wait time until zeroing when automatic gain adjustment happens during zeroing, default: 60s) introduced - this value is saved in the micrologger state
- default values for `warmup` (default: 3s) and `wait` (default: 0s) adjusted since the longer warmup leads to a more stable signal but usually does not require additional wait time anymore for the culture to settle
