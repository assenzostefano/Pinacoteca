#ifndef ERROR_REGISTRY_H
#define ERROR_REGISTRY_H

#include <Arduino.h>

enum PinacotecaErrorCode : uint8_t {
    PIN_ERR_TEMP_SENSOR = 0,
    PIN_ERR_HUM_SENSOR = 1,
    PIN_ERR_LIGHT_SENSOR = 2,
    PIN_ERR_SERVO = 3,
    PIN_ERR_TURNSTILE = 4,
    PIN_ERR_LIGHT_ACTUATOR = 5,
    PIN_ERR_THERMOSTAT_ACTUATOR = 6,
    PIN_ERR_HUMIDIFIER_ACTUATOR = 7,
    PIN_ERR_STOPLIGHT_ACTUATOR = 8,
    PIN_ERR_GPIO_IO = 9
};

inline uint32_t& pinacotecaErrorFlags() {
    static uint32_t flags = 0;
    return flags;
}

inline void pinacotecaSetError(PinacotecaErrorCode code) {
    pinacotecaErrorFlags() |= (1UL << static_cast<uint8_t>(code));
}

inline void pinacotecaClearError(PinacotecaErrorCode code) {
    pinacotecaErrorFlags() &= ~(1UL << static_cast<uint8_t>(code));
}

inline bool pinacotecaHasError(PinacotecaErrorCode code) {
    return (pinacotecaErrorFlags() & (1UL << static_cast<uint8_t>(code))) != 0;
}

inline bool pinacotecaHasAnyError() {
    return pinacotecaErrorFlags() != 0;
}

inline void pinacotecaClearAllErrors() {
    pinacotecaErrorFlags() = 0;
}

inline const char* pinacotecaFirstErrorText() {
    if (pinacotecaHasError(PIN_ERR_TEMP_SENSOR)) return "Temp sensor FAIL";
    if (pinacotecaHasError(PIN_ERR_HUM_SENSOR)) return "Hum sensor FAIL";
    if (pinacotecaHasError(PIN_ERR_LIGHT_SENSOR)) return "Light sensor FAIL";
    if (pinacotecaHasError(PIN_ERR_SERVO)) return "Servo FAIL";
    if (pinacotecaHasError(PIN_ERR_TURNSTILE)) return "Turnstile FAIL";
    if (pinacotecaHasError(PIN_ERR_LIGHT_ACTUATOR)) return "Lights actuator";
    if (pinacotecaHasError(PIN_ERR_THERMOSTAT_ACTUATOR)) return "Thermo actuator";
    if (pinacotecaHasError(PIN_ERR_HUMIDIFIER_ACTUATOR)) return "Hum actuator";
    if (pinacotecaHasError(PIN_ERR_STOPLIGHT_ACTUATOR)) return "Stoplight FAIL";
    if (pinacotecaHasError(PIN_ERR_GPIO_IO)) return "GPIO I/O FAIL";
    return "No errors";
}

#endif