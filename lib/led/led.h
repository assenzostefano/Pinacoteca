// led.h
// GPIO output helpers for LEDs and digital actuators.
// GpioOutput class + legacy compatible functions (led, ledDimming, ledRGB).

#ifndef LED_H
#define LED_H

#include <Arduino.h>
#include "../system/error_registry.h"

// Class that manages a digital/PWM output pin
class GpioOutput {
  private:
    uint8_t _pin; // Pin number

  public:
    // Constructor: pass the pin number
    GpioOutput(uint8_t pin) : _pin(pin) {}

    // Initialize the pin as OUTPUT
    void begin() {
      pinMode(_pin, OUTPUT);
    }

    // Write HIGH or LOW
    bool write(bool state) {
      digitalWrite(_pin, state ? HIGH : LOW);
      pinacotecaClearError(PIN_ERR_GPIO_IO);
      return true;
    }

    // Write a PWM value (0-255)
    bool writePwm(int value) {
      value = constrain(value, 0, 255);
      analogWrite(_pin, value);
      pinacotecaClearError(PIN_ERR_GPIO_IO);
      return true;
    }

    // Write an RGB color by name ("BLUE", "RED", "GREEN", etc.)
    bool writeRgb(const char* colorName) {
      String name(colorName);
      name.trim();
      name.toUpperCase();

      int color = 0;
      if      (name == "BLUE")   color = 255;
      else if (name == "RED")    color = 255 << 16;
      else if (name == "GREEN")  color = 255 << 8;
      else if (name == "WHITE")  color = (255 << 16) | (255 << 8) | 255;
      else if (name == "YELLOW") color = (255 << 16) | (255 << 8);
      else if (name == "OFF")    color = 0;
      else {
        pinacotecaSetError(PIN_ERR_GPIO_IO);
        return false;
      }

      analogWrite(_pin, color);
      pinacotecaClearError(PIN_ERR_GPIO_IO);
      return true;
    }

    // Return the pin number
    uint8_t pin() const { return _pin; }
};

// --- Legacy compatible functions ---

// Turn a digital LED on/off
inline bool led(int pin, bool state) {
  if (pin < 0) {
    pinacotecaSetError(PIN_ERR_GPIO_IO);
    return false;
  }
  digitalWrite(pin, state);
  pinacotecaClearError(PIN_ERR_GPIO_IO);
  return true;
}

// Set a PWM value on an LED (dimming)
inline bool ledDimming(int pin, int pwmValue) {
  if (pin < 0) {
    pinacotecaSetError(PIN_ERR_GPIO_IO);
    return false;
  }
  pwmValue = constrain(pwmValue, 0, 255);
  analogWrite(pin, pwmValue);
  pinacotecaClearError(PIN_ERR_GPIO_IO);
  return true;
}

// Set a color on an RGB LED
inline bool ledRGB(int pin, const char* colorName) {
  if (pin < 0) {
    pinacotecaSetError(PIN_ERR_GPIO_IO);
    return false;
  }
  GpioOutput out(static_cast<uint8_t>(pin));
  return out.writeRgb(colorName);
}

#endif // LED_H