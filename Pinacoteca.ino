#include <Servo.h>

// Local libraries
#include "lib/servomotor/servomotor.h"

int counter_people = 0;

// Pin setup
int in_button = 1;
int out_button = 2;
int servomotor = 3;

Servo myservo;

void setup() {
    pinMode(in_button, INPUT);
    pinMode(out_button, INPUT);

    myservo.attach(servomotor);
    myservo.write(-90);
}

void loop() {
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