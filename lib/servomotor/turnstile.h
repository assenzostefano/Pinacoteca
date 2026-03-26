#ifndef TURNSTILE_H
#define TURNSTILE_H

#include <Arduino.h>
#include "servomotor.h"

class Turnstile {
    private:
        uint8_t _inSensorPin;
        uint8_t _outSensorPin;
        uint8_t _maxPeople;
        uint8_t _currentPeople;
        Servo* _servo;
        bool _isGateOpen;
        bool _lastInDetected;
        bool _lastOutDetected;
        unsigned long _closeAtMillis;
        unsigned int _openTimeMs;
        float _triggerDistanceCm;

    public:
                Turnstile(int inSensorPin, int outSensorPin, int maxPeople, float triggerDistanceCm = 25.0f)
            : _inSensorPin(static_cast<uint8_t>(inSensorPin)),
              _outSensorPin(static_cast<uint8_t>(outSensorPin)),
              _maxPeople(static_cast<uint8_t>(constrain(maxPeople, 0, 255))),
              _currentPeople(0),
              _servo(nullptr),
              _isGateOpen(false),
              _lastInDetected(false),
              _lastOutDetected(false),
              _closeAtMillis(0),
              _openTimeMs(700),
                            _triggerDistanceCm(triggerDistanceCm > 0.0f ? triggerDistanceCm : 25.0f) {}

        void begin(Servo* servo) {
            _servo = servo;
            pinMode(_inSensorPin, INPUT);
            pinMode(_outSensorPin, INPUT);

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

            float inDistance = readDistanceCm(_inSensorPin);
            float outDistance = readDistanceCm(_outSensorPin);

            bool inDetected = isPersonDetected(inDistance);
            bool outDetected = isPersonDetected(outDistance);

            bool inTriggered = inDetected && !_lastInDetected;
            bool outTriggered = outDetected && !_lastOutDetected;

            _lastInDetected = inDetected;
            _lastOutDetected = outDetected;

            if (_isGateOpen) {
                return;
            }

            if (inTriggered && _currentPeople < _maxPeople) {
                if (!moveServo(90)) {
                    pinacotecaSetError(PIN_ERR_TURNSTILE);
                    return;
                }
                _currentPeople++;
                _isGateOpen = true;
                _closeAtMillis = now + _openTimeMs;
            }

            if (outTriggered && _currentPeople > 0) {
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

        bool isPersonDetected(float distanceCm) const {
            return distanceCm > 0.0f && distanceCm <= _triggerDistanceCm;
        }

        float readDistanceCm(uint8_t sensorPin) {
            pinMode(sensorPin, OUTPUT);
            digitalWrite(sensorPin, LOW);
            delayMicroseconds(2);
            digitalWrite(sensorPin, HIGH);
            delayMicroseconds(10);
            digitalWrite(sensorPin, LOW);

            pinMode(sensorPin, INPUT);
            unsigned long duration = pulseIn(sensorPin, HIGH, 25000);
            if (duration == 0) {
                return -1.0f;
            }

            return (duration * 0.0343f) / 2.0f;
        }
};

#endif