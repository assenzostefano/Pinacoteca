#ifndef LED_H
#define LED_H

#include <Arduino.h>

bool led(int pin, bool state) {
    digitalWrite(pin, state);
    return true;
}

bool ledDimming(int pin, int pwm_value) {
    analogWrite(pin, pwm_value);
    return true;
}

#endif // LED_H