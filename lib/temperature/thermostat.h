// thermostat.h
// Climate controller — heating and cooling regulation.
// Monitors temperature and activates heating or cooling
// to maintain a target temperature, with a pause between state changes.

#ifndef THERMOSTAT_H
#define THERMOSTAT_H

#include <Arduino.h>
#include "temperature.h"
#include "../led/led.h"

class Thermostat {
  private:
    uint8_t _sensorPin;
    uint8_t _heatingPin;
    uint8_t _coolingPin;
    float _targetTemp;
    uint8_t _state; // 0 = OFF, 1 = Heating, 2 = Cooling
    unsigned long _previousMillis;
    unsigned long _pauseTime;

    // Apply the current state to the actuators
    bool applyState() {
      if (_state == 1) { // Heating
        return led(_heatingPin, HIGH) && led(_coolingPin, LOW);
      } else if (_state == 2) { // Cooling
        return led(_heatingPin, LOW) && ledRGB(_coolingPin, "BLUE");
      } else { // OFF
        return led(_heatingPin, LOW) && led(_coolingPin, LOW);
      }
    }

  public:
    Thermostat(int sensorPin, float targetTemp, int heatingPin, int coolingPin,
               unsigned long pauseTime = 5000)
      : _sensorPin(static_cast<uint8_t>(sensorPin)),
        _heatingPin(static_cast<uint8_t>(heatingPin)),
        _coolingPin(static_cast<uint8_t>(coolingPin)),
        _targetTemp(targetTemp),
        _state(0),
        _previousMillis(0),
        _pauseTime(pauseTime) {}

    void begin() {
      pinMode(_sensorPin, INPUT);
      pinMode(_heatingPin, OUTPUT);
      pinMode(_coolingPin, OUTPUT);
    }

    // Run one control cycle
    bool update() {
      float temp = readTemperatureCelsius(_sensorPin);
      unsigned long now = millis();

      // Sensor error: turn everything off
      if (temp == SENSOR_ERROR_VALUE) {
        pinacotecaSetError(PIN_ERR_TEMP_SENSOR);
        if (!led(_heatingPin, LOW) || !led(_coolingPin, LOW)) {
          pinacotecaSetError(PIN_ERR_THERMOSTAT_ACTUATOR);
        }
        return false;
      }

      pinacotecaClearError(PIN_ERR_TEMP_SENSOR);

      const float TOLERANCE = 1.0;
      int desired = 0; // 0 = OFF

      if (temp < (_targetTemp - TOLERANCE)) {
        desired = 1; // Heating
      } else if (temp > (_targetTemp + TOLERANCE)) {
        desired = 2; // Cooling
      }

      // Same state or turning off: apply directly
      if (desired == _state || desired == 0) {
        _state = desired;

        if (!applyState()) {
          pinacotecaSetError(PIN_ERR_THERMOSTAT_ACTUATOR);
          return false;
        }

        pinacotecaClearError(PIN_ERR_THERMOSTAT_ACTUATOR);
        _previousMillis = now;
        return true;
      }

      // Pause before switching state (avoids rapid oscillation)
      if (!led(_heatingPin, LOW) || !led(_coolingPin, LOW)) {
        pinacotecaSetError(PIN_ERR_THERMOSTAT_ACTUATOR);
        return false;
      }

      if (now - _previousMillis >= _pauseTime) {
        _state = desired;
        Serial.println("Climate mode changed successfully.");
      }

      pinacotecaClearError(PIN_ERR_THERMOSTAT_ACTUATOR);
      return true;
    }

    void setTargetTemperature(float t)   { _targetTemp = t; }
    float getTargetTemperature() const   { return _targetTemp; }
    float getCurrentTemperature() const  { return readTemperatureCelsius(_sensorPin); }
};

#endif // THERMOSTAT_H
