#include "led.h"

int stoplight(int counter_people, int max_people, int green_light, int red_light) {
    if (counter_people < max_people) {
        led(green_light, HIGH);
        led(red_light, LOW);

        return 1;

    } else if (counter_people >= max_people) {
        led(green_light, LOW);
        led(red_light, HIGH);

        return 2;
    }
}