#ifndef TURNSTILE_H
#define TURNSTILE_H

#include <Arduino.h>
#include "servomotor.h"

class Turnstile {
    private:
        uint8_t _inButton;
        uint8_t _outButton;
        uint8_t _maxPeople;
        uint8_t _currentPeople;
        Servo* _servo;
        bool _isGateOpen;
        bool _lastInState;
        bool _lastOutState;
        unsigned long _closeAtMillis;
        unsigned int _openTimeMs;

    public:
        Turnstile(int inButton, int outButton, int maxPeople)
            : _inButton(static_cast<uint8_t>(inButton)),
              _outButton(static_cast<uint8_t>(outButton)),
              _maxPeople(static_cast<uint8_t>(constrain(maxPeople, 0, 255))),
              _currentPeople(0),
              _servo(nullptr),
              _isGateOpen(false),
              _lastInState(false),
              _lastOutState(false),
              _closeAtMillis(0),
              _openTimeMs(700) {}

        void begin(Servo* servo) {
            _servo = servo;
            pinMode(_inButton, INPUT);
            pinMode(_outButton, INPUT);

            if (_servo == nullptr) {
                pinacotecaSetError(PIN_ERR_TURNSTILE);
            } else {
                pinacotecaClearError(PIN_ERR_TURNSTILE);
            }
        }

        void update() {
            if (!_servo) {
                pinacotecaSetError(PIN_ERR_TURNSTILE);
                return;
            }

            unsigned long now = millis();
            if (_isGateOpen && now >= _closeAtMillis) {
                if (!moveServo(-90)) {
                    pinacotecaSetError(PIN_ERR_TURNSTILE);
                    return;
                }
                _isGateOpen = false;
            }

            bool inState = digitalRead(_inButton);
            bool outState = digitalRead(_outButton);

            bool inPressed = inState && !_lastInState;
            bool outPressed = outState && !_lastOutState;

            _lastInState = inState;
            _lastOutState = outState;

            if (_isGateOpen) {
                return;
            }

            if (inPressed && _currentPeople < _maxPeople) {
                if (!moveServo(90)) {
                    pinacotecaSetError(PIN_ERR_TURNSTILE);
                    return;
                }
                _currentPeople++;
                _isGateOpen = true;
                _closeAtMillis = now + _openTimeMs;
            }

            if (outPressed && _currentPeople > 0) {
                if (!moveServo(90)) {
                    pinacotecaSetError(PIN_ERR_TURNSTILE);
                    return;
                }
                _currentPeople--;
                _isGateOpen = true;
                _closeAtMillis = now + _openTimeMs;
            }

            pinacotecaClearError(PIN_ERR_TURNSTILE);
        }

        int getPeopleCount() const { return _currentPeople; }

        int getMaxPeople() const { return _maxPeople; }

        void setPeopleCount(int people) {
            _currentPeople = static_cast<uint8_t>(constrain(people, 0, _maxPeople));
        }

    private:
        bool moveServo(int angle) {
            if (!antiSufferingServo(angle, *_servo)) {
                pinacotecaSetError(PIN_ERR_SERVO);
                return false;
            }

            pinacotecaClearError(PIN_ERR_SERVO);
            return true;
        }
};

#endif