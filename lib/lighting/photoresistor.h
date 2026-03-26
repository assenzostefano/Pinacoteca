// photoresistor.h
// LDR (photoresistor) light sensor driver.
// Reads the analog value and converts it to lux.

#ifndef PHOTORESISTOR_H
#define PHOTORESISTOR_H

#include <Arduino.h>
#include "../system/error_registry.h"

// Class that manages an LDR photoresistor
class LightSensor {
  private:
    uint8_t _pin; // Analog sensor pin

  public:
    // Constructor: pass the analog pin
    LightSensor(uint8_t pin) : _pin(pin) {}

    // Initialize the pin as INPUT
    void begin() {
      pinMode(_pin, INPUT);
    }

    // Read the light level in lux
    // Returns SENSOR_ERROR_VALUE if the sensor fails
    float readLux() const {
      int raw = analogRead(_pin);

      if (raw <= 0 || raw >= 1023) {
        pinacotecaSetError(PIN_ERR_LIGHT_SENSOR);
        return SENSOR_ERROR_VALUE;
      }

      pinacotecaClearError(PIN_ERR_LIGHT_SENSOR);

      // LDR to lux conversion constants
      const float GAMMA = 0.7;
      const float RL10 = 50;

      float voltage = raw / 1024.0 * 5.0;
      float resistance = 2000.0 * voltage / (1.0 - voltage / 5.0);
      float lux = pow(RL10 * 1e3 * pow(10, GAMMA) / resistance, (1.0 / GAMMA));

      return lux;
    }

    // Return the configured pin number
    uint8_t pin() const { return _pin; }
};

// Legacy wrapper — prefer LightSensor class
inline float readLux(int lightSensorPin) {
  LightSensor sensor(static_cast<uint8_t>(lightSensorPin));
  return sensor.readLux();
}

#endif // PHOTORESISTOR_H