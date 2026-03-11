#include "temperature.h"
#include "../led/led.h"

int thermostat(int temperature_sensor, int desired_temperature, int yellow_light, int blu_light) {
    int temperature_value = analogRead(temperature_sensor);
    
    // Formula per il calcolo della temperatura con sensore NTC (Wokwi standard)
    const float BETA = 3950; // Coefficiente Beta del termistore
    float temperature_celsius = 1 / (log(1 / (1023.0 / temperature_value - 1)) / BETA + 1.0 / 298.15) - 273.15;

    Serial.print("Current Temperature: ");
    Serial.print(temperature_celsius);
    Serial.println(" °C");

    // Tolleranza per evitare che il motore si accenda/spenga in continuazione
    // Se la temperatura desiderata è 20, il sistema considera "ottimale" tra 19.0 e 21.0
    float tolerance = 1.0; 

    if (temperature_celsius < (desired_temperature - tolerance)) {
        Serial.println("Heating ON");
        led(yellow_light, HIGH);
        led(blu_light, LOW);
    } else if (temperature_celsius > (desired_temperature + tolerance)) {
        Serial.println("Cooling ON");
        led(blu_light, HIGH);
        led(yellow_light, LOW);
    } else {
        Serial.println("Temperature is optimal");
        led(yellow_light, LOW);
        led(blu_light, LOW);
    }
    return true;
}