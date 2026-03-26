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

bool ledRGB(int pin, String colorName) {
    if (pin < 0) {
        pinacotecaSetError(PIN_ERR_GPIO_IO);
        return false;
    }

    colorName.trim();
    colorName.toUpperCase();

    int color = 0;
    if (colorName == "BLUE") {
        color = 255;
    } else if (colorName == "RED") {
        color = 255 << 16;
    } else if (colorName == "GREEN") {
        color = 255 << 8;
    } else if (colorName == "WHITE") {
        color = (255 << 16) | (255 << 8) | 255;
    } else if (colorName == "YELLOW") {
        color = (255 << 16) | (255 << 8);
    } else if (colorName == "OFF") {
        color = 0;
    } else if (colorName == "BLUE") {
        color = 255;
    } else {
        pinacotecaSetError(PIN_ERR_GPIO_IO);
        return false;
    }

    analogWrite(pin, color); // RGB Led
    pinacotecaClearError(PIN_ERR_GPIO_IO);
    return true;
}

#endif // LED_H