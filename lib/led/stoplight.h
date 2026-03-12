#ifndef STOPLIGHT_H
#define STOPLIGHT_H

#include <Arduino.h>
#include "led.h"

class Stoplight {
    private:
        int _greenPin;
        int _redPin;

    public:
        Stoplight(int greenPin, int redPin) : _greenPin(greenPin), _redPin(redPin) {}

        void begin() {
            pinMode(_greenPin, OUTPUT);
            pinMode(_redPin, OUTPUT);
        }

        void update(int currentPeople, int maxPeople) {
            if (currentPeople < maxPeople) {
                led(_greenPin, HIGH);
                led(_redPin, LOW);
            } else {
                led(_greenPin, LOW);
                led(_redPin, HIGH);
            }
        }
};

#endif
