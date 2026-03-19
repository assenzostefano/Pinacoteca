#ifndef PHOTORESISTOR_H
#define PHOTORESISTOR_H

#include <Arduino.h>
#include "../system/error_registry.h"

float readLux(int light_sensor_pin) {
    int analogValue = analogRead(light_sensor_pin);
    
    if (analogValue <= 0 || analogValue >= 1023) {
        pinacotecaSetError(PIN_ERR_LIGHT_SENSOR);
        return -999.0;
    }

    pinacotecaClearError(PIN_ERR_LIGHT_SENSOR);
    
    const float GAMMA = 0.7;
    const float RL10 = 50;
    
    // Convert the analog value to voltage
    float voltage = analogValue / 1024.0 * 5.0;
    
    // Calculate the resistance and then the real LUX value
    float resistance = 2000.0 * voltage / (1.0 - voltage / 5.0);
    float lux = pow(RL10 * 1e3 * pow(10, GAMMA) / resistance, (1.0 / GAMMA));
    
    return lux;
}
#endif // PHOTORESISTOR_H