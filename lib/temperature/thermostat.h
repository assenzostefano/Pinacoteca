#ifndef THERMOSTAT_H
#define THERMOSTAT_H

#include <Arduino.h>
#include "temperature.h"
#include "../led/led.h"

class Thermostat {
    private:
        uint8_t _sensorPin;
        uint8_t _heatingPin;
        uint8_t _coolingPin;
        float _desiredTemperature;
        
        uint8_t _currentState; // 0 = OFF, 1 = Heating, 2 = Cooling
        unsigned long _previousMillis;
        unsigned long _pauseTime;

    public:
        Thermostat(int sensorPin, float desiredTemperature, int heatingPin, int coolingPin)
            : _sensorPin(static_cast<uint8_t>(sensorPin)),
              _heatingPin(static_cast<uint8_t>(heatingPin)),
              _coolingPin(static_cast<uint8_t>(coolingPin)),
              _desiredTemperature(desiredTemperature), _currentState(0), _previousMillis(0), _pauseTime(5000) {} // TODO: 5000ms for testing, real is 60000ms

        void begin() {
            pinMode(_sensorPin, INPUT);
            pinMode(_heatingPin, OUTPUT);
            pinMode(_coolingPin, OUTPUT);
        }

        bool update() {
            float temperature_celsius = readTemperatureCelsius(_sensorPin);
            unsigned long current_millis = millis();
            
            if (temperature_celsius == -999.0) { // If sensor error is detected
                pinacotecaSetError(PIN_ERR_TEMP_SENSOR);
                if (!led(_heatingPin, LOW) || !led(_coolingPin, LOW)) {
                    pinacotecaSetError(PIN_ERR_THERMOSTAT_ACTUATOR);
                }
                return false;
            }

            pinacotecaClearError(PIN_ERR_TEMP_SENSOR);
            
            float tolerance = 1.0; 
            int desired_action = 0; 

            if (temperature_celsius < (_desiredTemperature - tolerance)) {
                desired_action = 1; // Heating
            } else if (temperature_celsius > (_desiredTemperature + tolerance)) {
                desired_action = 2; // Cooling
            } else {
                desired_action = 0; // Stop
            }
            
            if (desired_action == _currentState || desired_action == 0) {
                _currentState = desired_action;
                
                if (_currentState == 0) { // OFF
                     if (!led(_heatingPin, LOW) || !led(_coolingPin, LOW)) {
                        pinacotecaSetError(PIN_ERR_THERMOSTAT_ACTUATOR);
                        return false;
                     }
                } else if (_currentState == 1) { // Heating
                     if (!led(_heatingPin, HIGH) || !led(_coolingPin, LOW)) {
                        pinacotecaSetError(PIN_ERR_THERMOSTAT_ACTUATOR);
                        return false;
                     }
                } else if (_currentState == 2) { // Cooling
                     if (!led(_heatingPin, LOW) || !led(_coolingPin, HIGH)) {
                        pinacotecaSetError(PIN_ERR_THERMOSTAT_ACTUATOR);
                        return false;
                     }
                }

                 pinacotecaClearError(PIN_ERR_THERMOSTAT_ACTUATOR);

                _previousMillis = current_millis; // Reset timer when state changes or is OFF
                return true;

            } else {
                // Pause before changing state to prevent rapid toggling

                if (!led(_heatingPin, LOW) || !led(_coolingPin, LOW)) {
                    pinacotecaSetError(PIN_ERR_THERMOSTAT_ACTUATOR);
                    return false;
                }

                if (current_millis - _previousMillis >= _pauseTime) { 
                    _currentState = desired_action; 
                    Serial.println("Climate mode changed successfully.");
                }

                pinacotecaClearError(PIN_ERR_THERMOSTAT_ACTUATOR);
            }

            return true;
        }

        // Public methods to set/get target temperature and read current temperature
        
        void setTargetTemperature(float newTarget) {
            _desiredTemperature = newTarget;
        }
        
        float getTargetTemperature() const {
            return _desiredTemperature;
        }
        
        float getCurrentTemperature() const {
            return readTemperatureCelsius(_sensorPin);
        }
};

#endif
