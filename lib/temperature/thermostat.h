#include "temperature.h"
#include "../led/led.h"

// Variabili globali per gestire lo stato e il timer in modo non bloccante
unsigned long previous_millis = 0;
int current_state = 0; // 0 = Spento, 1 = Heating, 2 = Cooling

// Imposto a 60000 millisecondi (1 minuto), ma per provarlo metti 5000 (5 secondi) !
const unsigned long PAUSE_TIME = 5000; 

int thermostat(int temperature_sensor, int desired_temperature, int yellow_light, int blu_light) {
    int temperature_value = analogRead(temperature_sensor);
    unsigned long current_millis = millis();
    
    // Formula per il calcolo della temperatura con sensore NTC (Wokwi standard)
    const float BETA = 3950; // Coefficiente Beta del termistore
    float temperature_celsius = 1 / (log(1 / (1023.0 / temperature_value - 1)) / BETA + 1.0 / 298.15) - 273.15;

    // Tolleranza per evitare che il motore si accenda/spenga in continuazione
    float tolerance = 1.0; 

    // 1. Determino l'azione teorica che vorrei fare in base alla temperatura
    int desired_action = 0; 
    if (temperature_celsius < (desired_temperature - tolerance)) {
        desired_action = 1; // Voglio scaldare
    } else if (temperature_celsius > (desired_temperature + tolerance)) {
        desired_action = 2; // Voglio raffreddare
    } else {
        desired_action = 0; // Va bene così, voglio fermare tutto
    }

    // 2. Controllo se devo applicare l'azione desiderata (Logica dello stop di 1 minuto)
    
    // Se l'azione voluta è uguale allo stato attuale, o sto spegnendo (che è immediato)
    if (desired_action == current_state || desired_action == 0) {
        current_state = desired_action;
        
        if (current_state == 0) { // Spegni tutto
             led(yellow_light, LOW);
             led(blu_light, LOW);
        } else if (current_state == 1) { // Mantieni acceso riscaldamento
             led(yellow_light, HIGH);
             led(blu_light, LOW);
        } else if (current_state == 2) { // Mantieni acceso raffreddamento
             led(yellow_light, LOW);
             led(blu_light, HIGH);
        }

        // Resetto il timer per prepararmi a un futuro cambio di stato
        previous_millis = current_millis; 

    } else {
        // Devo CAMBIARE STATO (da freddo a caldo o viceversa).
        // Passaggio 1: Spengo tutto per sicurezza.
        led(yellow_light, LOW);
        led(blu_light, LOW);
        
        // Stampiamo una sola volta in console l'avviso di attesa
        if ((current_millis - previous_millis) % 5000 < 50) { 
           Serial.println("Waiting 1 minute before mode change..."); 
        }

        // Passaggio 2: Controllo il timer
        if (current_millis - previous_millis >= PAUSE_TIME) {
            // È passato il minuto di pausa! Posso finalmente applicare il nuovo stato.
            current_state = desired_action; 
            Serial.println("Mode changed successfully.");
        }
    }

    return true;
}