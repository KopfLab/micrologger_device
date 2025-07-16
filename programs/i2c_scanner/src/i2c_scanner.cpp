#include "Particle.h"
#include "Wire.h"

// enable system treading
#ifndef SYSTEM_VERSION_v620
SYSTEM_THREAD(ENABLED);
#endif

// manual mode, no wifi
SYSTEM_MODE(MANUAL);

// log handler
SerialLogHandler logHandler(LOG_LEVEL_TRACE);

// setup
void setup() {
  Wire.begin();
}
 
// loop
uint counter = 0;
unsigned long timer = 0;
const std::chrono::milliseconds wait = 2s;
const std::chrono::milliseconds timeout = 100ms;

void loop() {
  byte error, address;
  int n_devices = 0;
 
  if (millis() - timer > wait.count()) {
    
    Log.info("I2C scan #%d...", ++counter);
  
    for(address = 1; address < 127; address++ ) {
      timer = millis();
      WITH_LOCK(Wire) {
        // lock for thread safety, not really necessary here but good practice
        Wire.beginTransmission(address);
        error = Wire.endTransmission();
      }
      if (error == 0) {
        Log.info("I2C device #%d at address 0x%02X", ++n_devices, address);
      } else if (error==4) {
        Log.error("Unknown error at address 0x%02X", address);
      } else if (millis() - timer > timeout.count()) {
        Log.error("Timeout at address 0x%02X, there might be an I2C issue.", address);
      }    
    }
    if (n_devices == 0)
      Log.info("No I2C devices found.\n");
    else
      Log.info("Done.\n");
  }
}