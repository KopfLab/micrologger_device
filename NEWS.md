
NB:
 - version updates at the 0.x level usually require a state reset because they change/add/remove state variables, i.e. should never be done in the middle of an experiment. 
 - version changes at the 0.0.x level can be flashed at any time. They do make state changes and thus will resume their current state correctly after flash+restart.

# version 1.5.0

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
