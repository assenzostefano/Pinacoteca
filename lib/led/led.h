#ifndef LED_H
#define LED_H

#include <Arduino.h>

bool led(int pin, bool state) {
    digitalWrite(pin, state);
    return true;
}

#endif // LED_H