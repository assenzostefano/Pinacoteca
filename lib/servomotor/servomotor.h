/**
 * @file servomotor.h
 * @brief Safe servo motor driver with 0–180° range protection.
 *
 * Prevents the servo from exceeding mechanical limits.
 * Provides both a class interface (SafeServo) and a legacy
 * free-function wrapper (antiSufferingServo).
 */

#ifndef SERVOMOTOR_H
#define SERVOMOTOR_H

#include <Arduino.h>
#include <Servo.h>
#include "../system/error_registry.h"

/**
 * @brief Wraps a Servo pointer and enforces 0–180° bounds on every move.
 */
class SafeServo {
  private:
    Servo* _servo;

  public:
    explicit SafeServo(Servo* servo) : _servo(servo) {}

    /**
     * @brief Move the servo by a relative angle (e.g. +90, -90).
     * @return true if the move was within bounds and executed.
     */
    bool move(int delta) {
      if (_servo == nullptr || !_servo->attached()) {
        pinacotecaSetError(PIN_ERR_SERVO);
        return false;
      }

      int currentPos = _servo->read();
      int newAngle = currentPos + delta;

      if (newAngle > 180 || newAngle < 0) {
        Serial.println(F("Limit movement Servo Motor"));
        pinacotecaSetError(PIN_ERR_SERVO);
        return false;
      }

      _servo->write(newAngle);
      pinacotecaClearError(PIN_ERR_SERVO);
      return true;
    }

    /**
     * @brief Move the servo to an absolute angle (0–180).
     * @return true if the target was within bounds and reached.
     */
    bool moveTo(int angle) {
      if (_servo == nullptr) return false;
      int delta = angle - _servo->read();
      return move(delta);
    }

    /** @brief Read the current servo position in degrees. */
    int read() const {
      return (_servo != nullptr) ? _servo->read() : -1;
    }
};

/** @brief Legacy wrapper — prefer SafeServo class. */
inline bool antiSufferingServo(int angle, Servo& s) {
  SafeServo ss(&s);
  return ss.move(angle);
}

#endif // SERVOMOTOR_H