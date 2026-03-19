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
            if (currentLux <= -900.0) {
                pinacotecaSetError(PIN_ERR_LIGHT_SENSOR);
                if (!ledDimming(_dimmerPin, 0)) {
                    pinacotecaSetError(PIN_ERR_LIGHT_ACTUATOR);
                }
                return false;
            }

            pinacotecaClearError(PIN_ERR_LIGHT_SENSOR);
            float missingLight = _targetLux - currentLux;
            
            int pwm_value = 0;
            
            if (missingLight > 0) {
#if defined(ARDUINO_ARCH_AVR)
                pwm_value = 255;
#else
                float deficitRatio = missingLight / _targetLux;
                deficitRatio = constrain(deficitRatio, 0.0f, 1.0f);
                pwm_value = static_cast<int>(deficitRatio * 255.0f);
#endif
            } else {
                pwm_value = 0;
            }
            
            if (!ledDimming(_dimmerPin, pwm_value)) {
                pinacotecaSetError(PIN_ERR_LIGHT_ACTUATOR);
                return false;
            }

            pinacotecaClearError(PIN_ERR_LIGHT_ACTUATOR);
            
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