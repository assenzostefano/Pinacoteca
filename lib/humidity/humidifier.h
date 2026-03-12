#include "../led/led.h"

bool humidifier_control(float humidity_value, int target_humidity, int humidifier_pin) {
    float tolerance = 2.0; 

    if (humidity_value > (target_humidity + tolerance)) {
        Serial.println("Humidity too high. Dehumidifier ON");
        led(humidifier_pin, HIGH);

    } else if (humidity_value < (target_humidity - tolerance)) {
        Serial.println("Humidity too low. Dehumidifier OFF");
        led(humidifier_pin, LOW);
        
    } else {
        Serial.println("Humidity is optimal.");
        led(humidifier_pin, LOW);
    }
    
    return true;
}