#ifndef LIGHTING_CONTROL_H
#define LIGHTING_CONTROL_H

#include <Arduino.h>
#include "photoresistor.h"
#include "../led/led.h"

class LightingControl {
    private:
        uint8_t _dimmerPin;
        uint16_t _targetLux;
        uint8_t _sensorPin;

    public:
        LightingControl(int sensorPin, int dimmerPin, int targetLux)
            : _dimmerPin(static_cast<uint8_t>(dimmerPin)),
              _targetLux(static_cast<uint16_t>(constrain(targetLux, 0, 65535))),
              _sensorPin(static_cast<uint8_t>(sensorPin)) {}

        void begin() {
            pinMode(_sensorPin, INPUT);
            pinMode(_dimmerPin, OUTPUT);
        }

        bool update() {
            float currentLux = readLux(_sensorPin);
            float missingLight = _targetLux - currentLux;
            
            int pwm_value = 0;
            
            if (missingLight > 0) { // If we need more light, calculate the PWM value
                pwm_value = 255; // TODO: For Wokwi limitations
                pwm_value = constrain(pwm_value, 0, 255);
            } else {
                pwm_value = 0;
            }
            
            ledDimming(_dimmerPin, pwm_value);
            
            return true;
        }

        void setTargetLux(int newLuxTarget) {
            _targetLux = static_cast<uint16_t>(constrain(newLuxTarget, 0, 65535));
        }

        int getTargetLux() const {
            return _targetLux;
        }

        float getCurrentLux() const {
            return readLux(_sensorPin);
        }
};

#endif // LIGHTING_CONTROL_H