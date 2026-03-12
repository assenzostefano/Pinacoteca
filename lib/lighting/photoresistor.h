#ifndef PHOTORESISTOR_H
#define PHOTORESISTOR_H

#include <Arduino.h>

// Funzione che legge il pin analogico e restituisce il valore convertito in LUX
float read_lux(int light_sensor_pin) {
    // 1. Legge il valore grezzo (da 0 a 1023)
    int analogValue = analogRead(light_sensor_pin);
    
    // 2. Formule matematiche specifiche per la fotoresistenza (Standard Wokwi/Arduino)
    // Queste costanti dipendono dalla combinazione del resistore da 10K e l'LDR
    const float GAMMA = 0.7;
    const float RL10 = 50;

    // Converte il segnale in tensione (Voltage)
    float voltage = analogValue / 1024.0 * 5.0;
    
    // Calcola la resistenza della fotoresistenza
    float resistance = 2000.0 * voltage / (1.0 - voltage / 5.0);
    
    // Converte in LUX
    float lux = pow(RL10 * 1e3 * pow(10, GAMMA) / resistance, (1 / GAMMA));
    
    return lux;
}

#endif // PHOTORESISTOR_H