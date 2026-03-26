// temperature.h
// NTC analog temperature sensor driver.
// Reads an analog NTC thermistor and converts the value to Celsius.

#ifndef TEMPERATURE_H
#define TEMPERATURE_H

#include <Arduino.h>
#include "../system/error_registry.h"

// Class that manages an NTC temperature sensor
class TemperatureSensor {
  private:
    uint8_t _pin; // Analog sensor pin

  public:
    // Constructor: pass the analog pin of the sensor
    TemperatureSensor(uint8_t pin) : _pin(pin) {}

    // Initialize the pin as INPUT
    void begin() {
      pinMode(_pin, INPUT);
    }

    // Read the temperature in Celsius
    // Returns SENSOR_ERROR_VALUE (-999.0) if the sensor fails
    float readCelsius() const {
      int raw = analogRead(_pin);

      if (raw <= 0 || raw >= 1023) {
        pinacotecaSetError(PIN_ERR_TEMP_SENSOR);
        return SENSOR_ERROR_VALUE;
      }

      pinacotecaClearError(PIN_ERR_TEMP_SENSOR);

      const float BETA = 3950.0;
      float kelvin = 1.0 / (log(1.0 / (1023.0 / raw - 1.0)) / BETA + 1.0 / 298.15);
      return kelvin - 273.15;
    }
};

// Legacy wrapper — prefer TemperatureSensor class
inline float readTemperatureCelsius(int sensorPin) {
  TemperatureSensor sensor(static_cast<uint8_t>(sensorPin));
  return sensor.readCelsius();
}

#endif // TEMPERATURE_H