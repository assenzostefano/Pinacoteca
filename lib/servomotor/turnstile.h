#ifndef TURNSTILE_H
#define TURNSTILE_H

#include <Arduino.h>
#include "servomotor.h"

class Turnstile {
    private:
        int _inButton;
        int _outButton;
        int _maxPeople;
        int _currentPeople;
        Servo* _servo;

    public:
        Turnstile(int inButton, int outButton, int maxPeople)
            : _inButton(inButton), _outButton(outButton), _maxPeople(maxPeople), _currentPeople(0), _servo(nullptr) {}

        void begin(Servo* servo) {
            _servo = servo;
            pinMode(_inButton, INPUT);
            pinMode(_outButton, INPUT);
        }

        void update() {
            if (!_servo) return; // Ensure servo is initialized

            bool inState = digitalRead(_inButton);
            bool outState = digitalRead(_outButton);

            if (inState == HIGH && _currentPeople < _maxPeople) {
                moveServo(90);
                _currentPeople++;
                delay(1000);
                moveServo(-90);
            }

            if (outState == HIGH && _currentPeople > 0) {
                moveServo(90);
                _currentPeople--;
                delay(1000);
                moveServo(-90);
            }
        }

        int getPeopleCount() const { return _currentPeople; }

    private:
        void moveServo(int angle) {
            antiSufferingServo(angle, *_servo);
        }
};

#endif