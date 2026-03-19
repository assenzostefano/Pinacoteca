#ifndef HUMIDITY_H
#define HUMIDITY_H

#include <Arduino.h>
#include <DHT.h> 
#include "../system/error_registry.h"

#define DHTPIN 2 // PIN DHT Sensor
#define DHTTYPE DHT22 // Model (Wokwi supports DHT22)

DHT dht_sensor(DHTPIN, DHTTYPE); 

// Initialize the DHT sensor
void beginHumiditySensor() {
    dht_sensor.begin();
}

float readHumidity() {
    float humidity_value = dht_sensor.readHumidity();
    
    if (isnan(humidity_value)) { // isnan (Is Not A Number) check if the reading is valid, if not it returns a specific error value
        pinacotecaSetError(PIN_ERR_HUM_SENSOR);
        return -999.0;
    }

    pinacotecaClearError(PIN_ERR_HUM_SENSOR);
    
    return humidity_value;
}

#endif // HUMIDITY_H