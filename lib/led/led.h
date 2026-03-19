#ifndef LED_H
#define LED_H

#include <Arduino.h>
#include "../system/error_registry.h"

bool led(int pin, bool state) {
    if (pin < 0) {
        pinacotecaSetError(PIN_ERR_GPIO_IO);
        return false;
    }

    digitalWrite(pin, state);
    pinacotecaClearError(PIN_ERR_GPIO_IO);
    return true;
}

bool ledDimming(int pin, int pwm_value) {
    if (pin < 0) {
        pinacotecaSetError(PIN_ERR_GPIO_IO);
        return false;
    }

    pwm_value = constrain(pwm_value, 0, 255);
    analogWrite(pin, pwm_value);
    pinacotecaClearError(PIN_ERR_GPIO_IO);
    return true;
}

#endif // LED_H