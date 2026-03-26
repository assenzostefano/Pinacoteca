// error_registry.h
// Centralised error tracking for the Pinacoteca system.
// Uses a compact bitmask so every subsystem can set, clear
// and query errors without dynamic memory.

#ifndef ERROR_REGISTRY_H
#define ERROR_REGISTRY_H

#include <Arduino.h>

// Value returned when a sensor read fails
const float SENSOR_ERROR_VALUE = -999.0;

// Error codes — one bit per subsystem
enum PinacotecaErrorCode : uint8_t {
  PIN_ERR_TEMP_SENSOR          = 0,
  PIN_ERR_HUM_SENSOR           = 1,
  PIN_ERR_LIGHT_SENSOR         = 2,
  PIN_ERR_SERVO                = 3,
  PIN_ERR_TURNSTILE            = 4,
  PIN_ERR_LIGHT_ACTUATOR       = 5,
  PIN_ERR_THERMOSTAT_ACTUATOR  = 6,
  PIN_ERR_HUMIDIFIER_ACTUATOR  = 7,
  PIN_ERR_STOPLIGHT_ACTUATOR   = 8,
  PIN_ERR_GPIO_IO              = 9
};

// Global error-flag register (32-bit bitmask)
inline uint32_t& pinacotecaErrorFlags() {
  static uint32_t flags = 0;
  return flags;
}

// Set an error flag
inline void pinacotecaSetError(PinacotecaErrorCode code) {
  pinacotecaErrorFlags() |= (1UL << static_cast<uint8_t>(code));
}

// Clear an error flag
inline void pinacotecaClearError(PinacotecaErrorCode code) {
  pinacotecaErrorFlags() &= ~(1UL << static_cast<uint8_t>(code));
}

// Check whether a specific error is active
inline bool pinacotecaHasError(PinacotecaErrorCode code) {
  return (pinacotecaErrorFlags() & (1UL << static_cast<uint8_t>(code))) != 0;
}

// Check whether any error is active
inline bool pinacotecaHasAnyError() {
  return pinacotecaErrorFlags() != 0;
}

// Clear every error flag at once
inline void pinacotecaClearAllErrors() {
  pinacotecaErrorFlags() = 0;
}

// Return human-readable text for the first active error
inline const char* pinacotecaFirstErrorText() {
  if (pinacotecaHasError(PIN_ERR_TEMP_SENSOR))         return "Temp sensor FAIL";
  if (pinacotecaHasError(PIN_ERR_HUM_SENSOR))          return "Hum sensor FAIL";
  if (pinacotecaHasError(PIN_ERR_LIGHT_SENSOR))        return "Light sensor FAIL";
  if (pinacotecaHasError(PIN_ERR_SERVO))               return "Servo FAIL";
  if (pinacotecaHasError(PIN_ERR_TURNSTILE))           return "Turnstile FAIL";
  if (pinacotecaHasError(PIN_ERR_LIGHT_ACTUATOR))      return "Lights actuator";
  if (pinacotecaHasError(PIN_ERR_THERMOSTAT_ACTUATOR)) return "Thermo actuator";
  if (pinacotecaHasError(PIN_ERR_HUMIDIFIER_ACTUATOR)) return "Hum actuator";
  if (pinacotecaHasError(PIN_ERR_STOPLIGHT_ACTUATOR))  return "Stoplight FAIL";
  if (pinacotecaHasError(PIN_ERR_GPIO_IO))             return "GPIO I/O FAIL";
  return "No errors";
}

#endif