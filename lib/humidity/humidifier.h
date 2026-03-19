#ifndef HUMIDIFIER_H
#define HUMIDIFIER_H

#include <Arduino.h>
#include "humidity.h"
#include "../led/led.h"

class HumidifierControl {
    private:
        uint8_t _humidifierPin;
        float _targetHumidity;

    public:
        HumidifierControl(int humidifierPin, float targetHumidity)
            : _humidifierPin(static_cast<uint8_t>(humidifierPin)), _targetHumidity(targetHumidity) {}

        void begin() {
            beginHumiditySensor(); // Initialize the DHT sensor
            pinMode(_humidifierPin, OUTPUT);
        }

        bool update() {
            float currentHumidity = readHumidity();
            
            if (currentHumidity == -999.0) { // Check if the humidity reading is valid
                pinacotecaSetError(PIN_ERR_HUM_SENSOR);
                if (!led(_humidifierPin, LOW)) {
                    pinacotecaSetError(PIN_ERR_HUMIDIFIER_ACTUATOR);
                }
                return false;
            }

            pinacotecaClearError(PIN_ERR_HUM_SENSOR);

            float tolerance = 2.0; 

            if (currentHumidity > (_targetHumidity + tolerance)) {
                if (!led(_humidifierPin, HIGH)) {
                    pinacotecaSetError(PIN_ERR_HUMIDIFIER_ACTUATOR);
                    return false;
                }

            } else if (currentHumidity < (_targetHumidity - tolerance)) {
                if (!led(_humidifierPin, LOW)) {
                    pinacotecaSetError(PIN_ERR_HUMIDIFIER_ACTUATOR);
                    return false;
                }
                
            } else {
                if (!led(_humidifierPin, LOW)) {
                    pinacotecaSetError(PIN_ERR_HUMIDIFIER_ACTUATOR);
                    return false;
                }
            }

            pinacotecaClearError(PIN_ERR_HUMIDIFIER_ACTUATOR);
            
            return true;
        }

        void setTargetHumidity(float newHumidityTarget) {
            _targetHumidity = newHumidityTarget;
        }

        float getTargetHumidity() const {
            return _targetHumidity;
        }

        float getCurrentHumidity() const {
            return readHumidity();
        }
};

#endif // HUMIDIFIER_H