#include <DHT.h> // Library for DHT sensor
#define DHTPIN 2 // PIN DHT Sensor (Humidity and Temperature)
#define DHTTYPE DHT22 // Specific model of DHT sensor (Wokwi uses DHT22)

DHT dht(DHTPIN, DHTTYPE); // Initialize DHT sensor

float humidity_control() {
    float humidity_value = dht.readHumidity();
    
    if (isnan(humidity_value)) { // isnan (Is Not a Number) check if the reading failed
        Serial.println("Failed to read from DHT sensor!");
        return false;
    }
    
    return humidity_value;
}