#ifndef TEMPERATURE_H
#define TEMPERATURE_H

#include <Arduino.h>

float readTemperatureCelsius(int sensorPin) { // Read analog value and convert to Celsius
    int temperature_value = analogRead(sensorPin);
    
    // Important! Check for sensor errors (e.g., disconnected sensor) and return a specific error value
    if (temperature_value <= 0 || temperature_value >= 1023) {
        return -999.0;
    }

    const float BETA = 3950.0;
    float temperature_celsius = 1.0 / (log(1.0 / (1023.0 / temperature_value - 1.0)) / BETA + 1.0 / 298.15) - 273.15;
    
    return temperature_celsius;
}

#endif // TEMPERATURE_H