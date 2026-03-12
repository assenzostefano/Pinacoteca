#include <Servo.h>

#include "lib/servomotor/turnstile.h"
#include "lib/led/stoplight.h"

#include "lib/temperature/thermostat.h"
#include "lib/humidity/humidity.h"
#include "lib/humidity/humidifier.h"
#include "lib/lighting/photoresistor.h"
#include "lib/lighting/lighting_control.h"

// Setup PIN
const int IS_HUMIDIFIER_LED = 11;
const int PIN_SERVOMOTOR = 3;
const int PIN_GREEN_LIGHT = 4;
const int PIN_RED_LIGHT = 5;
const int PIN_IN_BUTTON = 6;
const int PIN_OUT_BUTTON = 7;

const int PIN_BLU_LIGHT = 8; // Riscaldamento
const int PIN_YELLOW_LIGHT = 9; // Raffreddamento

const int PIN_PLAFONIERE = 10;

const int PIN_TEMPERATURE_SENSOR = A0;
const int PIN_PHOTORESISTOR = A1;

// --- CREAZIONE DEGLI OGGETTI ---
Servo myServo;

Turnstile entranceTurnstile(PIN_IN_BUTTON, PIN_OUT_BUTTON, 5); // max 5 people
Stoplight trafficLight(PIN_GREEN_LIGHT, PIN_RED_LIGHT);
Thermostat mainThermostat(PIN_TEMPERATURE_SENSOR, 20.0, PIN_BLU_LIGHT, PIN_YELLOW_LIGHT);
LightingControl galleryLighting(PIN_PHOTORESISTOR, PIN_PLAFONIERE, 200); // Target 200 LUX

void setup() {
    Serial.begin(9600);
    
    // Initialize the servo motor
    myServo.attach(PIN_SERVOMOTOR);
    myServo.write(0);
    entranceTurnstile.begin(&myServo);
    
    trafficLight.begin(); // Initialize the traffic light
    mainThermostat.begin(); // Initialize the thermostat
    galleryLighting.begin(); // Initialize lighting control

    pinMode(IS_HUMIDIFIER_LED, OUTPUT);
}

void loop() {
    // Turnstile
    entranceTurnstile.update(); 
    
    // Current people count
    int currentPeople = entranceTurnstile.getPeopleCount();
    
    // Update traffic light based on current people count
    trafficLight.update(currentPeople, 5); 

    // Thermostat
    mainThermostat.update();

    // Lighting (mantiene illuminazione diffusa di 200 LUX)
    galleryLighting.update();

    // TODO: Add humidity control logic here
}