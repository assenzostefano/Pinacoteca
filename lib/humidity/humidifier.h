#include "../led/led.h"

bool humidifier_control(float humidity_value, int target_humidity, int humidifier_pin) {    
    // Tolleranza per evitare che umidificatore si accenda/spenga in continuazione
    float tolerance = 2.0; 

    if (humidity_value > (target_humidity + tolerance)) {
        Serial.println("Humidity too high. Dehumidifier ON");
        led(humidifier_pin, HIGH);
        
    // Se l umidita e piu bassa del target (es < 63%), lo spegniamo
    } else if (humidity_value < (target_humidity - tolerance)) {
        Serial.println("Humidity too low. Dehumidifier OFF");
        led(humidifier_pin, LOW);
        
    } else {
        Serial.println("Humidity is optimal.");
        led(humidifier_pin, LOW);
    }
    
    return true;
}