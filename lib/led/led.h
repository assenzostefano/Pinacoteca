/**
 * @file led.h
 * @brief GPIO output helpers for LEDs and digital actuators.
 *
 * Provides the GpioOutput class for managing digital/PWM pins,
 * plus legacy-compatible free functions (led, ledDimming, ledRGB).
 */

#ifndef LED_H
#define LED_H

#include <Arduino.h>
#include "../system/error_registry.h"

/**
 * @brief Manages a single digital/PWM output pin.
 */
class GpioOutput {
  private:
    uint8_t _pin;

  public:
    explicit GpioOutput(uint8_t pin) : _pin(pin) {}

    void begin() {
      pinMode(_pin, OUTPUT);
    }

    /** @brief Write HIGH or LOW to the pin. */
    bool write(bool state) {
      digitalWrite(_pin, state ? HIGH : LOW);
      pinacotecaClearError(PIN_ERR_GPIO_IO);
      return true;
    }

    /** @brief Write a PWM value (0–255). */
    bool writePwm(uint8_t value) {
      analogWrite(_pin, value);
      pinacotecaClearError(PIN_ERR_GPIO_IO);
      return true;
    }

    /**
     * @brief Set an RGB LED colour by name.
     * @param colorName  One of: "BLUE","RED","GREEN","WHITE","YELLOW","OFF".
     *
     * Uses direct C-string comparison to avoid heap-allocating a String.
     */
    bool writeRgb(const char* colorName) {
      if (colorName == nullptr) {
        pinacotecaSetError(PIN_ERR_GPIO_IO);
        return false;
      }

      // Skip leading whitespace
      while (*colorName == ' ' || *colorName == '\t') ++colorName;

      // Compare case-insensitively using a compact helper
      auto ciEq = [](const char* a, const char* b) -> bool {
        while (*a && *b) {
          char ca = (*a >= 'a' && *a <= 'z') ? (*a - 32) : *a;
          char cb = (*b >= 'a' && *b <= 'z') ? (*b - 32) : *b;
          if (ca != cb) return false;
          ++a; ++b;
        }
        // Skip trailing whitespace on 'a'
        while (*a == ' ' || *a == '\t') ++a;
        return (*a == '\0' && *b == '\0');
      };

      int color = -1;
      if      (ciEq(colorName, "BLUE"))   color = 255;
      else if (ciEq(colorName, "RED"))    color = 255 << 16;
      else if (ciEq(colorName, "GREEN"))  color = 255 << 8;
      else if (ciEq(colorName, "WHITE"))  color = (255 << 16) | (255 << 8) | 255;
      else if (ciEq(colorName, "YELLOW")) color = (255 << 16) | (255 << 8);
      else if (ciEq(colorName, "OFF"))    color = 0;

      if (color < 0) {
        pinacotecaSetError(PIN_ERR_GPIO_IO);
        return false;
      }

      analogWrite(_pin, color);
      pinacotecaClearError(PIN_ERR_GPIO_IO);
      return true;
    }

    uint8_t pin() const { return _pin; }
};

// ── Legacy compatible free functions ────────────────────────────

/** @brief Turn a digital LED on/off (legacy wrapper). */
inline bool led(uint8_t pin, bool state) {
  digitalWrite(pin, state);
  pinacotecaClearError(PIN_ERR_GPIO_IO);
  return true;
}

/** @brief Set a PWM value on an LED (legacy wrapper). */
inline bool ledDimming(uint8_t pin, int pwmValue) {
  pwmValue = constrain(pwmValue, 0, 255);
  analogWrite(pin, pwmValue);
  pinacotecaClearError(PIN_ERR_GPIO_IO);
  return true;
}

/** @brief Set a colour on an RGB LED (legacy wrapper). */
inline bool ledRGB(uint8_t pin, const char* colorName) {
  GpioOutput out(pin);
  return out.writeRgb(colorName);
}

#endif // LED_H