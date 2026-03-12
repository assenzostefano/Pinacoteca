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

void setup() {
    Serial.begin(9600);
    
    myServo.attach(PIN_SERVOMOTOR); // Initialize the servo motor
    myServo.write(0);
    entranceTurnstile.begin(&myServo);
    
    trafficLight.begin(); // Initialize the traffic light

    pinMode(PIN_YELLOW_LIGHT, OUTPUT);
    pinMode(PIN_BLU_LIGHT, OUTPUT);

    pinMode(PIN_TEMPERATURE_SENSOR, INPUT);
    pinMode(IS_HUMIDIFIER_LED, OUTPUT);
    pinMode(PIN_PHOTORESISTOR, INPUT);

    pinMode(PIN_PLAFONIERE, OUTPUT);
}

void loop() {
    // Turnstile
    entranceTurnstile.update(); 
    
    // Current people count
    int currentPeople = entranceTurnstile.getPeopleCount();
    
    // Update traffic light based on current people count
    trafficLight.update(currentPeople, 5); 

    // TODO: Add temperature, humidity, and lighting control logic here
}