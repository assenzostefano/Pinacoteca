#include <Servo.h>

// Local libraries
#include "lib/servomotor/turnstile.h"
#include "lib/led/stoplight.h"
#include "lib/temperature/thermostat.h"
#include "lib/humidity/humidity.h"
#include "lib/humidity/humidifier.h"

int counter_people = 0;

// Pin setup
int in_button = 6;
int out_button = 7;
int servomotor = 3;

int green_light = 4;
int red_light = 5;

int yellow_light = 8; // Raffreddamento
int blu_light = 9; // Riscaldamento

int temperature_sensor = A0;

int humidity_sensor = 11;

Servo myservo;

void setup() {
    Serial.begin(9600);
    dht.begin();
    
    pinMode(in_button, INPUT);
    pinMode(out_button, INPUT);

    pinMode(green_light, OUTPUT);
    pinMode(red_light, OUTPUT);

    pinMode(yellow_light, OUTPUT);
    pinMode(blu_light, OUTPUT);

    pinMode(temperature_sensor, INPUT);
    pinMode(humidity_sensor, INPUT);

    myservo.attach(servomotor);
    myservo.write(0);
}

void loop() {

    /* TODO:
     * 1. [X] mantenere una temperatura dell’ambiente costante a 20°C;
     * 2. [] mantenere un’illuminazione diffusa di 200 LUX
     * 3. [X] mantenere un’umidità relativa del 65%
     * 4. [X] controllare affluenza in sala nel limite di 5 persone presenti
     * 5. [] disporre di un pannello di gestione che visualizzi la situazione in essere, permetta di modificare i parametri sopra indicati con una variazione di +10% o -10%
     * 6. [X] controllare il sistema di climatizzazione dotato di un ingresso (attivo a +12V) per il raffreddamento e un ingresso (attivo a +12V) per il riscaldamento. Il climatizzatore può funzionare solo in raffreddamento o in riscaldamento alternativamente, il cambio stato deve prevedere uno stop di almeno 1 minuto
     * 7. [] controllare il sistema di illuminazione a luce diffusa costituito da 4 plafoniere LED da 40W posizionate a soffitto (altezza 3,0 m)
     * 8. [X] controllare un umidificatore che può prelevare l’umidità dall’aria con una caratteristica di 0,9 lt/h, si attiva con un ingresso alto (+12V)
     * 9. [X] controllare una barriera di ingresso/uscita automatica e un semaforo d’accesso (ROSSO=attendere, VERDE=entrare)
    */

    // Stoplight
    stoplight(counter_people, 5, green_light, red_light);

    // Tornello (max 5 persone)
    turnstile(in_button, out_button, myservo, counter_people);

    // Thermostat (20°C)
    thermostat(temperature_sensor, 20, yellow_light, blu_light);

    // Humidity sensor
    // Humidity Control (Umidificatore)
    float current_humidity = humidity_control();
    if (!isnan(current_humidity)) {
        humidifier_control(current_humidity, 65, 11); // Pin 11 è il LED dell'umidificatore
    }
}