/**
 * @file turnstile.h
 * @brief Gallery entry turnstile with ultrasonic people counting.
 *
 * Uses two HC-SR04 sensors (IN/OUT) to count people.
 * Supports both standard dual-pin wiring and shared single-pin wiring.
 * Drives a servo gate that opens for a configurable time window.
 */

#ifndef TURNSTILE_H
#define TURNSTILE_H

#include <Arduino.h>
#include "servomotor.h"

class Turnstile {
  private:
    uint8_t _inTrigPin;
    uint8_t _inEchoPin;
    uint8_t _outTrigPin;
    uint8_t _outEchoPin;
    uint8_t _maxPeople;
    uint8_t _currentPeople;
    Servo* _servo;
    bool _isGateOpen;
    bool _lastInDetected;
    bool _lastOutDetected;
    unsigned long _closeAtMillis;
    uint16_t _openTimeMs;
    float _triggerDistanceCm;
    unsigned long _lastUpdateMillis;
    bool _readInNext;

    /// Ultrasonic pulse timeout in microseconds (~1.3 m range).
    /// This is well above the trigger threshold and keeps the loop responsive.
    static constexpr unsigned long ULTRASONIC_TIMEOUT = 8000UL;

    /// Move the servo by a relative angle
    bool moveServo(int angle) {
      if (!antiSufferingServo(angle, *_servo)) {
        pinacotecaSetError(PIN_ERR_SERVO);
        return false;
      }
      pinacotecaClearError(PIN_ERR_SERVO);
      return true;
    }

    /// Check if a person was detected at the given distance
    bool isPersonDetected(float distanceCm) const {
      return distanceCm > 0.0f && distanceCm <= _triggerDistanceCm;
    }

    /**
     * @brief Measure distance with an HC-SR04 sensor.
     * @param trigPin Trigger pin.
     * @param echoPin Echo pin.
     * @return Distance in cm, or -1.0 on timeout.
     */
    float readDistanceCm(uint8_t trigPin, uint8_t echoPin) {
      if (trigPin == echoPin) {
        pinMode(trigPin, OUTPUT);
      }
      digitalWrite(trigPin, LOW);
      delayMicroseconds(2);
      digitalWrite(trigPin, HIGH);
      delayMicroseconds(10);
      digitalWrite(trigPin, LOW);

      if (trigPin == echoPin) {
        pinMode(echoPin, INPUT);
      }

      unsigned long duration = pulseIn(echoPin, HIGH, ULTRASONIC_TIMEOUT);

      if (duration == 0) return -1.0f;
      return (duration * 0.0343f) / 2.0f;
    }

  public:
    Turnstile(uint8_t inTrigPin, uint8_t inEchoPin,
              uint8_t outTrigPin, uint8_t outEchoPin,
              uint8_t maxPeople, float triggerDistanceCm = 25.0f)
      : _inTrigPin(inTrigPin),
        _inEchoPin(inEchoPin),
        _outTrigPin(outTrigPin),
        _outEchoPin(outEchoPin),
        _maxPeople(maxPeople),
        _currentPeople(0),
        _servo(nullptr),
        _isGateOpen(false),
        _lastInDetected(false),
        _lastOutDetected(false),
        _closeAtMillis(0),
        _openTimeMs(700),
        _triggerDistanceCm(triggerDistanceCm > 0.0f ? triggerDistanceCm : 25.0f),
        _lastUpdateMillis(0),
        _readInNext(true) {}

    void begin(Servo* servo) {
      _servo = servo;
      if (_inTrigPin != _inEchoPin) {
        pinMode(_inTrigPin, OUTPUT);
        digitalWrite(_inTrigPin, LOW);
      }
      pinMode(_inEchoPin, INPUT);

      if (_outTrigPin != _outEchoPin) {
        pinMode(_outTrigPin, OUTPUT);
        digitalWrite(_outTrigPin, LOW);
      }
      pinMode(_outEchoPin, INPUT);

      if (_servo == nullptr) {
        pinacotecaSetError(PIN_ERR_TURNSTILE);
      } else {
        pinacotecaClearError(PIN_ERR_TURNSTILE);
      }
    }

    /** @brief Run one sensing + gate cycle (non-blocking). */
    void update() {
      if (!_servo) {
        pinacotecaSetError(PIN_ERR_TURNSTILE);
        return;
      }

      unsigned long now = millis();

      // Close the gate after the timeout
      if (_isGateOpen && now >= _closeAtMillis) {
        if (!moveServo(-90)) {
          pinacotecaSetError(PIN_ERR_TURNSTILE);
          return;
        }
        _isGateOpen = false;
      }

      if (_isGateOpen) return;

      if (now - _lastUpdateMillis < 60) return;
      _lastUpdateMillis = now;

      if (_readInNext) {
        float inDist = readDistanceCm(_inTrigPin, _inEchoPin);
        bool inDetected = isPersonDetected(inDist);
        bool inTriggered = inDetected && !_lastInDetected;
        _lastInDetected = inDetected;

        // Person entering
        if (inTriggered && _currentPeople < _maxPeople) {
          if (!moveServo(90)) {
            pinacotecaSetError(PIN_ERR_TURNSTILE);
            return;
          }
          _currentPeople++;
          _isGateOpen = true;
          _closeAtMillis = millis() + _openTimeMs;
        }
        _readInNext = false;
      } else {
        float outDist = readDistanceCm(_outTrigPin, _outEchoPin);
        bool outDetected = isPersonDetected(outDist);
        bool outTriggered = outDetected && !_lastOutDetected;
        _lastOutDetected = outDetected;

        // Person leaving
        if (outTriggered && _currentPeople > 0) {
          if (!moveServo(90)) {
            pinacotecaSetError(PIN_ERR_TURNSTILE);
            return;
          }
          _currentPeople--;
          _isGateOpen = true;
          _closeAtMillis = millis() + _openTimeMs;
        }
        _readInNext = true;
      }

      pinacotecaClearError(PIN_ERR_TURNSTILE);
    }

    bool getInDetected() const { return _lastInDetected; }
    bool getOutDetected() const { return _lastOutDetected; }

    uint8_t getPeopleCount() const { return _currentPeople; }
    uint8_t getMaxPeople() const { return _maxPeople; }

    void setMaxPeople(uint8_t maxLimit) {
      _maxPeople = maxLimit;
      if (_currentPeople > _maxPeople) {
        _currentPeople = _maxPeople;
      }
    }
};

#endif // TURNSTILE_H