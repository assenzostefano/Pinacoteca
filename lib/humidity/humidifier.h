// humidifier.h
// Humidifier controller — humidity regulation.
// Reads humidity from a DHT sensor and activates/deactivates the humidifier.

#ifndef HUMIDIFIER_H
#define HUMIDIFIER_H

#include <Arduino.h>
#include "humidity.h"
#include "../led/led.h"

class HumidifierControl {
  private:
    uint8_t _pin;           // Humidifier actuator pin
    float _targetHumidity;  // Target humidity in %

  public:
    HumidifierControl(int humidifierPin, float targetHumidity)
      : _pin(static_cast<uint8_t>(humidifierPin)),
        _targetHumidity(targetHumidity) {}

    void begin() {
      beginHumiditySensor();
      pinMode(_pin, OUTPUT);
    }

    // Run one control cycle
    bool update() {
      float current = readHumidity();

      // Sensor error: turn off the actuator
      if (current == SENSOR_ERROR_VALUE) {
        pinacotecaSetError(PIN_ERR_HUM_SENSOR);
        if (!led(_pin, LOW)) {
          pinacotecaSetError(PIN_ERR_HUMIDIFIER_ACTUATOR);
        }
        return false;
      }

      pinacotecaClearError(PIN_ERR_HUM_SENSOR);

      // Activate only if humidity is too high (above target + tolerance)
      const float TOLERANCE = 2.0;
      bool activate = current > (_targetHumidity + TOLERANCE);

      if (!led(_pin, activate ? HIGH : LOW)) {
        pinacotecaSetError(PIN_ERR_HUMIDIFIER_ACTUATOR);
        return false;
      }

      pinacotecaClearError(PIN_ERR_HUMIDIFIER_ACTUATOR);
      return true;
    }

    void setTargetHumidity(float h)    { _targetHumidity = h; }
    float getTargetHumidity() const    { return _targetHumidity; }
    float getCurrentHumidity() const   { return readHumidity(); }
};

#endif // HUMIDIFIER_H