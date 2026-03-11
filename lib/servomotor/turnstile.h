#include "servomotor.h"

bool turnstile(int in_button, int out_button, Servo &myservo, int &counter_people) {
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
    return true;
}