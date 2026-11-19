
NB:
 - version updates at the 0.x level usually require a state reset because they change/add/remove state variables, i.e. should never be done in the middle of an experiment. 
 - version changes at the 0.0.x level can be flashed at any time. They do make state changes and thus will resume their current state correctly after flash+restart.


# version 1.8.0

- update to SDDS_particleSpike 0.4.0: new saveable setting `SYSTEM.publishing.bursts.maxPoints` (default 512) queues a burst for publishing as soon as it reaches that many data points. New read-only `SYSTEM.publishing.bursts.points` shows how many data points are in the current burst. This changes the state structure, i.e. requires a state reset.
- the published (averaged) stirrer speed `stirrer.speed_rpm` now only averages steady stirring. Every speed transition (pauses for OD reads, vortexing, setpoint changes, start/stop) is excluded from the average and instead delimited by single data points (`n: 1`): when a transition starts, the average so far (if it has at least 2 values) and the last speed before the transition; when it is finished (+ one motor read interval for a fresh speed reading), the new speed, after which averaging resumes (also during pauses and at the vortex peak). These points do not trigger extra publishes, they are sent with the next regular burst. The live `speed_rpm` still shows the actual speed. (requires SDDS_particleSpike 0.4.0)
- the stirrer speed `stirrer.speed_rpm` is no longer recorded by default (`varIntervals_ms` = `0`); set `SYSTEM.publishing.varIntervals_ms.stirrer.speed_rpm` to `-1` to record it as described below

# version 1.7.0

- **requires a state reset** (the `SYSTEM` tree changed with the SDDS_particleSpike update, see its NEWS for details)
- bluetooth access to the SDDS tree (e.g. with the SDDS browser at https://devices.kopflab.org or `rake sddsBrowser` in SDDS_particleSpike), with `SYSTEM.bluetooth` (`state`, `status`, `connection`); bluetooth clients also set the device time if it has none yet (local wifi only, no cloud)
- `SYSTEM.wifi` submenu: turn wifi on/off (`state`, saved), `status`, `mac`, `network`, `signal_percent` and a `setup` submenu to add or replace wifi networks (e.g. via bluetooth); `SYSTEM.action` no longer has `disconnect`/`reconnect`
- `SYSTEM.vitals.timeSource` shows where the device time came from (`cloud`, `bluetooth`)
- data recorded while offline is kept until the device is online (split into bursts that fit into a cloud event, oldest data discarded only when RAM runs low) and can be downloaded via bluetooth with the SDDS browser (*Fetch device data*) without removing it from the queue
- averaged values with a single data point in an interval are now published with `n = 1` (previously without `n`, like a discrete value)
- fix zeroing with automatic gain adjustment: the stirrer (and lights) now resume their normal operation during the pause between the gain adjustment and the zero (`sensor.gain.zeroPause_ms`), so the zero goes through the same stir/vortex/stop sequence under the same conditions as every regular read (previously the stirrer stayed stopped for the entire pause). The zero also starts right when the pause is over.
- display: no wifi symbol at all when wifi is turned off (`SYSTEM.wifi.state=OFF`), bluetooth symbol while a bluetooth client is connected

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
