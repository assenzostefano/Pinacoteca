#include <Servo.h>

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
    myservo.write(90);
}

void loop() {
    if (in_button == TRUE && counter_people <= 5) {
        myservo.write(90);
    }
}