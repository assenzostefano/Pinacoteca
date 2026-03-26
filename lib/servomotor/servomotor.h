// servomotor.h
// Safe servo motor driver with range protection.
// Prevents the servo from exceeding the 0-180 degree limits.

#ifndef SERVOMOTOR_H
#define SERVOMOTOR_H

#include <Arduino.h>
#include <Servo.h>
#include "../system/error_registry.h"

// Class that manages a servo motor with range protection
class SafeServo {
  private:
    Servo* _servo; // Pointer to the servo

  public:
    // Constructor: pass a pointer to the servo
    SafeServo(Servo* servo) : _servo(servo) {}

    // Move the servo by a relative angle (e.g. +90, -90)
    bool move(int delta) {
      if (_servo == nullptr || !_servo->attached()) {
        pinacotecaSetError(PIN_ERR_SERVO);
        return false;
      }

      int currentPos = _servo->read();
      int newAngle = currentPos + delta;

      if (newAngle > 180 || newAngle < 0) {
        Serial.println("Limit movement Servo Motor");
        pinacotecaSetError(PIN_ERR_SERVO);
        return false;
      }

      _servo->write(newAngle);
      pinacotecaClearError(PIN_ERR_SERVO);
      return true;
    }

    // Move the servo to an absolute angle (0-180)
    bool moveTo(int angle) {
      if (_servo == nullptr) return false;
      int delta = angle - _servo->read();
      return move(delta);
    }

    // Read the current position of the servo
    int read() const {
      return (_servo != nullptr) ? _servo->read() : -1;
    }
};

// Legacy wrapper — prefer SafeServo class
inline bool antiSufferingServo(int angle, Servo& s) {
  SafeServo ss(&s);
  return ss.move(angle);
}

#endif // SERVOMOTOR_H