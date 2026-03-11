#include <Servo.h>

// Local libraries
#include "lib/servomotor/servomotor.h"
#include "lib/led/stoplight.h"

int counter_people = 0;

// Pin setup
int in_button = 6;
int out_button = 7;
int servomotor = 3;

int green_light = 4;
int red_light = 5;

Servo myservo;

void setup() {
    Serial.begin(9600);
    
    pinMode(in_button, INPUT);
    pinMode(out_button, INPUT);

    pinMode(green_light, OUTPUT);
    pinMode(red_light, OUTPUT);

    myservo.attach(servomotor);
    myservo.write(0); // Posizione chiusa standard (0-180)
}

void loop() {
    // Stoplight
    stoplight(counter_people, 5, green_light, red_light);

    // Tornello (max 5 persone)
    bool in_button_state = digitalRead(in_button);
    bool out_button_state = digitalRead(out_button);

    if (in_button_state == HIGH && counter_people < 5) {
        antiSufferingServo(90, myservo);
        counter_people++;
        delay(1000);
        antiSufferingServo(-90, myservo);
    }

    if (out_button_state == HIGH && counter_people > 0) {
        antiSufferingServo(90, myservo);
        counter_people--;
        delay(1000);
        antiSufferingServo(-90, myservo);
    }
}