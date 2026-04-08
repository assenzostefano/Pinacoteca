/**
 * @file turnstile.h
 * @brief Gallery entry turnstile with ultrasonic people counting.
 *
 * Uses two HC-SR04 sensors (IN/OUT) on shared TRIG/ECHO pins
 * to count people entering and leaving the gallery.
 * Drives a servo gate that opens for a configurable time window.
 */

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
    uint16_t _openTimeMs;
    float _triggerDistanceCm;

    /// Ultrasonic pulse timeout in microseconds (~4.3 m range)
    static constexpr unsigned long ULTRASONIC_TIMEOUT = 25000UL;

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
     * @brief Measure distance with a single-pin HC-SR04 sensor.
     * @param sensorPin  Shared TRIG/ECHO pin.
     * @return Distance in cm, or -1.0 on timeout.
     */
    float readDistanceCm(uint8_t sensorPin) {
      pinMode(sensorPin, OUTPUT);
      digitalWrite(sensorPin, LOW);
      delayMicroseconds(2);
      digitalWrite(sensorPin, HIGH);
      delayMicroseconds(10);
      digitalWrite(sensorPin, LOW);

      pinMode(sensorPin, INPUT);
      unsigned long duration = pulseIn(sensorPin, HIGH, ULTRASONIC_TIMEOUT);

      if (duration == 0) return -1.0f;
      return (duration * 0.0343f) / 2.0f;
    }

  public:
    Turnstile(uint8_t inSensorPin, uint8_t outSensorPin,
              uint8_t maxPeople, float triggerDistanceCm = 25.0f)
      : _inSensorPin(inSensorPin),
        _outSensorPin(outSensorPin),
        _maxPeople(maxPeople),
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
      float inDist = readDistanceCm(_inSensorPin);
      float outDist = readDistanceCm(_outSensorPin);

      bool inDetected = isPersonDetected(inDist);
      bool outDetected = isPersonDetected(outDist);

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

    uint8_t getPeopleCount() const { return _currentPeople; }
    uint8_t getMaxPeople() const { return _maxPeople; }

    void setPeopleCount(uint8_t people) {
      _currentPeople = (people > _maxPeople) ? _maxPeople : people;
    }
};

#endif // TURNSTILE_H