#ifndef HUMIDIFIER_H
#define HUMIDIFIER_H

#include <Arduino.h>
#include "humidity.h"
#include "../led/led.h"

class HumidifierControl {
    private:
        int _humidifierPin;
        float _targetHumidity;

    public:
        HumidifierControl(int humidifierPin, float targetHumidity)
            : _humidifierPin(humidifierPin), _targetHumidity(targetHumidity) {}

        void begin() {
            beginHumiditySensor(); // Initialize the DHT sensor
            pinMode(_humidifierPin, OUTPUT);
        }

        bool update() {
            float currentHumidity = readHumidity();
            
            if (currentHumidity == -999.0) { // Check if the humidity reading is valid
                led(_humidifierPin, LOW);
                return false;
            }

            float tolerance = 2.0; 

            if (currentHumidity > (_targetHumidity + tolerance)) {
                led(_humidifierPin, HIGH); // Too much humidity -> Dehumidifier ON

            } else if (currentHumidity < (_targetHumidity - tolerance)) {
                led(_humidifierPin, LOW); // Too low humidity -> Humidifier ON
                
            } else {
                led(_humidifierPin, LOW); // Optimal humidity -> Both OFF
            }
            
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