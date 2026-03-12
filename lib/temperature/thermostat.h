#include "temperature.h"
#include "../led/led.h"

unsigned long previous_millis = 0; // Global variables to manage the thermostat state and timing
int current_state = 0; // 0 = OFF, 1 = Heating, 2 = Cooling

// TODO: Set to 5000 milliseconds (5 seconds) for testing purposes. In a real scenario, this should be 60000 ms (1 minute).
const unsigned long PAUSE_TIME = 5000; 

int thermostat(int temperature_sensor, int desired_temperature, int yellow_light, int blu_light) {
    int temperature_value = analogRead(temperature_sensor);
    unsigned long current_millis = millis();
    
    // Formula for converting the raw analog value from the thermistor to Celsius.
    const float BETA = 3950;
    float temperature_celsius = 1 / (log(1 / (1023.0 / temperature_value - 1)) / BETA + 1.0 / 298.15) - 273.15;

    float tolerance = 1.0; 

    int desired_action = 0; 
    if (temperature_celsius < (desired_temperature - tolerance)) { // If the temperature is too low, heat up
        desired_action = 1; // Heating
    } else if (temperature_celsius > (desired_temperature + tolerance)) { // If the temperature is too high, cool down
        desired_action = 2; // Cooling
    } else {
        desired_action = 0; // Stop
    }
    
    if (desired_action == current_state || desired_action == 0) {
        current_state = desired_action;
        
        if (current_state == 0) { // Turn off everything
             led(yellow_light, LOW);
             led(blu_light, LOW);
        } else if (current_state == 1) { // keep heating ON
             led(yellow_light, HIGH);
             led(blu_light, LOW);
        } else if (current_state == 2) { // keep cooling ON
             led(yellow_light, LOW);
             led(blu_light, HIGH);
        }

        previous_millis = current_millis; // Reset timer for the next state change
        
        return true;

    } else {
        // wait for the pause time to elapse before changing the mode

        led(yellow_light, LOW);
        led(blu_light, LOW);

        if ((current_millis - previous_millis) % 5000 < 50) { 
           Serial.println("Waiting 1 minute before mode change..."); 
        }

        if (current_millis - previous_millis >= PAUSE_TIME) { // Check if the pause time has elapsed
            current_state = desired_action; 
            Serial.println("Mode changed successfully.");
        }
    }

    return true;
}