#include <DHT.h>
#define DHTPIN 10
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

float humidity_control() {
    float humidity_value = dht.readHumidity();
    
    if (isnan(humidity_value)) {
        Serial.println("Failed to read from DHT sensor!");
        return false;
    }
    
    return humidity_value;
}