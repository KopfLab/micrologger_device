# this is basically the ministat v2

# sensor - OPT101

## power

- can run at any voltage from 2.7 to 36V and output is **indepenent** of the input voltage but will max out ~1V below the power and no longer record higher light intensities
- to max out GPIO input (3.3V), need to drive at > 4V (5V works well) but then need to also protect the GPIO input against overvoltage which gets problematic at ~3.7 V
- to protect this, use a small signal schottky diode with low forward voltage drop (ideally ~200mV) to the 3.3V supply to clamp the voltage on the GPIO pin to 3.3V + 0.2V = ~ 3.5 V, options:
    - BAT41: at 1mA max drop is 450mV (looks like ~250mV at 0.1mA) and very low leakage current (only 100 nA at 50V) --> seems to have the lowest leak A and V drop for through hole components
    - BAT43W (SMD only): at 2mA max drop is 330mV, 450mV at 15mA
    - BAT54 (SMD only): at 0.1mA max drop is 240mV, 1mA max drop is 320mV (very similar to 43W)
    - BAT81, 82, 83: at 0.1mA max drop is 330 mV    
    - some other options: https://www.vishay.com/en/diodes/ss-schottky/


## I2C addresses:

 - MCP4652 (dual trimpot): 0x2C if address pins tied to ground
 - I2C display: 0x3c
 - IR temperature sensor: 0x5a
 - tmp117 temperature sensor: 0x48
 - TCA9534 gpio expander: 0x20 --> https://learn.sparkfun.com/tutorials/sparkfun-qwiic-gpio-hookup-guide#arduino-examples
 - PWM controller PCA9633: 0x62 for inidviual, 0x70 for AllCall address (to all PCA9633 chips), and 0x03 for software reset. This chips is a 4-bit, 4 channel fan (mosfet), light sheet (mosfet)
 - not used MCP4017: 0x2f --> https://github.com/SparkysWidgets/SW_MCP4017-Library
 - not used AD5246: 0x2e --> https://github.com/SparkysWidgets/SW_MCP4017-Library

## signal

- typically need to reduce the signal amplification with resistors < 1MOhm
- with LED driven at ~3.3V from a GPIO pin, a ~250 kOhm resistor usually maxes out the detector to its reference voltage even if it's as high as 24V
- the current is typically low, even at full intensity with a high resistor value (and corresponding signal voltage), this stays < 10µA (but will rise rapidly if it exceeds the internal GPIO overvoltage of ~3.7V) - for stability best to stay at max overvolage of 3.4V (not all the way to 3.7!), this can be reached with a supply voltage of 4.2V to the OPT100 and light saturation --> a voltage divider with 1kOhm and 6.8kOhm will lead to voltage of ~4.2V (starting with 5V supply, note that voltage divider predicts slightly higher but we have some pulls to GND that lower this slightly) which should work for this. Current draw of the opt can be up to 4mA current to OPT101 (and ~0.7mA dead current from source to sink)
- if signal instability is an issue, can place a 0.01-μF to 0.1-μF (e.g. 67 pF) bypass capacitor close to sensor to connect Vin & Gnd. Additionally for low resistanece (i.e. low amplification) at or below 100 kOhm, might also need a capacitor (33 pF for 100kOhm, higher for others) across the resistor
- pulldown resistor to signal line should be 10 kOhm or bigger (no need to have them lower since the voltage is clamped to ~3.7V), use 47 kOhm or even 100 kOhm as a more common pulldown! Note that using an external resistor is a bit more reliable than internal (digital) pulldowns which are different on different pins and from controller to controller - this resistor could be surface mounted
- a digital trimmer could provide programmatic control over the amplification, try out AD5241 (1 trimmer with 2 additional logic outputs), which could also logic control 2 other things via the I2C (e.g. motor on/off, and with a MOSFET the LED on/off) --> since the AD5241 runs 5V logic, need to use a level shifter for Argons/P2s, simplest is this tiny Pololu PCB with mosfets (https://www.pololu.com/product/2595) or slightly bigger Sparkfun PCB with BSS138 mosfets (probably works too with BS270): https://cdn.sparkfun.com/datasheets/BreakoutBoards/Logic_Level_Bidirectional.pdf - alternatively just make the level shifting circuits yourself on the controller PCB

# amplification / digital potentiometer

need to be able to cover a range of ~200 kOhm but the most commonly available are 100 kOhm (1 MOhm could work too but then the gain adjustment is too course), this is achieved most easily by a either a dual 100kΩ rheostat or two separate 100kΩ rheostats

## 1 dual rheostat

 - MCP4652 100kΩ Dual Rheostat, tempco 50 ppm/C, 8-bit (256 steps): https://jlcpcb.com/partdetail/MicrochipTech-MCP4652T_104EMF/C623638
 - alternative (with 2 extra pins): MCP4651 100kΩ Dual Potentiometer version of MCP4652 https://jlcpcb.com/partdetail/MicrochipTech-MCP4651_104EST/C637129
 - AD5248 is the equivalent to MCP4652 (8-bit 100kΩ Dual Rheostat) but only 35 ppm/C tempco, also a lot more expensive and not as available: https://jlcpcb.com/partdetail/AnalogDevices-AD5248BRMZ100/C144252, this might be useful for programming: https://learn.adafruit.com/ds3502-i2c-potentiometer/overview

## 2 single rheostats

 - AD5246 100kΩ Single Rheostat, tempco 45ppm/C, 4-bit (128 steps): https://jlcpcb.com/partdetail/AnalogDevices-AD5246BKSZ100RL7/C649406
 - MCP4017 100kΩ Single Rheostat, tempco 50ppm/C, 4-bit (128 steps): https://jlcpcb.com/partdetail/MicrochipTech-MCP4017T_104ELT/C510890 (discontinued)
 - newer version: MCP40D17 (same specs as MCP4017): https://jlcpcb.com/partdetail/MicrochipTech-MCP40D17T_104ELT/C115214 (not available yet)
 - MCP4532 100kΩ Single Potentiometer, tempco 50ppm/C, 4-bit (128 steps): https://jlcpcb.com/partdetail/MicrochipTech-MCP4532T_104EMS/C627275



# LED - 630nm (MTE7063NK2)

 - this LED has 50mA max current but best to drive at 20-30mA where it has ~1.9 V forward voltage
 - with 47 Ohm resistor and 3.3V supply this runs at ~30mA
 - with 100 Ohm resistor and 5V supply this runs at ~31mA  
 - can drive the LED from the AD5241 extra logic output with a BS170 or BS270 (lower gate voltage?) mosfet (or the equivalent SMD component) --> keep in mind that as diodes these have forward voltage drops that will aslo affect the LED current! need to test this out but it should only be a few mV


# temperature sensing

 - use a temp sensor via I2C since that's already going to the micrologger now
 - ideally use tmp117 (https://cdn.sparkfun.com/assets/2/0/f/c/a/SparkFun_TMP117_Qwiic_Schematic_v1.pdf, address 0x48) but I don't see a good packaged version yet, alternatively the tmp100? another interesting option is a low angle (10degree) IR temperature sensor, such as this one: https://www.digikey.com/en/products/detail/melexis-technologies-nv/MLX90614ESF-ACF-000-TU/2652200 that can communicate via I2C like protocol, this one could go right underneat the light sensor on the same PCB and going through the wall of the micrologger (it's okay if it records IR from the stir bar + liquid) - for notes on how to get started with communication, check out this testing board: https://www.sparkfun.com/products/10740

# display

 - switching to oled: https://www.youtube.com/watch?v=7x1P80X1V3E with this model (which is easily available from various amazon vendors for about $14): https://www.amazon.com/gp/product/B09LND6QJ1
 - needs to be switched from SPI to I2C
 - great resource: https://luma-oled.readthedocs.io/en/latest/hardware.html
 - great summary of the different resistors: https://www.amazon.com/gp/customer-reviews/R373WMLIYUB5MF/ref=cm_cr_dp_d_rvw_ttl?ie=UTF8&ASIN=B09LND6QJ1
 - attach to PCB with M2.5 x 28mm threaded spacers from Mouser (761-M1273-2545-AL) and with narrow head M2.5 x 6mm machine screws from McMaster (cheaper: 91800A126 or nicer: 94017A152)
 - don't want to have it on the 3.3V so we don't have any interference but also a problem if it's on the 5V when the 5V rail is competing between the USB and DC/DC adapter



# lights

 - Auragami light sheets need 24V power and are 26mW (~1.1 mA at 24V) per LED --> with 40 LEDs this should only be about 50mA

# motor

 - Nidec 24H brushless servo motor requires 22.8 to 25.2V to power. No-load current at 24V (5200 to 6400rpm) is about 110mA, max power is 11W (~460mA)
    -> pin 8/red: power (+24VDC)
    -> pin 7/black: ground (GND)
    -> pin 6/yellow: speed (as long as break is not engaged), freq 20 to 30 kHz (25000 works welll), duty cycle = speed (0 = stop, 255 = max speed)
      note: LED dimmers like the PCA9633 we're using cannot operate at this frequency, the simplest solution if we do not want to use a line on the RJ45 for motor speed is to use a small microcontroller (e.g. ATtiny402) on the sensor board side that talks to the photon by I2C and takes care of the PWM for the motor, the LED on/off and the light control
    -> pin 5/green: break, HIGH = engaged (stop), LOW = not engaged (pin 6 decides speed) --> set permanently to LOW
    -> pin 4/blue: direction, HIGH (2 to 5V) = CW, LOW = CC --> set permanently to one or the other (jumper selectable?)
    -> pin 3/white: power for the decoder, can be 5 VDC or 3.3 VDC --> pin 7/8 signals are this voltage use 3.3 VDC OR use a level shifter (can use multiple channel ones, one for the I2C, one for this?)
    -> pin 2/orange: signal of the optical decoder, seems like it has 100 pulses per revolution so freq (in Hz) / 100 * 60 = rpm
    -> pin 1/brown: offset (falling edge) of the decoder, same info as pin 2 if we already know cc vs cw
        - 0/255: stop
        - 1/255: doesn't work
        - 2/255: 37 Hz ~ 22 rpm
        - 3/255: 78 Hz ~ 47 rpm
        - 4/255: 116 Hz ~ 70 rpm
        - 5/255: 153 Hz ~ 92 rpm
        - 10/255: 330 Hz ~ 198 rpm
        - 20/255: 694 Hz ~ 416 rpm
        - 50/255: 1.6 kHz ~ 960 rpm ~ 50 mA without load
        - 100/255: 3.1 kHz ~ 1860 rpm ~ 140 mA without load
        - 150/255: 4.9 kHZ ~ 2900 rpm ~ 170 mA without load
        - 255/255: 9.54 kHz ~ 5700 rpm ~ 70 mA without load (current max without load actually seems to be around 150/255)
        --> estimated formula for 8 bit: x/255 --> rpm = (x-1) * 22 rpm (response factor more like 20rpm around 150/255 and then goes back to 22 rpm again)
        --> for 12 bit:
        --> x = (rpm/22 + 1) * 16 = 16 + rpm * 16/22
        - with interrupt delays of about 40-70uS we should be able to measure the entire range (10 kHz = 100µs interrupts) but I'm not sure if anything else will mess with that during operation

## motor power

 - the speed pin (6/yellow) needs to be pulled low to GND otherwise the motor spins on start-up (unless break is engaged which can cause other problems)
    --> if pulled low with 1kOhm resistor the voltage is about 0.44V (see analysis below) which suggests the motor has an internal pullup on this pin to 5V of about ~10 kOhm
    --> if controlling this with a mosfet, need to provide an alternative path to high voltage that pulls the voltage above ~0.7V (also considering the body diode voltage drop of the moseft, ~0.8V at most)
    --> see circuit analysis
 - resistor values for pull down to ground (assumes ON = direct connection to 5V):
    - 2.3kOhm: pull down does NOT work because effective voltage on the pin is 1.34V (counts as HIGH), 0.35 mA to GND
    - 2.2k: 0.845 V, 0.4 mA to GND when off, 2.2mA to GND when on
    - 1.5k: 0.597 V, 0.42 mA to GND when off, 3.34mA to GND when on (2mA if 3.3V supply voltage)
    - 1k: 0V, 0.44 mA to GND when off, 5mA to GND when on (3mA if 3.3V supply voltage)
 --> with a BSS138 use a 1000 Ohm resistor to pull to ground (with the 10kOhm pull up internally to 5V, this gives a voltage of 0.44V which is low enough to deactivate the motor and only a 0.5mA standby current)
 --> then the mosfet shifts it with a parallel resistor to 5V that is 1kOhm which makes the net resistor 900 Ohm and pulls the voltage to ~2.5V which is enough to activate motor and a total of ~2.6 mA current which is totally fine
 --> I modeled this in a circuit lab circuit (but don't have good access)
 Note that this circuit is divided between the sensor board (with the pulldown to GND to have it off by default), and the brain where the I/O step up from 3.3 V to 5V is located (unidirectional level shifter if you will) mostly beccause space is abundant on this end but really limited on the sensor board but in theory this could all be sitting on the sensor board side

## motor attachment

corner standoffs for micrologger base
- base is 34 mm high
- rotor is 20 mm above base
- magnet holder is at least 3.5 mm tall (3.2 mm magnets plus 0.25mm base layer)
- i.e. total is 57.5mm
- with the longest metric standoffs from McMaster (52mm long, 95947A087) this requires a rather thick base (>5.5mm) so need to go with an imperial standoff: 2.25" is 57.15mm long (93505A006) which is male/female and thus can just thread directly into the nylon base

mounting standoffs for motor itself
- base plus mounting bracket is 3.5 mm above the motor base so 20mm shaft + 3.5 mm magnet holder - 3.5mm bracket height  - 4mm µlogger base thickness = 16mm M4 standoffs (McMaster, 95947A050). For a little extra clearance might have to make the base 5mm thick!
- plus 6mm round head M4 screws (92000A216) to attach from motor side, and 10mm flat head M4 screws (92010A220) to attach through µLogger base

## motor magnets
- 3.2 mm above the motor shaft, design/print upside down with a thin layer (0.25 mm) base then the magnet holes above (unroofed since the top of the motor assembly will hold them in place) and then the side walls above where there is solid support underneath to grip the motor shaft with the teeth walls --> total is ~3.5 mmm above the motor shaft so total then becomes 54mm + 3.5mm = 57.5mm which requires a rather thick base (~5.5mm to still work with the 52mm feet)
-  


# overall power

power over ethernet technical specs (up to 100m distance): https://www.blackbox.com/en-nz/insights/blogs/detail/technology/2021/05/04/poe-cabling-what-you-need-to-know-part-1
 - PoE+	IEEE 802.3at (Type-2)	25.5 W	30 W	2-pair	0.6 A	PTZ cameras, video IP phones, alarm systems
 - PoE++ IEEE 802.3bt (Type 4)	71.3 W	100 W	4-pair	0.96 A	PoE-Powered LED Lighting, outdoor PTZ cameras

but technical specs:
    Maximum Current Carrying Capacity of Ribbon Cable (while keeping temperature rise to less than 30 °C)
    24 AWG	3.0 A
    26 AWG	1.8 A
    28 AWG	1.5 A
    30 AWG	1.1 A

# power supply voltage

 - R1: 10kOhm = 9.93
 - R2: 200kOhm = 200.1

forward voltage drop across the M400x diode is 0.6 V with minimal current and ~0.9 V at 1A (probably best to calculate with assumed 0.9V drop)

sensing calculation:
V at A0 = Vin * R1/(R1+R2) --> Vin = A0 * (10 + 200)/10 + 0.9V

 External voltage sensing on P2 pin A0:
  - R1 = 150 kOhm, R2 = 10 kOhm
  --> V at A0 for 24V --> 24V * 10/(10+150) = 1.5External voltage sensing on P2 pin A0:
   - R1 = 150 kOhm, R2 = 10 kOhm
   --> V at A0 for 24V --> 24V * 10/(10+150) = 1.550

# ethernet cable

 - 8p8c: 1=Gnd, 2=24V, 3=I2C-SDA, 4=I2C-SCL, 5=Motor-speed, 6=Detector-signal, 7=Light-intensity, 8=Motor-decoder
 -> 24V to 5V required on distribution board; level shifters required to convert 5V from I2C and motor-decoder

# connectors

 - the JST XHP connectors (2.54mm) are used by the light sheets and are easy to buy from amazon, including different length cables that can be easily assembled
 - the JST PH (2.0mm) connectors are primarly used to connect batteries (e.g poli) and usually are male to female connectors
 - the JST ZH connectors (1.5mm, 26 AWG, 1.0 A rating), are used by the motor connectors / JST ZR connectors are compatible but lower amperage rating (0.7A, 28 AWG)

# housing
 - threaded spacers between OLED and main PCB are M2.5, 28mm long: https://www.mouser.com/ProductDetail/761-M1273-2545-AL
 - standoff spacers between main PCB and back wall are M3, 12mm long (this length does not seem to be available for M2.5): https://www.mcmaster.com/94669A307/


# checklist

 - power up with 24V / GND pins (not via ethernet!)
 - when powered up but beam OFF:
    - signal vs. gain resistance should be ~200kOhm (100 kOhm base resistor + 2 * 50 kOhm = midpoints of the digipots), doesn't matter if motor and/or light are also on
    - when a little bit of light is shining on the detector, the resistance should go up (with much light into several MOhm and too high to measure)
    - signal vs. ground voltage should be < 20mV, when detector opening completely covered ~ 10mV (this is a bit higher with motor and/or lights on and/or beam on but detector covered up)
    - when a little bit of light is shining on the detector, the voltage should go up
 - when powered up and beam ON:
    - signal vs gain restiance should be immesurably high (unless detector is covered in which case it should be back to ~200 kOhm)
    - signal vs ground voltage should be at ~3.2 V
 - plugging in the ethernet cable should only have minimal effects (both loose cable and connector to empty/microcontroller less brain)
 - neither should switching to brain powersupply have any effect
