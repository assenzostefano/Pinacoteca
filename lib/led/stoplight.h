// stoplight.h
// Entry stoplight (green/red) for the gallery turnstile.
// Green when there is room, red when full.

#ifndef STOPLIGHT_H
#define STOPLIGHT_H

#include <Arduino.h>
#include "led.h"

class Stoplight {
  private:
    uint8_t _greenPin;
    uint8_t _redPin;

  public:
    Stoplight(int greenPin, int redPin)
      : _greenPin(static_cast<uint8_t>(greenPin)),
        _redPin(static_cast<uint8_t>(redPin)) {}

    void begin() {
      pinMode(_greenPin, OUTPUT);
      pinMode(_redPin, OUTPUT);
    }

    // Update the stoplight based on current people count
    void update(int currentPeople, int maxPeople) {
      bool belowMax = (currentPeople < maxPeople);

      if (!led(_greenPin, belowMax ? HIGH : LOW) ||
          !led(_redPin, belowMax ? LOW : HIGH)) {
        pinacotecaSetError(PIN_ERR_STOPLIGHT_ACTUATOR);
        return;
      }

      pinacotecaClearError(PIN_ERR_STOPLIGHT_ACTUATOR);
    }
};

#endif // STOPLIGHT_H
