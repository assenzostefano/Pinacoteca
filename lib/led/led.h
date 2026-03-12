#ifndef LED_H
#define LED_H

#include <Arduino.h>

bool led(int pin, bool state) {
    digitalWrite(pin, state);
    return true;
}

// TODO: Implement functions for dimming

#endif // LED_H