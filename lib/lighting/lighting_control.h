#ifndef LIGHTING_CONTROL_H
#define LIGHTING_CONTROL_H

#include <Arduino.h>

bool lighting_control(float current_lux, int target_lux, int dimmer_pin) {
    float missing_light = target_lux - current_lux;
    
    int pwm_value = 0;
    
    if (missing_light > 0) {
        pwm_value = 255; // Max brightness (only for Wokwi limitations)
        
        pwm_value = constrain(pwm_value, 0, 255); // constrain (arduino function) to ensure it's within 0-255
        
        Serial.print("Dimming lights to: ");
        Serial.print((pwm_value * 100) / 255);
        Serial.println("%");
    } else {
        pwm_value = 0;
        Serial.println("Luce ambientale sufficiente. Plafoniere SPENTE.");
        return false;
    }
    
    Serial.print("Applying PWM value: ");
    Serial.println(pwm_value);
    analogWrite(dimmer_pin, pwm_value); // TODO: Use internal library led.h
    
    return true;
}

#endif // LIGHTING_CONTROL_H