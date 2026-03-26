// lighting_control.h
// Gallery lighting controller with PWM dimming.
// Reads ambient light from a photoresistor and adjusts
// supplemental lighting to reach a target lux level.

#ifndef LIGHTING_CONTROL_H
#define LIGHTING_CONTROL_H

#include <Arduino.h>
#include "photoresistor.h"
#include "../led/led.h"

class LightingControl {
  private:
    uint8_t _sensorPin;  // Photoresistor pin
    uint8_t _dimmerPin;  // PWM ceiling lights pin
    uint16_t _targetLux; // Desired lux level

    // Calculate the PWM value from the light deficit
    int computePwm(float missingLight) const {
      if (missingLight <= 0.0) return 0;

#if defined(ARDUINO_ARCH_AVR)
      return 255; // On AVR there is no proportional dimming
#else
      float ratio = missingLight / _targetLux;
      if (ratio > 1.0) ratio = 1.0;
      if (ratio < 0.0) ratio = 0.0;
      return (int)(ratio * 255.0);
#endif
    }

  public:
    LightingControl(int sensorPin, int dimmerPin, int targetLux)
      : _sensorPin(static_cast<uint8_t>(sensorPin)),
        _dimmerPin(static_cast<uint8_t>(dimmerPin)),
        _targetLux(static_cast<uint16_t>(constrain(targetLux, 0, 65535))) {}

    void begin() {
      pinMode(_sensorPin, INPUT);
      pinMode(_dimmerPin, OUTPUT);
    }

    // Run one control cycle
    bool update() {
      float lux = readLux(_sensorPin);

      // Sensor error: turn off the lights
      if (lux <= SENSOR_ERROR_VALUE + 1.0) {
        pinacotecaSetError(PIN_ERR_LIGHT_SENSOR);
        if (!ledDimming(_dimmerPin, 0)) {
          pinacotecaSetError(PIN_ERR_LIGHT_ACTUATOR);
        }
        return false;
      }

      pinacotecaClearError(PIN_ERR_LIGHT_SENSOR);

      float missing = _targetLux - lux;
      int pwm = computePwm(missing);

      if (!ledDimming(_dimmerPin, pwm)) {
        pinacotecaSetError(PIN_ERR_LIGHT_ACTUATOR);
        return false;
      }

      pinacotecaClearError(PIN_ERR_LIGHT_ACTUATOR);
      return true;
    }

    void setTargetLux(int lux) {
      _targetLux = static_cast<uint16_t>(constrain(lux, 0, 65535));
    }

    int getTargetLux() const     { return _targetLux; }
    float getCurrentLux() const  { return readLux(_sensorPin); }
};

#endif // LIGHTING_CONTROL_H