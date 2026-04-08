/**
 * @file thermostat.h
 * @brief Climate controller — heating and cooling regulation.
 *
 * Monitors temperature via an NTC sensor and activates heating
 * or cooling to maintain a target temperature. Implements a
 * configurable pause between state transitions to prevent
 * rapid oscillation of HVAC equipment.
 */

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
    float _lastTemp;             ///< Cached last valid reading
    uint8_t _state;              ///< 0 = OFF, 1 = Heating, 2 = Cooling
    unsigned long _previousMillis;
    unsigned long _pauseTime;

    static constexpr float TOLERANCE = 1.0f;

    /// Apply the current state to the actuators
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
    Thermostat(uint8_t sensorPin, float targetTemp,
               uint8_t heatingPin, uint8_t coolingPin,
               unsigned long pauseTime = 5000)
      : _sensorPin(sensorPin),
        _heatingPin(heatingPin),
        _coolingPin(coolingPin),
        _targetTemp(targetTemp),
        _lastTemp(SENSOR_ERROR_VALUE),
        _state(0),
        _previousMillis(0),
        _pauseTime(pauseTime) {}

    void begin() {
      pinMode(_sensorPin, INPUT);
      pinMode(_heatingPin, OUTPUT);
      pinMode(_coolingPin, OUTPUT);
    }

    /**
     * @brief Run one control cycle.
     * @return true if the cycle completed normally, false on sensor error.
     */
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

      _lastTemp = temp;
      pinacotecaClearError(PIN_ERR_TEMP_SENSOR);

      uint8_t desired = 0; // 0 = OFF

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
        Serial.println(F("Climate mode changed successfully."));
      }

      pinacotecaClearError(PIN_ERR_THERMOSTAT_ACTUATOR);
      return true;
    }

    void setTargetTemperature(float t)   { _targetTemp = t; }
    float getTargetTemperature() const   { return _targetTemp; }

    /**
     * @brief Get the current temperature reading.
     * @note Performs a live ADC read each call — not a cached getter.
     */
    float getCurrentTemperature() const  { return readTemperatureCelsius(_sensorPin); }
};

#endif // THERMOSTAT_H
