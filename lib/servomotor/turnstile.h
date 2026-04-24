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
    bool _sensorDebugEnabled;
    unsigned long _closeAtMillis;
    unsigned long _lastDebugPrintMillis;
    uint16_t _openTimeMs;
    uint16_t _debugPrintIntervalMs;
    float _triggerDistanceCm;

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

    void debugSensors(float inDist,
                      float outDist,
                      bool inDetected,
                      bool outDetected,
                      unsigned long now) {
      if (!_sensorDebugEnabled) {
        return;
      }

      if (_debugPrintIntervalMs > 0 &&
          _lastDebugPrintMillis != 0 &&
          (now - _lastDebugPrintMillis) < _debugPrintIntervalMs) {
        return;
      }

      _lastDebugPrintMillis = now;

      Serial.print(F("TURNSTILE:SENSORS IN="));
      if (inDist < 0.0f) {
        Serial.print(F("timeout"));
      } else {
        Serial.print(inDist);
        Serial.print(F("cm"));
      }

      Serial.print(F(" OUT="));
      if (outDist < 0.0f) {
        Serial.print(F("timeout"));
      } else {
        Serial.print(outDist);
        Serial.print(F("cm"));
      }

      Serial.print(F(" IN_DET="));
      Serial.print(inDetected ? 1 : 0);
      Serial.print(F(" OUT_DET="));
      Serial.print(outDetected ? 1 : 0);
      Serial.print(F(" PEOPLE="));
      Serial.println(static_cast<int>(_currentPeople));
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
        _sensorDebugEnabled(false),
        _closeAtMillis(0),
        _openTimeMs(700),
        _lastDebugPrintMillis(0),
        _debugPrintIntervalMs(500),
        _triggerDistanceCm(triggerDistanceCm > 0.0f ? triggerDistanceCm : 25.0f) {}

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

      // Read the sensors
      float inDist = readDistanceCm(_inTrigPin, _inEchoPin);
      float outDist = readDistanceCm(_outTrigPin, _outEchoPin);

      bool inDetected = isPersonDetected(inDist);
      bool outDetected = isPersonDetected(outDist);

      debugSensors(inDist, outDist, inDetected, outDetected, now);

      bool inTriggered = inDetected && !_lastInDetected;
      bool outTriggered = outDetected && !_lastOutDetected;

      _lastInDetected = inDetected;
      _lastOutDetected = outDetected;

      if (_isGateOpen) return;

      // Person entering
      if (inTriggered && _currentPeople < _maxPeople) {
        if (!moveServo(90)) {
          pinacotecaSetError(PIN_ERR_TURNSTILE);
          return;
        }
        _currentPeople++;
        _isGateOpen = true;
        _closeAtMillis = now + _openTimeMs;
      }

      // Person leaving
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

    void setSensorDebug(bool enabled, uint16_t intervalMs = 500) {
      _sensorDebugEnabled = enabled;
      _debugPrintIntervalMs = intervalMs;
      _lastDebugPrintMillis = 0;
    }

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