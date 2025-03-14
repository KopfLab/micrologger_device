### USAGE ###

# install the CLI from https://github.com/spark/particle-cli
# log into your particle account with: particle login
# to compile: make PROGRAM 
# to flash latest compile via USB: make flash
# to flash latest compile via cloud: make flash device=DEVICE
# to start serial monitor: make monitor
# to compile & flash: make PROGRAM flash
# to compile, flash & monitor: make PROGRAM flash monitor

### PARAMS ###

# which microcontroller to use?
# micrologger is designed for p2, others may or may not work
# others: photon, argon, boron
PLATFORM?=p2

# recommended versions
# see https://docs.particle.io/reference/product-lifecycle/long-term-support-lts-releases/
p2_VREC?=5.9.0
photon_VREC?=2.3.1
argon_VREC?=4.2.0
boron_VREC?=4.2.0

# which version to use? default is the recommended one for the platform
VERSION?=$($(PLATFORM)_VREC)

# flash to a specific device via the cloud?
device?=

# binary file to flash, if none specified uses the latest that was generated
BIN?=

### PROGRAMS ###

MODULES:=
tests/blink: 
devices/micrologger: MODULES=modules/logger modules/oled modules/stirrer

### HELPERS ###

# list available devices
list:
	@echo "\nINFO: querying list of available USB devices..."
	@particle usb list

# get mac address
mac:
	@echo "\nINFO: checking for MAC address...."
	@particle serial mac

# start serial monitor
monitor:
	@echo "\nINFO: connecting to serial monitor..."
	@trap "exit" INT; while :; do particle serial monitor; done

# dfu mode
dfu:
	@particle usb dfu

# start photon repair doctor
# note: use https://docs.particle.io/tools/doctor/ now, particle doctor no longer supported
doctor:
	@echo "USED THIS INSTEAD NOW: https://docs.particle.io/tools/doctor/"

# update device to latest version (sometimes needed manually)
update: dfu
	@echo "\nINFO: updating device OS to LTE..."
	@particle update --target $(VERSION)

# flash tinker
# commands: digitalWrite "D7,HIGH", analogWrite, digitalRead, analogRead "A0"
tinker: dfu
	@echo "\nINFO: flashing tinker..."
	@particle flash --usb tinker

# set up particle 2
# particle setup only works for P1!
# alternatively go to https://setup.particle.io/
p2-setup: 
	@echo "INFO: don't do this over serial, go to https://setup.particle.io/ instead now!"
#	@particle serial list
#	@particle serial mac
#	@particle identify
#	@particle serial wifi
	

### COMPILE & FLASH ###

# compile binary
%: 
	@if [ -d "src/$@" ]; then \
		echo "\nINFO: compiling $@ in the cloud for $(PLATFORM) $(VERSION)...."; \
		cd src && particle compile $(PLATFORM) $(MODULES) $@ $@/project.properties --target $(VERSION) --saveTo ../$(subst /,_,$@)-$(PLATFORM)-$(VERSION).bin; \
	else \
		echo "\nERROR: cannot compile program '$*', folder src/$@ does not exist\n"; \
	fi

# flash (via cloud if device is set, via usb if none provided)
# by the default the latest bin, unless BIN otherwise specified
flash:
ifeq ($(device),)
ifeq ($(BIN),)
# flash latest bin by usb
	@$(MAKE) usb_flash BIN=$(shell ls -Art *.bin | tail -n 1)
else
# flash specified bin by usb
	@$(MAKE) usb_flash BIN=$(BIN)
endif
else
ifeq ($(BIN),)
# flash latest bin via cloud
	@$(MAKE) cloud_flash BIN=$(shell ls -Art *.bin | tail -n 1)
else
# flash specified bin via cloud
	@$(MAKE) cloud_flash BIN=$(BIN)
endif
endif

# flash via the cloud
cloud_flash: 
ifeq ($(device),)
	@echo "ERROR: no device provided, specify the name(s) to flash via make ... device=???."
else
	@for dev in $(device);  \
		do echo "\nINFO: flashing $(BIN) to '$${dev}' via the cloud..."; \
		particle flash $${dev} $(BIN); \
	done;
endif

# usb flash
usb_flash: 
	@echo "INFO: putting device into DFU mode"
	@particle usb dfu
	@echo "INFO: flashing $(BIN) over USB (requires device in DFU mode = yellow blinking)..."
	@particle flash --usb  $(BIN)

# cleaning
clean:
	@echo "INFO: removing all .bin files..."
	@rm -f ./*.bin
