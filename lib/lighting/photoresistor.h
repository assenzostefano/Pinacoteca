#ifndef PHOTORESISTOR_H
#define PHOTORESISTOR_H

#include <Arduino.h>

float readLux(int light_sensor_pin) {
    int analogValue = analogRead(light_sensor_pin);
    
    // Handle edge cases to avoid division by zero or negative lux values
    if (analogValue <= 0) return 0.0;
    if (analogValue >= 1023) return 9999.0;
    
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