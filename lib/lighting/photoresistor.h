#ifndef PHOTORESISTOR_H
#define PHOTORESISTOR_H

#include <Arduino.h>

float photoresistor_control(int light_sensor_pin) {
    int analogValue = analogRead(light_sensor_pin);
    
    const float GAMMA = 0.7;
    const float RL10 = 50;
    
    // Convert the analog value to voltage
    float voltage = analogValue / 1024.0 * 5.0;
    
    // Calculate the resistance and then the real LUX value
    float resistance = 2000 * voltage / (1 - voltage / 5.0);
    float lux = pow(RL10 * 1e3 * pow(10, GAMMA) / resistance, (1 / GAMMA));
    
    Serial.print("Photoresistor Lux: ");
    Serial.println(lux);
    
    return lux;
}
#endif // PHOTORESISTOR_H
