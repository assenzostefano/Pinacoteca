#ifndef SERVOMOTOR_H
#define SERVOMOTOR_H

#include <Arduino.h>
#include <Servo.h>

bool antiSufferingServo(int angle, Servo &s ){
  int pos_attuale = s.read();
  int calcolo = pos_attuale + angle;
  
  if (calcolo > 180 || calcolo < 0){
    Serial.println("Limit movement Servo Motor");
    
    return false;

  } else {
    s.write(calcolo);
    
    return true;
  }
}

#endif